#include "server_handlers.h"

void handle_toggle_account_status_request(int client_socket_fd, struct Request *req)
{
    printf("Processing TOGGLE_ACCOUNT_STATUS by Manager %d for Account %d...\n", req->user_id, req->int_data_1);
    struct Response res;
    struct Account account_record;
    int acct_fd = -1;
    off_t acct_offset = -1;
    int acct_rec_locked = 0;
    int target_account_id = req->int_data_1;
    int new_status = (int)req->double_data_1;

    memset(&res, 0, sizeof(res));
    res.success = 1;
    acct_fd = open(ACCOUNT_FILE, O_RDWR);
    if (acct_fd < 0)
    {
        perror("Server: Error opening account file");
        res.success = 0;
        strcpy(res.message, "Server error: Could not access account data.");
        goto cleanup_toggle;
    }
    if (!find_account_record(acct_fd, target_account_id, &account_record, &acct_offset))
    {
        res.success = 0;
        strcpy(res.message, "Error: Account record not found.");
        goto cleanup_toggle;
    }

    printf("Attempting lock account offset %ld\n", acct_offset);
    if (lock_record(acct_fd, F_SETLKW, F_WRLCK, acct_offset, sizeof(account_record)) == -1)
    {
        perror("Server: Failed lock account record");
        res.success = 0;
        strcpy(res.message, "Server busy (account record in use).");
        goto cleanup_toggle;
    }
    acct_rec_locked = 1;
    printf("Account record lock acquired.\n");

    if (account_record.is_active == new_status)
    {
        sprintf(res.message, "Account %d is already %s.", target_account_id, new_status ? "active" : "inactive");
        res.success = 0;
    }
    else
    {
        account_record.is_active = new_status;
        lseek(acct_fd, acct_offset, SEEK_SET);
        if (write(acct_fd, &account_record, sizeof(account_record)) <= 0)
        {
            perror("Server: Failed write updated account record");
            res.success = 0;
            strcpy(res.message, "Server error: Failed to update status.");
        }
        else
        {
            res.success = 1;
            sprintf(res.message, "Account %d has been %s.", target_account_id, new_status ? "activated" : "deactivated");
            printf("Account %d status changed to %d by Manager %d.\n", target_account_id, new_status, req->user_id);
        }
    }

    if (acct_rec_locked)
    {
        lock_record(acct_fd, F_SETLK, F_UNLCK, acct_offset, sizeof(account_record));
        acct_rec_locked = 0;
        printf("Account record lock released.\n");
    }

cleanup_toggle:
    if (acct_rec_locked)
    {
        lock_record(acct_fd, F_SETLK, F_UNLCK, acct_offset, sizeof(account_record));
        printf("Account record lock released cleanup.\n");
    }
    if (acct_fd != -1)
        close(acct_fd);
    if (write(client_socket_fd, &res, sizeof(res)) < 0)
    {
        perror("Server: write() response failed");
    }
}

void handle_review_feedback_request(int client_socket_fd, struct Request *req)
{
    printf("Processing VIEW_FEEDBACK request from Manager ID %d...\n", req->user_id);
    struct Response res;
    struct Feedback feedback_record;
    int fb_fd = -1;
    int count = 0;

    memset(&res, 0, sizeof(res));
    fb_fd = open(FEEDBACK_FILE, O_RDONLY);
    if (fb_fd < 0)
    {
        perror("Server: Error opening feedback file");
        res.success = 0;
        strcpy(res.message, "Server error: Could not read feedback.");
        write(client_socket_fd, &res, sizeof(res));
        return;
    }
    if (lock_file(fb_fd, F_RDLCK) == -1)
    {
        perror("Server: Failed to lock feedback file");
        res.success = 0;
        strcpy(res.message, "Server busy. Please try again.");
        write(client_socket_fd, &res, sizeof(res));
        close(fb_fd);
        return;
    }

    ssize_t read_bytes;
    while ((read_bytes = read(fb_fd, &feedback_record, sizeof(feedback_record))) == sizeof(feedback_record))
    {
        count++;
        res.success = 1;
        time_t tx_time = feedback_record.timestamp;
        struct tm *tm_info = localtime(&tx_time);
        char time_str[30];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
        snprintf(res.message, sizeof(res.message), "From Customer ID: %d | Time: %s\nMessage: %s",
                 feedback_record.customer_id, time_str, feedback_record.message);
        if (write(client_socket_fd, &res, sizeof(res)) < 0)
        {
            perror("Server: write() feedback failed");
            break;
        }
    }
    if (read_bytes < 0)
    {
        perror("Server: Error reading feedback file");
    }
    unlock_file(fb_fd);
    close(fb_fd);

    memset(&res, 0, sizeof(res));
    res.success = 0;
    if (count == 0 && read_bytes == 0)
    {
        strcpy(res.message, "No feedback found.");
    }
    else if (count > 0 && read_bytes >= 0)
    {
        strcpy(res.message, "End of feedback.");
    }
    else
    {
        strcpy(res.message, "Error retrieving feedback.");
    }
    if (write(client_socket_fd, &res, sizeof(res)) < 0)
    {
        perror("Server: write() end-of-list failed");
    }
}

void handle_view_unassigned_loans_request(int client_socket_fd, struct Request *req)
{
    printf("Processing VIEW_UNASSIGNED_LOANS request from Manager ID %d...\n", req->user_id);
    struct Response res;
    struct Loan loan_record;
    int loan_fd = -1;
    int count = 0;

    memset(&res, 0, sizeof(res));
    loan_fd = open(LOAN_FILE, O_RDONLY);
    if (loan_fd < 0)
    {
        perror("Server: Error opening loan file");
        res.success = 0;
        strcpy(res.message, "Server error: Could not read loan data.");
        write(client_socket_fd, &res, sizeof(res));
        return;
    }
    if (lock_file(loan_fd, F_RDLCK) == -1)
    {
        perror("Server: Failed to lock loan file");
        res.success = 0;
        strcpy(res.message, "Server busy. Please try again.");
        write(client_socket_fd, &res, sizeof(res));
        close(loan_fd);
        return;
    }

    ssize_t read_bytes;
    while ((read_bytes = read(loan_fd, &loan_record, sizeof(loan_record))) == sizeof(loan_record))
    {
        if (loan_record.status == 0 && loan_record.assigned_to_employee_id == -1)
        {
            count++;
            res.success = 1;
            snprintf(res.message, sizeof(res.message), "Loan ID: %d | Customer ID: %d | Amount: $%.2f",
                     loan_record.loan_id, loan_record.customer_id, loan_record.amount);
            if (write(client_socket_fd, &res, sizeof(res)) < 0)
            {
                perror("Server: write() loan record failed");
                break;
            }
        }
    }
    if (read_bytes < 0)
    {
        perror("Server: Error reading loan file");
    }
    unlock_file(loan_fd);
    close(loan_fd);

    memset(&res, 0, sizeof(res));
    res.success = 0;
    if (count == 0 && read_bytes == 0)
    {
        strcpy(res.message, "No unassigned loans found.");
    }
    else if (count > 0 && read_bytes >= 0)
    {
        strcpy(res.message, "End of unassigned loan list.");
    }
    else
    {
        strcpy(res.message, "Error occurred while retrieving loans.");
    }
    if (write(client_socket_fd, &res, sizeof(res)) < 0)
    {
        perror("Server: write() end-of-list failed");
    }
}

void handle_assign_loan_request(int client_socket_fd, struct Request *req)
{
    printf("Processing ASSIGN_LOAN request from Manager ID %d...\n", req->user_id);
    struct Response res;
    struct Loan loan_record;
    int loan_fd = -1;
    off_t loan_offset = -1;
    int loan_rec_locked = 0;
    int target_loan_id = req->int_data_1;
    int target_employee_id = req->int_data_2;

    memset(&res, 0, sizeof(res));
    res.success = 1;
    loan_fd = open(LOAN_FILE, O_RDWR);
    if (loan_fd < 0)
    {
        perror("Server: Error opening loan file");
        res.success = 0;
        strcpy(res.message, "Server error: Could not access loan data.");
        goto cleanup_assign_loan;
    }
    if (!find_loan_record(loan_fd, target_loan_id, &loan_record, &loan_offset))
    {
        res.success = 0;
        strcpy(res.message, "Error: Loan record not found.");
        goto cleanup_assign_loan;
    }

    printf("Attempting to lock loan record at offset %ld\n", loan_offset);
    if (lock_record(loan_fd, F_SETLKW, F_WRLCK, loan_offset, sizeof(struct Loan)) == -1)
    {
        perror("Server: Failed to get write lock on loan record");
        res.success = 0;
        strcpy(res.message, "Server busy (loan record in use).");
        goto cleanup_assign_loan;
    }
    loan_rec_locked = 1;
    printf("Loan record lock acquired.\n");

    if (loan_record.status != 0 || loan_record.assigned_to_employee_id != -1)
    {
        res.success = 0;
        sprintf(res.message, "Loan ID %d is not pending or is already assigned.", target_loan_id);
    }
    else
    {
        loan_record.assigned_to_employee_id = target_employee_id;
        lseek(loan_fd, loan_offset, SEEK_SET);
        if (write(loan_fd, &loan_record, sizeof(struct Loan)) <= 0)
        {
            perror("Server: Failed to write updated loan record");
            res.success = 0;
            strcpy(res.message, "Server error: Failed to assign loan.");
        }
        else
        {
            res.success = 1;
            sprintf(res.message, "Loan ID %d assigned to Employee ID %d.", target_loan_id, target_employee_id);
            printf("Loan %d assigned to Employee %d by Manager %d.\n", target_loan_id, target_employee_id, req->user_id);
        }
    }

    if (loan_rec_locked)
    {
        lock_record(loan_fd, F_SETLK, F_UNLCK, loan_offset, sizeof(struct Loan));
        printf("Loan record lock released.\n");
    }

cleanup_assign_loan:
    if (loan_rec_locked)
    {
        lock_record(loan_fd, F_SETLK, F_UNLCK, loan_offset, sizeof(struct Loan));
        printf("Loan record lock released during cleanup.\n");
    }
    if (loan_fd != -1)
        close(loan_fd);
    if (write(client_socket_fd, &res, sizeof(res)) < 0)
    {
        perror("Server: write() response failed");
    }
}
