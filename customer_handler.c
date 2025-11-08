#include "server_handlers.h"

void handle_view_balance_request(int client_socket_fd, struct Request *req)
{
    printf("Processing VIEW_BALANCE request for Customer ID %d...\n", req->user_id);
    struct Response res;
    struct Account account;
    int account_id = -1;
    int found = 0;
    int acct_fd = -1;
    memset(&res, 0, sizeof(res));

    account_id = get_account_id_from_user_id(req->user_id);
    if (account_id == -1)
    {
        res.success = 0;
        strcpy(res.message, "Error: Could not find a matching account for your user ID.");
    }
    else
    {
        acct_fd = open(ACCOUNT_FILE, O_RDONLY);
        if (acct_fd < 0)
        {
            perror("Server: Error opening account file");
            res.success = 0;
            strcpy(res.message, "Server error: Could not read account data.");
        }
        else
        {
            if (lock_file(acct_fd, F_RDLCK) == -1)
            {
                perror("Server: Failed to get read lock");
                res.success = 0;
                strcpy(res.message, "Server busy.");
            }
            else
            {
                ssize_t read_bytes;
                while ((read_bytes = read(acct_fd, &account, sizeof(account))) == sizeof(account))
                {
                    if (account.account_id == account_id)
                    {
                        found = 1;
                        if (account.is_active == 0)
                        {
                            res.success = 0;
                            strcpy(res.message, "Account is deactivated. Please contact the bank.");
                        }
                        else
                        {
                            res.success = 1;
                            res.double_data_1 = account.balance;
                            sprintf(res.message, "Balance retrieved.");
                        }
                        break;
                    }
                }
                if (read_bytes < 0)
                {
                    perror("Server: Error reading account file");
                    res.success = 0;
                    strcpy(res.message, "Server error: Failed to read balance.");
                }
                if (!found && res.success != 0)
                {
                    res.success = 0;
                    strcpy(res.message, "Error: Account data not found.");
                }
                unlock_file(acct_fd);
            }
            close(acct_fd);
        }
    }
    if (write(client_socket_fd, &res, sizeof(res)) < 0)
    {
        perror("Server: write() response failed");
    }
}

void handle_deposit_request(int client_socket_fd, struct Request *req)
{
    printf("Processing DEPOSIT request for Customer ID %d...\n", req->user_id);
    struct Response res;
    struct Account account;
    struct Transaction new_transaction;
    int account_id = -1;
    int acct_fd = -1, tran_fd = -1;
    int acct_rec_locked = 0;
    int tran_file_locked = 0;
    off_t account_offset = -1;

    memset(&res, 0, sizeof(res));
    res.success = 1;
    account_id = get_account_id_from_user_id(req->user_id);
    if (account_id == -1)
    {
        res.success = 0;
        strcpy(res.message, "Error: Could not find account for user ID.");
        goto cleanup_deposit;
    }
    acct_fd = open(ACCOUNT_FILE, O_RDWR);
    if (acct_fd < 0)
    {
        perror("Server: Error opening account file");
        res.success = 0;
        strcpy(res.message, "Server error: Could not read account data.");
        goto cleanup_deposit;
    }

    ssize_t read_bytes;
    int found = 0;
    while ((read_bytes = read(acct_fd, &account, sizeof(account))) == sizeof(account))
    {
        if (account.account_id == account_id)
        {
            found = 1;
            account_offset = lseek(acct_fd, 0, SEEK_CUR) - sizeof(account);
            if (account.is_active == 0)
            {
                res.success = 0;
                strcpy(res.message, "Deposit failed: Account is deactivated.");
                goto cleanup_deposit;
            }
            printf("Attempting lock offset %ld\n", account_offset);
            if (lock_record(acct_fd, F_SETLKW, F_WRLCK, account_offset, sizeof(account)) == -1)
            {
                perror("Server: Failed lock");
                res.success = 0;
                strcpy(res.message, "Server busy (account in use).");
            }
            else
            {
                acct_rec_locked = 1;
            }
            break;
        }
    }

    if (read_bytes < 0)
    {
        perror("Server: Error reading account file");
        res.success = 0;
        strcpy(res.message, "Server error: Failed read account.");
    }
    if (!found && res.success != 0)
    {
        res.success = 0;
        strcpy(res.message, "Error: Account data not found.");
    }
    if (res.success == 0)
    {
        goto cleanup_deposit;
    }

    printf("Account record lock acquired.\n");
    account.balance += req->double_data_1;
    lseek(acct_fd, account_offset, SEEK_SET);
    if (write(acct_fd, &account, sizeof(account)) <= 0)
    {
        perror("Server: Failed write updated account");
        res.success = 0;
        strcpy(res.message, "Server error: Failed save deposit.");
    }
    else
    {
        res.success = 1;
        printf("Account balance updated.\n");
    }

    if (acct_rec_locked)
    {
        lock_record(acct_fd, F_SETLK, F_UNLCK, account_offset, sizeof(account));
        acct_rec_locked = 0;
        printf("Account record lock released.\n");
    }

    if (res.success == 1)
    {
        tran_fd = open(TRANSACTION_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (tran_fd < 0)
        {
            perror("Server: Error opening transaction file");
            strcpy(res.message, "Deposit ok, but failed log transaction.");
        }
        else
        {
            if (lock_file(tran_fd, F_WRLCK) == -1)
            {
                perror("Server: Failed lock transaction file");
                strcpy(res.message, "Deposit ok, but failed log transaction.");
            }
            else
            {
                tran_file_locked = 1;
                int new_tran_id = get_next_id(tran_fd, sizeof(struct Transaction), offsetof(struct Transaction, transaction_id));
                new_transaction.transaction_id = new_tran_id;
                new_transaction.account_id = account_id;
                new_transaction.type = DEPOSIT;
                new_transaction.amount = req->double_data_1;
                new_transaction.timestamp = time(NULL);
                if (write(tran_fd, &new_transaction, sizeof(new_transaction)) <= 0)
                {
                    perror("Server: Failed write transaction");
                    strcpy(res.message, "Deposit ok, but failed log transaction.");
                }
                else
                {
                    res.double_data_1 = account.balance;
                    sprintf(res.message, "Deposit successful.");
                }
                unlock_file(tran_fd);
                tran_file_locked = 0;
            }
            close(tran_fd);
            tran_fd = -1;
        }
    }

cleanup_deposit:
    if (acct_rec_locked)
    {
        lock_record(acct_fd, F_SETLK, F_UNLCK, account_offset, sizeof(account));
        printf("Account record lock released cleanup.\n");
    }
    if (tran_file_locked)
    {
        unlock_file(tran_fd);
        printf("Transaction file lock released cleanup.\n");
    }
    if (acct_fd != -1)
        close(acct_fd);
    if (tran_fd != -1)
        close(tran_fd);
    if (write(client_socket_fd, &res, sizeof(res)) < 0)
    {
        perror("Server: write() response failed");
    }
}

void handle_withdraw_request(int client_socket_fd, struct Request *req)
{
    printf("Processing WITHDRAW request for Customer ID %d...\n", req->user_id);
    struct Response res;
    struct Account account;
    struct Transaction new_transaction;
    int account_id = -1;
    int acct_fd = -1, tran_fd = -1;
    int acct_rec_locked = 0;
    int tran_file_locked = 0;
    off_t account_offset = -1;

    memset(&res, 0, sizeof(res));
    res.success = 1;
    account_id = get_account_id_from_user_id(req->user_id);
    if (account_id == -1)
    {
        res.success = 0;
        strcpy(res.message, "Error: Could not find account for user ID.");
        goto cleanup_withdraw;
    }
    acct_fd = open(ACCOUNT_FILE, O_RDWR);
    if (acct_fd < 0)
    {
        perror("Server: Error opening account file");
        res.success = 0;
        strcpy(res.message, "Server error: Could not read account data.");
        goto cleanup_withdraw;
    }

    ssize_t read_bytes;
    int found = 0;
    while ((read_bytes = read(acct_fd, &account, sizeof(account))) == sizeof(account))
    {
        if (account.account_id == account_id)
        {
            found = 1;
            account_offset = lseek(acct_fd, 0, SEEK_CUR) - sizeof(account);
            if (account.is_active == 0)
            {
                res.success = 0;
                strcpy(res.message, "Withdrawal failed: Account is deactivated.");
                goto cleanup_withdraw;
            }
            printf("Attempting lock offset %ld\n", account_offset);
            if (lock_record(acct_fd, F_SETLKW, F_WRLCK, account_offset, sizeof(account)) == -1)
            {
                perror("Server: Failed lock");
                res.success = 0;
                strcpy(res.message, "Server busy (account in use).");
            }
            else
            {
                acct_rec_locked = 1;
            }
            break;
        }
    }

    if (read_bytes < 0)
    {
        perror("Server: Error reading account file");
        res.success = 0;
        strcpy(res.message, "Server error: Failed read account.");
    }
    if (!found && res.success != 0)
    {
        res.success = 0;
        strcpy(res.message, "Error: Account data not found.");
    }
    if (res.success == 0)
    {
        goto cleanup_withdraw;
    }

    printf("Account record lock acquired.\n");
    if (account.balance < req->double_data_1)
    {
        res.success = 0;
        sprintf(res.message, "Withdrawal failed: Insufficient funds. Balance: $%.2f", account.balance);
    }
    else
    {
        account.balance -= req->double_data_1;
        lseek(acct_fd, account_offset, SEEK_SET);
        if (write(acct_fd, &account, sizeof(account)) <= 0)
        {
            perror("Server: Failed write updated account");
            res.success = 0;
            strcpy(res.message, "Server error: Failed save withdrawal.");
        }
        else
        {
            res.success = 1;
            printf("Account balance updated after withdrawal.\n");
            tran_fd = open(TRANSACTION_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (tran_fd < 0)
            {
                perror("Server: Error opening transaction file");
                strcpy(res.message, "Withdrawal ok, but failed log transaction.");
            }
            else
            {
                if (lock_file(tran_fd, F_WRLCK) == -1)
                {
                    perror("Server: Failed lock transaction file");
                    strcpy(res.message, "Withdrawal ok, but failed log transaction.");
                }
                else
                {
                    tran_file_locked = 1;
                    int new_tran_id = get_next_id(tran_fd, sizeof(struct Transaction), offsetof(struct Transaction, transaction_id));
                    new_transaction.transaction_id = new_tran_id;
                    new_transaction.account_id = account_id;
                    new_transaction.type = WITHDRAW;
                    new_transaction.amount = req->double_data_1;
                    new_transaction.timestamp = time(NULL);
                    if (write(tran_fd, &new_transaction, sizeof(new_transaction)) <= 0)
                    {
                        perror("Server: Failed write transaction");
                        strcpy(res.message, "Withdrawal ok, but failed log transaction.");
                    }
                    else
                    {
                        res.double_data_1 = account.balance;
                        sprintf(res.message, "Withdrawal successful.");
                    }
                    unlock_file(tran_fd);
                    tran_file_locked = 0;
                }
                close(tran_fd);
                tran_fd = -1;
            }
        }
    }

    if (acct_rec_locked)
    {
        lock_record(acct_fd, F_SETLK, F_UNLCK, account_offset, sizeof(account));
        acct_rec_locked = 0;
        printf("Account record lock released.\n");
    }

cleanup_withdraw:
    if (acct_rec_locked)
    {
        lock_record(acct_fd, F_SETLK, F_UNLCK, account_offset, sizeof(account));
        printf("Account record lock released cleanup.\n");
    }
    if (tran_file_locked)
    {
        unlock_file(tran_fd);
        printf("Transaction file lock released cleanup.\n");
    }
    if (acct_fd != -1)
        close(acct_fd);
    if (tran_fd != -1)
        close(tran_fd);
    if (write(client_socket_fd, &res, sizeof(res)) < 0)
    {
        perror("Server: write() response failed");
    }
}

void handle_transfer_request(int client_socket_fd, struct Request *req)
{
    printf("Processing TRANSFER from Customer %d to Account %d...\n", req->user_id, req->int_data_1);
    struct Response res;
    struct Account source_account, target_account;
    struct Transaction trans_debit, trans_credit;
    int source_account_id = -1, target_account_id = req->int_data_1;
    double amount = req->double_data_1;
    int acct_fd = -1, tran_fd = -1;
    off_t source_offset = -1, target_offset = -1;
    int source_locked = 0, target_locked = 0, tran_locked = 0;

    memset(&res, 0, sizeof(res));
    res.success = 1;
    source_account_id = get_account_id_from_user_id(req->user_id);
    if (source_account_id == -1)
    {
        res.success = 0;
        strcpy(res.message, "Error: Could not find your account ID.");
        goto cleanup_transfer;
    }
    if (source_account_id == target_account_id)
    {
        res.success = 0;
        strcpy(res.message, "Error: Cannot transfer to same account.");
        goto cleanup_transfer;
    }

    acct_fd = open(ACCOUNT_FILE, O_RDWR);
    if (acct_fd < 0)
    {
        perror("Server: Error opening account file");
        res.success = 0;
        strcpy(res.message, "Server error: Could not read account data.");
        goto cleanup_transfer;
    }

    if (!find_account_record(acct_fd, source_account_id, &source_account, &source_offset))
    {
        res.success = 0;
        strcpy(res.message, "Error: Source account data not found.");
        goto cleanup_transfer;
    }
    if (!find_account_record(acct_fd, target_account_id, &target_account, &target_offset))
    {
        res.success = 0;
        strcpy(res.message, "Error: Target account data not found.");
        goto cleanup_transfer;
    }

    if (source_account.is_active == 0)
    {
        res.success = 0;
        strcpy(res.message, "Transfer failed: Your account is deactivated.");
        goto cleanup_transfer;
    }
    if (target_account.is_active == 0)
    {
        res.success = 0;
        strcpy(res.message, "Transfer failed: The target account is deactivated.");
        goto cleanup_transfer;
    }

    printf("Locking source (%ld) and target (%ld)...\n", source_offset, target_offset);
    if (source_offset < target_offset)
    {
        if (lock_record(acct_fd, F_SETLKW, F_WRLCK, source_offset, sizeof(source_account)) == -1)
        {
            res.success = 0;
            goto lock_fail;
        }
        source_locked = 1;
        if (lock_record(acct_fd, F_SETLKW, F_WRLCK, target_offset, sizeof(target_account)) == -1)
        {
            res.success = 0;
            goto lock_fail;
        }
        target_locked = 1;
    }
    else
    {
        if (lock_record(acct_fd, F_SETLKW, F_WRLCK, target_offset, sizeof(target_account)) == -1)
        {
            res.success = 0;
            goto lock_fail;
        }
        target_locked = 1;
        if (lock_record(acct_fd, F_SETLKW, F_WRLCK, source_offset, sizeof(source_account)) == -1)
        {
            res.success = 0;
            goto lock_fail;
        }
        source_locked = 1;
    }
    printf("Both records locked.\n");

    if (source_account.balance < amount)
    {
        res.success = 0;
        sprintf(res.message, "Transfer failed: Insufficient funds. Balance: $%.2f", source_account.balance);
    }
    else
    {
        source_account.balance -= amount;
        target_account.balance += amount;
        lseek(acct_fd, source_offset, SEEK_SET);
        if (write(acct_fd, &source_account, sizeof(source_account)) <= 0)
        {
            perror("Server: Failed writing source");
            res.success = 0;
            goto write_fail;
        }
        lseek(acct_fd, target_offset, SEEK_SET);
        if (write(acct_fd, &target_account, sizeof(target_account)) <= 0)
        {
            perror("Server: Failed writing target");
            res.success = 0;
            goto write_fail;
        }
        res.success = 1;
        printf("Balances updated ok.\n");

        tran_fd = open(TRANSACTION_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (tran_fd < 0)
        {
            perror("Server: Error opening transaction file");
            strcpy(res.message, "Transfer ok, but failed log transactions.");
        }
        else
        {
            if (lock_file(tran_fd, F_WRLCK) == -1)
            {
                perror("Server: Failed lock transaction file");
                strcpy(res.message, "Transfer ok, but failed log transactions.");
            }
            else
            {
                tran_locked = 1;
                int tran_id_1 = get_next_id(tran_fd, sizeof(struct Transaction), offsetof(struct Transaction, transaction_id));
                int tran_id_2 = tran_id_1 + 1;
                trans_debit.transaction_id = tran_id_1;
                trans_debit.account_id = source_account_id;
                trans_debit.type = TRANSFER;
                trans_debit.amount = -amount;
                trans_debit.timestamp = time(NULL);
                trans_credit.transaction_id = tran_id_2;
                trans_credit.account_id = target_account_id;
                trans_credit.type = TRANSFER;
                trans_credit.amount = amount;
                trans_credit.timestamp = time(NULL);
                if (write(tran_fd, &trans_debit, sizeof(trans_debit)) <= 0 ||
                    write(tran_fd, &trans_credit, sizeof(trans_credit)) <= 0)
                {
                    perror("Server: Failed write transactions");
                    strcpy(res.message, "Transfer ok, but failed log transactions.");
                }
                else
                {
                    res.double_data_1 = source_account.balance;
                    sprintf(res.message, "Transfer successful.");
                }
                unlock_file(tran_fd);
                tran_locked = 0;
            }
            close(tran_fd);
            tran_fd = -1;
        }
    }

lock_fail:
write_fail:
    if (source_locked)
        lock_record(acct_fd, F_SETLK, F_UNLCK, source_offset, sizeof(source_account));
    if (target_locked)
        lock_record(acct_fd, F_SETLK, F_UNLCK, target_offset, sizeof(target_account));
    if (source_locked || target_locked)
        printf("Account records unlocked.\n");

cleanup_transfer:
    if (source_locked)
        lock_record(acct_fd, F_SETLK, F_UNLCK, source_offset, sizeof(source_account));
    if (target_locked)
        lock_record(acct_fd, F_SETLK, F_UNLCK, target_offset, sizeof(target_account));
    if (tran_locked)
        unlock_file(tran_fd);
    if (acct_fd != -1)
        close(acct_fd);
    if (tran_fd != -1)
        close(tran_fd);
    if (write(client_socket_fd, &res, sizeof(res)) < 0)
    {
        perror("Server: write() response failed");
    }
}

void handle_view_transactions_request(int client_socket_fd, struct Request *req)
{
    printf("Processing VIEW_TRANSACTIONS request for Customer ID %d...\n", req->user_id);
    struct Response res;
    struct Transaction transaction;
    int account_id = -1;
    int tran_fd = -1;
    int count = 0;
    memset(&res, 0, sizeof(res));

    account_id = get_account_id_from_user_id(req->user_id);
    if (account_id == -1)
    {
        res.success = 0;
        strcpy(res.message, "Error: Could not find account for user ID.");
        write(client_socket_fd, &res, sizeof(res));
        return;
    }
    tran_fd = open(TRANSACTION_FILE, O_RDONLY);
    if (tran_fd < 0)
    {
        perror("Server: Error opening transaction file");
        res.success = 0;
        strcpy(res.message, "Server error: Could not read history.");
        write(client_socket_fd, &res, sizeof(res));
        return;
    }
    if (lock_file(tran_fd, F_RDLCK) == -1)
    {
        perror("Server: Failed lock transaction file");
        res.success = 0;
        strcpy(res.message, "Server busy.");
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
        strcpy(res.message, "No transactions found.");
    }
    else if (count > 0 && read_bytes >= 0)
    {
        strcpy(res.message, "End of history.");
    }
    else
    {
        strcpy(res.message, "Error retrieving history.");
    }
    if (write(client_socket_fd, &res, sizeof(res)) < 0)
    {
        perror("Server: write() end-of-list failed");
    }
}

void handle_add_feedback_request(int client_socket_fd, struct Request *req)
{
    printf("Processing ADD_FEEDBACK request from Customer ID %d...\n", req->user_id);
    struct Response res;
    struct Feedback new_feedback;
    int fb_fd = -1;

    memset(&res, 0, sizeof(res));
    new_feedback.customer_id = req->user_id;
    new_feedback.timestamp = time(NULL);
    strncpy(new_feedback.message, req->char_data_1, MAX_FEEDBACK_LEN);
    new_feedback.message[MAX_FEEDBACK_LEN - 1] = '\0';

    fb_fd = open(FEEDBACK_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fb_fd < 0)
    {
        perror("Server: Error opening feedback file");
        res.success = 0;
        strcpy(res.message, "Server error: Could not save feedback.");
        goto cleanup_feedback;
    }
    if (lock_file(fb_fd, F_WRLCK) == -1)
    {
        perror("Server: Failed to lock feedback file");
        res.success = 0;
        strcpy(res.message, "Server busy. Please try again.");
        goto cleanup_feedback;
    }

    if (write(fb_fd, &new_feedback, sizeof(new_feedback)) <= 0)
    {
        perror("Server: Failed to write feedback");
        res.success = 0;
        strcpy(res.message, "Server error: Failed to save feedback.");
    }
    else
    {
        res.success = 1;
        strcpy(res.message, "Feedback submitted successfully. Thank you!");
        printf("Feedback received from Customer ID %d.\n", req->user_id);
    }
    unlock_file(fb_fd);

cleanup_feedback:
    if (fb_fd != -1)
        close(fb_fd);
    if (write(client_socket_fd, &res, sizeof(res)) < 0)
    {
        perror("Server: write() response failed");
    }
}

void handle_apply_for_loan_request(int client_socket_fd, struct Request *req)
{
    printf("Processing APPLY_LOAN request from Customer ID %d...\n", req->user_id);
    struct Response res;
    struct Loan new_loan;
    int loan_fd = -1;
    int loan_file_locked = 0;

    memset(&res, 0, sizeof(res));
    res.success = 1;
    loan_fd = open(LOAN_FILE, O_RDWR | O_CREAT | O_APPEND, 0644);
    if (loan_fd < 0)
    {
        perror("Server: Error opening loan file");
        res.success = 0;
        strcpy(res.message, "Server error: Could not access loan data.");
        goto cleanup_apply_loan;
    }
    if (lock_file(loan_fd, F_WRLCK) == -1)
    {
        perror("Server: Failed to lock loan file");
        res.success = 0;
        strcpy(res.message, "Server busy. Please try again.");
        goto cleanup_apply_loan;
    }
    loan_file_locked = 1;

    int new_loan_id = get_next_id(loan_fd, sizeof(struct Loan), offsetof(struct Loan, loan_id));
    new_loan.loan_id = new_loan_id;
    new_loan.customer_id = req->user_id;
    new_loan.amount = req->double_data_1;
    new_loan.status = 0;                   // 0 = Pending
    new_loan.assigned_to_employee_id = -1; // -1 = Unassigned

    if (write(loan_fd, &new_loan, sizeof(struct Loan)) <= 0)
    {
        perror("Server: Failed to write new loan record");
        res.success = 0;
        strcpy(res.message, "Server error: Failed to save loan application.");
    }
    else
    {
        res.success = 1;
        sprintf(res.message, "Loan application (ID: %d) for $%.2f submitted successfully.", new_loan_id, new_loan.amount);
        printf("Loan application %d submitted by Customer %d.\n", new_loan_id, req->user_id);
    }

cleanup_apply_loan:
    if (loan_file_locked)
        unlock_file(loan_fd);
    if (loan_fd != -1)
        close(loan_fd);
    if (write(client_socket_fd, &res, sizeof(res)) < 0)
    {
        perror("Server: write() response failed");
    }
}
