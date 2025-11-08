#include "server_handlers.h"

// Helper Functions

int get_account_id_from_user_id(int user_id)
{
    int cust_fd = -1;
    struct Customer customer;
    int account_id = -1;
    cust_fd = open(CUSTOMER_FILE, O_RDONLY);
    if (cust_fd < 0)
    {
        perror("Server: Error opening customer file");
        return -1;
    }
    if (lock_file(cust_fd, F_RDLCK) == -1)
    {
        perror("Server: Failed lock customer file");
        close(cust_fd);
        return -1;
    }
    ssize_t read_bytes;
    while ((read_bytes = read(cust_fd, &customer, sizeof(customer))) == sizeof(customer))
    {
        if (customer.customer_id == user_id)
        {
            account_id = customer.account_id;
            break;
        }
    }
    if (read_bytes < 0)
    {
        perror("Server: Error reading customer file");
    }
    unlock_file(cust_fd);
    close(cust_fd);
    return account_id;
}

int find_account_record(int fd, int account_id, struct Account *acc_out, off_t *offset_out)
{
    struct Account current_account;
    ssize_t read_bytes;
    off_t current_offset = 0;
    lseek(fd, 0, SEEK_SET);
    while ((read_bytes = read(fd, &current_account, sizeof(current_account))) == sizeof(current_account))
    {
        if (current_account.account_id == account_id)
        {
            *acc_out = current_account;
            *offset_out = current_offset;
            return 1;
        }
        current_offset += sizeof(current_account);
    }
    if (read_bytes < 0)
    {
        perror("Server: Error reading account file during find");
    }
    return 0;
}

int find_customer_record(int fd, int customer_id, struct Customer *cust_out, off_t *offset_out)
{
    struct Customer current_customer;
    ssize_t read_bytes;
    off_t current_offset = 0;
    lseek(fd, 0, SEEK_SET);
    while ((read_bytes = read(fd, &current_customer, sizeof(current_customer))) == sizeof(current_customer))
    {
        if (current_customer.customer_id == customer_id)
        {
            *cust_out = current_customer;
            *offset_out = current_offset;
            return 1;
        }
        current_offset += sizeof(current_customer);
    }
    if (read_bytes < 0)
    {
        perror("Server: Error reading customer file during find");
    }
    return 0;
}

int find_employee_record(int fd, int employee_id, struct Employee *emp_out, off_t *offset_out)
{
    struct Employee current_employee;
    ssize_t read_bytes;
    off_t current_offset = 0;
    lseek(fd, 0, SEEK_SET);
    while ((read_bytes = read(fd, &current_employee, sizeof(current_employee))) == sizeof(current_employee))
    {
        if (current_employee.employee_id == employee_id)
        {
            *emp_out = current_employee;
            *offset_out = current_offset;
            return 1;
        }
        current_offset += sizeof(current_employee);
    }
    if (read_bytes < 0)
    {
        perror("Server: Error reading employee file during find");
    }
    return 0;
}

int find_loan_record(int fd, int loan_id, struct Loan *loan_out, off_t *offset_out)
{
    struct Loan current_loan;
    ssize_t read_bytes;
    off_t current_offset = 0;
    lseek(fd, 0, SEEK_SET);
    while ((read_bytes = read(fd, &current_loan, sizeof(struct Loan))) == sizeof(struct Loan))
    {
        if (current_loan.loan_id == loan_id)
        {
            *loan_out = current_loan;
            *offset_out = current_offset;
            return 1;
        }
        current_offset += sizeof(struct Loan);
    }
    if (read_bytes < 0)
    {
        perror("Server: Error reading loan file during find");
    }
    return 0;
}

int find_user_record(int fd, int user_id, struct User *user_out, off_t *offset_out)
{
    struct User current_user;
    ssize_t read_bytes;
    off_t current_offset = 0;
    lseek(fd, 0, SEEK_SET);
    while ((read_bytes = read(fd, &current_user, sizeof(struct User))) == sizeof(struct User))
    {
        if (current_user.user_id == user_id)
        {
            *user_out = current_user;
            *offset_out = current_offset;
            return 1; // Found
        }
        current_offset += sizeof(struct User);
    }
    if (read_bytes < 0)
    {
        perror("Server: Error reading user file during find");
    }
    return 0; // Not found
}

int get_next_id(int fd, int struct_size, int id_offset)
{
    char buffer[struct_size];
    int max_id = 0;
    lseek(fd, 0, SEEK_SET);
    ssize_t read_bytes;
    while ((read_bytes = read(fd, buffer, struct_size)) == struct_size)
    {
        int current_id = *(int *)(buffer + id_offset);
        if (current_id > max_id)
            max_id = current_id;
    }
    if (read_bytes < 0)
    {
        perror("Server: Error reading file to get next ID");
    }
    return max_id + 1;
}

int lock_file(int fd, int lock_type)
{
    struct flock lock;
    lock.l_type = lock_type;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();
    return fcntl(fd, F_SETLKW, &lock);
}

int unlock_file(int fd)
{
    struct flock lock;
    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();
    return fcntl(fd, F_SETLK, &lock);
}

int lock_record(int fd, int cmd, int type, off_t offset, off_t len)
{
    struct flock lock;
    lock.l_type = type;
    lock.l_whence = SEEK_SET;
    lock.l_start = offset;
    lock.l_len = len;
    lock.l_pid = getpid();
    return fcntl(fd, cmd, &lock);
}

const char *get_transaction_type_string(OperationCode type)
{
    switch (type)
    {
    case DEPOSIT:
        return "DEPOSIT ";
    case WITHDRAW:
        return "WITHDRAW";
    case TRANSFER:
        return "TRANSFER";
    default:
        return "UNKNOWN ";
    }
}
