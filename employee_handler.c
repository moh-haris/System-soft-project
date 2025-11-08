#include "server_handlers.h"

void handle_add_customer_request(int client_socket_fd, struct Request *req)
{
    printf("Processing ADD_CUSTOMER request from Employee ID %d...\n", req->user_id);
    struct Response res;
    struct User new_user;
    struct Customer new_customer;
    struct Account new_account;
    struct Transaction new_transaction;
    int user_fd = -1, cust_fd = -1, acct_fd = -1, tran_fd = -1;
    int user_lock = 0, cust_lock = 0, acct_lock = 0, tran_lock = 0;

    memset(&res, 0, sizeof(res));
    res.success = 1;
    user_fd = open(USER_FILE, O_RDWR | O_CREAT, 0644);
    if (user_fd < 0)
    {
        perror("Server: Error opening user file");
        res.success = 0;
        strcpy(res.message, "Server error: Could not open user data.");
        goto cleanup_customer;
    }
    cust_fd = open(CUSTOMER_FILE, O_RDWR | O_CREAT, 0644);
    if (cust_fd < 0)
    {
        perror("Server: Error opening customer file");
        res.success = 0;
        strcpy(res.message, "Server error: Could not open customer data.");
        goto cleanup_customer;
    }
    acct_fd = open(ACCOUNT_FILE, O_RDWR | O_CREAT, 0644);
    if (acct_fd < 0)
    {
        perror("Server: Error opening account file");
        res.success = 0;
        strcpy(res.message, "Server error: Could not open account data.");
        goto cleanup_customer;
    }
    tran_fd = open(TRANSACTION_FILE, O_RDWR | O_CREAT, 0644);
    if (tran_fd < 0)
    {
        perror("Server: Error opening transaction file");
        res.success = 0;
        strcpy(res.message, "Server error: Could not open transaction data.");
        goto cleanup_customer;
    }

    if (lock_file(user_fd, F_WRLCK) == -1)
    {
        perror("Server: Failed to get user file lock");
        res.success = 0;
        strcpy(res.message, "Server busy. Please try again.");
        goto cleanup_customer;
    }
    user_lock = 1;
    if (lock_file(cust_fd, F_WRLCK) == -1)
    {
        perror("Server: Failed to get customer file lock");
        res.success = 0;
        strcpy(res.message, "Server busy. Please try again.");
        goto cleanup_customer;
    }
    cust_lock = 1;
    if (lock_file(acct_fd, F_WRLCK) == -1)
    {
        perror("Server: Failed to get account file lock");
        res.success = 0;
        strcpy(res.message, "Server busy. Please try again.");
        goto cleanup_customer;
    }
    acct_lock = 1;
    if (lock_file(tran_fd, F_WRLCK) == -1)
    {
        perror("Server: Failed to get transaction file lock");
        res.success = 0;
        strcpy(res.message, "Server busy. Please try again.");
        goto cleanup_customer;
    }
    tran_lock = 1;

    printf("All file locks acquired for ADD_CUSTOMER.\n");
    int max_user_id = get_next_id(user_fd, sizeof(struct User), offsetof(struct User, user_id)) - 1;
    int max_cust_id = get_next_id(cust_fd, sizeof(struct Customer), offsetof(struct Customer, customer_id)) - 1;
    int max_acct_id = get_next_id(acct_fd, sizeof(struct Account), offsetof(struct Account, account_id)) - 1;
    int new_shared_id = 0;
    if (max_user_id > max_cust_id)
        new_shared_id = max_user_id;
    else
        new_shared_id = max_cust_id;
    if (max_acct_id > new_shared_id)
        new_shared_id = max_acct_id;
    new_shared_id = new_shared_id + 1;
    int new_tran_id = get_next_id(tran_fd, sizeof(struct Transaction), offsetof(struct Transaction, transaction_id));

    new_account.account_id = new_shared_id;
    new_account.balance = req->double_data_1;
    new_account.is_active = 1;
    new_customer.customer_id = new_shared_id;
    new_customer.account_id = new_shared_id;
    strncpy(new_customer.name, req->char_data_1, MAX_NAME_LEN);
    new_user.user_id = new_shared_id;
    new_user.customer_or_employee_id = new_shared_id;
    new_user.role = CUSTOMER;
    strncpy(new_user.username, req->username, MAX_USERNAME_LEN);
    strncpy(new_user.password, req->password, MAX_PASSWORD_LEN);
    new_transaction.transaction_id = new_tran_id;
    new_transaction.account_id = new_shared_id;
    new_transaction.type = DEPOSIT;
    new_transaction.amount = req->double_data_1;
    new_transaction.timestamp = time(NULL);

    int write_error = 0;
    lseek(acct_fd, 0, SEEK_END);
    if (write(acct_fd, &new_account, sizeof(new_account)) <= 0)
        write_error = 1;
    lseek(cust_fd, 0, SEEK_END);
    if (write(cust_fd, &new_customer, sizeof(new_customer)) <= 0)
        write_error = 1;
    lseek(user_fd, 0, SEEK_END);
    if (write(user_fd, &new_user, sizeof(new_user)) <= 0)
        write_error = 1;
    lseek(tran_fd, 0, SEEK_END);
    if (write(tran_fd, &new_transaction, sizeof(new_transaction)) <= 0)
        write_error = 1;

    if (write_error)
    {
        perror("Server: Failed to write new customer data");
        res.success = 0;
        strcpy(res.message, "Server error: Failed to save customer.");
    }
    else
    {
        res.success = 1;
        sprintf(res.message, "Success! Customer '%s' added with ID %d and Account ID %d.", new_customer.name, new_user.user_id, new_account.account_id);
    }

cleanup_customer:
    if (user_lock)
        unlock_file(user_fd);
    if (cust_lock)
        unlock_file(cust_fd);
    if (acct_lock)
        unlock_file(acct_fd);
    if (tran_lock)
        unlock_file(tran_fd);
    if (user_lock || cust_lock || acct_lock || tran_lock)
        printf("All file locks released.\n");
    if (user_fd != -1)
        close(user_fd);
    if (cust_fd != -1)
        close(cust_fd);
    if (acct_fd != -1)
        close(acct_fd);
    if (tran_fd != -1)
        close(tran_fd);
    if (write(client_socket_fd, &res, sizeof(res)) < 0)
    {
        perror("Server: write() response failed");
    }
}

void handle_modify_customer_request(int client_socket_fd, struct Request *req)
{
    printf("Processing MODIFY_CUSTOMER by Employee %d for Customer %d...\n", req->user_id, req->int_data_1);
    struct Response res;
    struct Customer customer_record;
    int cust_fd = -1;
    off_t cust_offset = -1;
    int cust_rec_locked = 0;
    int target_customer_id = req->int_data_1;

    memset(&res, 0, sizeof(res));
    res.success = 1;
    cust_fd = open(CUSTOMER_FILE, O_RDWR);
    if (cust_fd < 0)
    {
        perror("Server: Error opening customer file");
        res.success = 0;
        strcpy(res.message, "Server error: Could not access customer data.");
        goto cleanup_modifycust;
    }
    if (!find_customer_record(cust_fd, target_customer_id, &customer_record, &cust_offset))
    {
        res.success = 0;
        strcpy(res.message, "Error: Customer record not found.");
        goto cleanup_modifycust;
    }

    printf("Attempting lock customer offset %ld\n", cust_offset);
    if (lock_record(cust_fd, F_SETLKW, F_WRLCK, cust_offset, sizeof(customer_record)) == -1)
    {
        perror("Server: Failed lock customer record");
        res.success = 0;
        strcpy(res.message, "Server busy (customer record in use).");
        goto cleanup_modifycust;
    }
    cust_rec_locked = 1;
    printf("Customer record lock acquired.\n");

    strncpy(customer_record.name, req->char_data_1, MAX_NAME_LEN);
    customer_record.name[MAX_NAME_LEN - 1] = '\0';
    lseek(cust_fd, cust_offset, SEEK_SET);
    if (write(cust_fd, &customer_record, sizeof(customer_record)) <= 0)
    {
        perror("Server: Failed write updated customer record");
        res.success = 0;
        strcpy(res.message, "Server error: Failed save customer details.");
    }
    else
    {
        res.success = 1;
        sprintf(res.message, "Customer ID %d details updated successfully.", target_customer_id);
        printf("Customer ID %d updated by Employee %d.\n", target_customer_id, req->user_id);
    }

    if (cust_rec_locked)
    {
        lock_record(cust_fd, F_SETLK, F_UNLCK, cust_offset, sizeof(customer_record));
        cust_rec_locked = 0;
        printf("Customer record lock released.\n");
    }

cleanup_modifycust:
    if (cust_rec_locked)
    {
        lock_record(cust_fd, F_SETLK, F_UNLCK, cust_offset, sizeof(customer_record));
        printf("Customer record lock released cleanup.\n");
    }
    if (cust_fd != -1)
        close(cust_fd);
    if (write(client_socket_fd, &res, sizeof(res)) < 0)
    {
        perror("Server: write() response failed");
    }
}

void handle_view_customer_txn_request(int client_socket_fd, struct Request *req)
{
    printf("Processing VIEW_CUSTOMER_TXN request from Employee ID %d for Customer ID %d...\n", req->user_id, req->int_data_1);
    struct Response res;
    struct Transaction transaction;
    int target_customer_id = req->int_data_1;
    int account_id = -1;
    int tran_fd = -1;
    int count = 0;

    memset(&res, 0, sizeof(res));
    account_id = get_account_id_from_user_id(target_customer_id);
    if (account_id == -1)
    {
        res.success = 0;
        strcpy(res.message, "Error: Could not find an account for the specified Customer ID.");
        write(client_socket_fd, &res, sizeof(res));
        return;
    }
    tran_fd = open(TRANSACTION_FILE, O_RDONLY);
    if (tran_fd < 0)
    {
        perror("Server: Error opening transaction file");
        res.success = 0;
        strcpy(res.message, "Server error: Could not read transaction history.");
        write(client_socket_fd, &res, sizeof(res));
        return;
    }
    if (lock_file(tran_fd, F_RDLCK) == -1)
    {
        perror("Server: Failed to lock transaction file");
        res.success = 0;
        strcpy(res.message, "Server busy. Please try again.");
        write(client_socket_fd, &res, sizeof(res));
        close(tran_fd);
        return;
    }

    ssize_t read_bytes;
    while ((read_bytes = read(tran_fd, &transaction, sizeof(transaction))) == sizeof(transaction))
    {
        if (transaction.account_id == account_id)
        {
            count++;
            res.success = 1;
            time_t tx_time = transaction.timestamp;
            struct tm *tm_info = localtime(&tx_time);
            char time_str[30];
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
            snprintf(res.message, sizeof(res.message), "ID: %d | Type: %-8s | Amount: %8.2f | Time: %s",
                     transaction.transaction_id, get_transaction_type_string(transaction.type), transaction.amount, time_str);
            if (write(client_socket_fd, &res, sizeof(res)) < 0)
            {
                perror("Server: write() transaction failed");
                break;
            }
        }
    }
    if (read_bytes < 0)
    {
        perror("Server: Error reading transaction file");
    }
    unlock_file(tran_fd);
    close(tran_fd);

    memset(&res, 0, sizeof(res));
    res.success = 0;
    if (count == 0 && read_bytes == 0)
    {
        strcpy(res.message, "No transactions found for this account.");
    }
    else if (count > 0 && read_bytes >= 0)
    {
        strcpy(res.message, "End of transaction history.");
    }
    else
    {
        strcpy(res.message, "Error occurred while retrieving history.");
    }
    if (write(client_socket_fd, &res, sizeof(res)) < 0)
    {
        perror("Server: write() end-of-list failed");
    }
}

void handle_view_assigned_loans_request(int client_socket_fd, struct Request *req)
{
    printf("Processing VIEW_ASSIGNED_LOANS request from Employee ID %d...\n", req->user_id);
    struct Response res;
    struct Loan loan_record;
    int loan_fd = -1;
    int count = 0;
    int employee_id = req->user_id;

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
        if (loan_record.assigned_to_employee_id == employee_id && loan_record.status == 0)
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
        strcpy(res.message, "No assigned loans found.");
    }
    else if (count > 0 && read_bytes >= 0)
    {
        strcpy(res.message, "End of assigned loan list.");
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

void handle_process_loan_request(int client_socket_fd, struct Request *req)
{
    printf("Processing PROCESS_LOAN request from Employee ID %d for Loan ID %d...\n", req->user_id, req->int_data_1);
    struct Response res;
    struct Loan loan_record;
    struct Account account_record;
    struct Transaction new_transaction;
    int loan_fd = -1, acct_fd = -1, tran_fd = -1;
    off_t loan_offset = -1, acct_offset = -1;
    int loan_rec_locked = 0, acct_rec_locked = 0, tran_file_locked = 0;
    int target_loan_id = req->int_data_1;
    int action = req->int_data_2; // 1=Approve, 2=Reject
    int employee_id = req->user_id;

    memset(&res, 0, sizeof(res));
    res.success = 1;

    loan_fd = open(LOAN_FILE, O_RDWR);
    if (loan_fd < 0)
    {
        perror("Server: Error opening loan file");
        res.success = 0;
        strcpy(res.message, "Server error: Could not access loan data.");
        goto cleanup_process_loan;
    }
    if (!find_loan_record(loan_fd, target_loan_id, &loan_record, &loan_offset))
    {
        res.success = 0;
        strcpy(res.message, "Error: Loan record not found.");
        goto cleanup_process_loan;
    }
    if (lock_record(loan_fd, F_SETLKW, F_WRLCK, loan_offset, sizeof(struct Loan)) == -1)
    {
        perror("Server: Failed to lock loan record");
        res.success = 0;
        strcpy(res.message, "Server busy (loan record in use).");
        goto cleanup_process_loan;
    }
    loan_rec_locked = 1;
    printf("Loan record lock acquired.\n");

    if (loan_record.assigned_to_employee_id != employee_id)
    {
        res.success = 0;
        strcpy(res.message, "Error: This loan is not assigned to you.");
    }
    else if (loan_record.status != 0)
    {
        res.success = 0;
        strcpy(res.message, "Error: This loan is not pending.");
    }
    if (res.success == 0)
    {
        goto cleanup_process_loan;
    }

    if (action == 2)
    {                           // REJECT
        loan_record.status = 2; // 2 = Rejected
        lseek(loan_fd, loan_offset, SEEK_SET);
        if (write(loan_fd, &loan_record, sizeof(struct Loan)) <= 0)
        {
            perror("Server: Failed to write rejected loan status");
            res.success = 0;
            strcpy(res.message, "Server error: Failed to update loan status.");
        }
        else
        {
            res.success = 1;
            sprintf(res.message, "Loan ID %d has been rejected.", target_loan_id);
        }
    }
    else if (action == 1)
    { // APPROVE
        acct_fd = open(ACCOUNT_FILE, O_RDWR);
        if (acct_fd < 0)
        {
            perror("Server: Error opening account file");
            res.success = 0;
            strcpy(res.message, "Server error: Could not access account data.");
            goto cleanup_process_loan;
        }
        if (!find_account_record(acct_fd, loan_record.customer_id, &account_record, &acct_offset))
        {
            res.success = 0;
            strcpy(res.message, "Error: Customer account for loan not found.");
            goto cleanup_process_loan;
        }
        if (lock_record(acct_fd, F_SETLKW, F_WRLCK, acct_offset, sizeof(struct Account)) == -1)
        {
            perror("Server: Failed to lock account record");
            res.success = 0;
            strcpy(res.message, "Server busy (customer account in use).");
            goto cleanup_process_loan;
        }
        acct_rec_locked = 1;

        account_record.balance += loan_record.amount;
        lseek(acct_fd, acct_offset, SEEK_SET);
        if (write(acct_fd, &account_record, sizeof(struct Account)) <= 0)
        {
            perror("Server: Failed to write new balance");
            res.success = 0;
            strcpy(res.message, "Server error: Failed to disburse loan.");
            goto cleanup_process_loan;
        }
        lock_record(acct_fd, F_SETLK, F_UNLCK, acct_offset, sizeof(struct Account));
        acct_rec_locked = 0;

        loan_record.status = 1; // 1 = Approved
        lseek(loan_fd, loan_offset, SEEK_SET);
        if (write(loan_fd, &loan_record, sizeof(struct Loan)) <= 0)
        {
            perror("Server: Failed to write approved loan status");
            res.success = 0;
            strcpy(res.message, "Loan approved, but failed to update loan status.");
        }
        else
        {
            res.success = 1;
            sprintf(res.message, "Loan ID %d approved. $%.2f disbursed to account %d.", target_loan_id, loan_record.amount, loan_record.customer_id);
        }

        if (res.success == 1)
        {
            tran_fd = open(TRANSACTION_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (tran_fd < 0)
            {
                strcpy(res.message, "Loan approved, but failed to log transaction.");
            }
            else
            {
                if (lock_file(tran_fd, F_WRLCK) == -1)
                {
                    strcpy(res.message, "Loan approved, but failed to log transaction.");
                }
                else
                {
                    tran_file_locked = 1;
                    int new_tran_id = get_next_id(tran_fd, sizeof(struct Transaction), offsetof(struct Transaction, transaction_id));
                    new_transaction.transaction_id = new_tran_id;
                    new_transaction.account_id = loan_record.customer_id;
                    new_transaction.type = DEPOSIT;
                    new_transaction.amount = loan_record.amount;
                    new_transaction.timestamp = time(NULL);
                    if (write(tran_fd, &new_transaction, sizeof(struct Transaction)) <= 0)
                    {
                        strcpy(res.message, "Loan approved, but failed to log transaction.");
                    }
                    unlock_file(tran_fd);
                    tran_file_locked = 0;
                }
                close(tran_fd);
                tran_fd = -1;
            }
        }
    }

cleanup_process_loan:
    if (loan_rec_locked)
        lock_record(loan_fd, F_SETLK, F_UNLCK, loan_offset, sizeof(struct Loan));
    if (acct_rec_locked)
        lock_record(acct_fd, F_SETLK, F_UNLCK, acct_offset, sizeof(struct Account));
    if (tran_file_locked)
        unlock_file(tran_fd);
    if (loan_fd != -1)
        close(loan_fd);
    if (acct_fd != -1)
        close(acct_fd);
    if (tran_fd != -1)
        close(tran_fd);
    if (write(client_socket_fd, &res, sizeof(struct Response)) < 0)
    {
        perror("Server: write() response failed");
    }
}
