#include "server_handlers.h"

void handle_add_employee_request(int client_socket_fd, struct Request *req)
{
    printf("Processing ADD_EMPLOYEE request from Admin ID %d...\n", req->user_id);
    struct Response res;
    struct User new_user;
    struct Employee new_employee;
    int user_fd = -1, emp_fd = -1;
    int user_lock = 0, emp_lock = 0;
    memset(&res, 0, sizeof(res));
    res.success = 1;

    user_fd = open(USER_FILE, O_RDWR | O_CREAT, 0644);
    if (user_fd < 0)
    {
        perror("Server: Error opening user file");
        res.success = 0;
        strcpy(res.message, "Server error: Could not open user data.");
        goto cleanup_employee;
    }
    emp_fd = open(EMPLOYEE_FILE, O_RDWR | O_CREAT, 0644);
    if (emp_fd < 0)
    {
        perror("Server: Error opening employee file");
        res.success = 0;
        strcpy(res.message, "Server error: Could not open employee data.");
        goto cleanup_employee;
    }
    //Checking and creating the locks
    if (lock_file(user_fd, F_WRLCK) == -1)
    {
        perror("Server: Failed to get user file write lock");
        res.success = 0;
        strcpy(res.message, "Server busy. Please try again.");
        goto cleanup_employee;
    }
    user_lock = 1;
    if (lock_file(emp_fd, F_WRLCK) == -1)
    {
        perror("Server: Failed to get employee file write lock");
        res.success = 0;
        strcpy(res.message, "Server busy. Please try again.");
        goto cleanup_employee;
    }
    emp_lock = 1;

    printf("File locks acquired for ADD_EMPLOYEE.\n");
    int max_user_id = get_next_id(user_fd, sizeof(struct User), offsetof(struct User, user_id)) - 1;
    int max_emp_id = get_next_id(emp_fd, sizeof(struct Employee), offsetof(struct Employee, employee_id)) - 1;
    int new_id = (max_user_id > max_emp_id ? max_user_id : max_emp_id) + 1;

    new_employee.employee_id = new_id;
    strncpy(new_employee.name, req->char_data_1, MAX_NAME_LEN);
    new_user.user_id = new_id;
    new_user.customer_or_employee_id = new_id;
    new_user.role = req->user_role;
    strncpy(new_user.username, req->username, MAX_USERNAME_LEN);
    strncpy(new_user.password, req->password, MAX_PASSWORD_LEN);

    lseek(emp_fd, 0, SEEK_END);
    if (write(emp_fd, &new_employee, sizeof(new_employee)) <= 0)
    {
        perror("Server: Failed to write new employee");
        res.success = 0;
        strcpy(res.message, "Server error: Failed to save employee data.");
    }
    else
    {
        lseek(user_fd, 0, SEEK_END);
        if (write(user_fd, &new_user, sizeof(new_user)) <= 0)
        {
            perror("Server: Failed to write new user");
            res.success = 0;
            strcpy(res.message, "Server error: Failed to save user login.");
        }
        else
        {
            res.success = 1;
            sprintf(res.message, "Success! User '%s' added with ID %d.", new_user.username, new_user.user_id);
        }
    }

cleanup_employee:
    if (user_lock)
        unlock_file(user_fd);
    if (emp_lock)
        unlock_file(emp_fd);
    if (user_lock || emp_lock)
        printf("File locks released.\n");
    if (user_fd != -1)
        close(user_fd);
    if (emp_fd != -1)
        close(emp_fd);
    if (write(client_socket_fd, &res, sizeof(res)) < 0)
    {
        perror("Server: write() response failed");
    }
}

void handle_modify_user_details_request(int client_socket_fd, struct Request *req)
{
    printf("Processing MODIFY_USER_DETAILS by Admin %d for User %d...\n", req->user_id, req->int_data_1);
    struct Response res;
    int target_user_id = req->int_data_1;
    UserRole target_role = req->user_role;
    char *new_name = req->char_data_1;
    int fd = -1;
    off_t record_offset = -1;
    int rec_locked = 0;
    size_t record_size = 0;

    memset(&res, 0, sizeof(res));
    res.success = 1;

    if (target_role == CUSTOMER)
    {
        struct Customer customer_record;
        record_size = sizeof(struct Customer);
        fd = open(CUSTOMER_FILE, O_RDWR);
        if (fd < 0)
        {
            perror("Server: Error opening customer file");
            res.success = 0;
            strcpy(res.message, "Server error: Could not access customer data.");
            goto cleanup_modify_user;
        }
        if (!find_customer_record(fd, target_user_id, &customer_record, &record_offset))
        {
            res.success = 0;
            strcpy(res.message, "Error: Customer record not found.");
            goto cleanup_modify_user;
        }
        if (lock_record(fd, F_SETLKW, F_WRLCK, record_offset, record_size) == -1)
        {
            perror("Server: Failed lock customer record");
            res.success = 0;
            strcpy(res.message, "Server busy (customer record in use).");
            goto cleanup_modify_user;
        }
        rec_locked = 1;
        strncpy(customer_record.name, new_name, MAX_NAME_LEN);
        lseek(fd, record_offset, SEEK_SET);
        if (write(fd, &customer_record, record_size) <= 0)
        {
            perror("Server: Failed write updated customer record");
            res.success = 0;
            strcpy(res.message, "Server error: Failed save details.");
        }
        else
        {
            res.success = 1;
            sprintf(res.message, "Customer ID %d name updated.", target_user_id);
        }
    }
    else if (target_role == EMPLOYEE || target_role == MANAGER)
    {
        struct Employee employee_record;
        record_size = sizeof(struct Employee);
        fd = open(EMPLOYEE_FILE, O_RDWR);
        if (fd < 0)
        {
            perror("Server: Error opening employee file");
            res.success = 0;
            strcpy(res.message, "Server error: Could not access employee data.");
            goto cleanup_modify_user;
        }
        if (!find_employee_record(fd, target_user_id, &employee_record, &record_offset))
        {
            res.success = 0;
            strcpy(res.message, "Error: Employee/Manager record not found.");
            goto cleanup_modify_user;
        }
        if (lock_record(fd, F_SETLKW, F_WRLCK, record_offset, record_size) == -1)
        {
            perror("Server: Failed lock employee record");
            res.success = 0;
            strcpy(res.message, "Server busy (employee record in use).");
            goto cleanup_modify_user;
        }
        rec_locked = 1;
        strncpy(employee_record.name, new_name, MAX_NAME_LEN);
        lseek(fd, record_offset, SEEK_SET);
        if (write(fd, &employee_record, record_size) <= 0)
        {
            perror("Server: Failed write updated employee record");
            res.success = 0;
            strcpy(res.message, "Server error: Failed save details.");
        }
        else
        {
            res.success = 1;
            sprintf(res.message, "Employee/Manager ID %d name updated.", target_user_id);
        }
    }
    else
    {
        res.success = 0;
        strcpy(res.message, "Error: Invalid user role specified.");
    }

cleanup_modify_user:
    if (rec_locked)
    {
        lock_record(fd, F_SETLK, F_UNLCK, record_offset, record_size);
        printf("User record lock released.\n");
    }
    if (fd != -1)
        close(fd);
    if (write(client_socket_fd, &res, sizeof(res)) < 0)
    {
        perror("Server: write() response failed");
    }
}

void handle_manage_roles_request(int client_socket_fd, struct Request *req)
{
    printf("Processing MANAGE_ROLES request from Admin ID %d for User ID %d...\n", req->user_id, req->int_data_1);
    struct Response res;
    struct User user_record;
    int user_fd = -1;
    off_t user_offset = -1;
    int user_rec_locked = 0;
    int target_user_id = req->int_data_1;
    UserRole new_role = (UserRole)req->int_data_2;

    memset(&res, 0, sizeof(res));
    res.success = 1;

    if (new_role < CUSTOMER || new_role > MANAGER)
    {
        res.success = 0;
        strcpy(res.message, "Error: Invalid target role. Can only set to Customer, Employee, or Manager.");
        goto cleanup_manage_roles;
    }
    if (target_user_id == 1)
    {
        res.success = 0;
        strcpy(res.message, "Error: Cannot modify the primary administrator account.");
        goto cleanup_manage_roles;
    }

    user_fd = open(USER_FILE, O_RDWR);
    if (user_fd < 0)
    {
        perror("Server: Error opening user file");
        res.success = 0;
        strcpy(res.message, "Server error: Could not access user data.");
        goto cleanup_manage_roles;
    }
    if (!find_user_record(user_fd, target_user_id, &user_record, &user_offset))
    {
        res.success = 0;
        strcpy(res.message, "Error: User record not found.");
        goto cleanup_manage_roles;
    }
    if (lock_record(user_fd, F_SETLKW, F_WRLCK, user_offset, sizeof(struct User)) == -1)
    {
        perror("Server: Failed to lock user record");
        res.success = 0;
        strcpy(res.message, "Server busy (user record in use).");
        goto cleanup_manage_roles;
    }
    user_rec_locked = 1;
    printf("User record lock acquired.\n");

    if (user_record.role == new_role)
    {
        res.success = 0;
        sprintf(res.message, "User ID %d already has that role.", target_user_id);
    }
    else
    {
        user_record.role = new_role;
        lseek(user_fd, user_offset, SEEK_SET);
        if (write(user_fd, &user_record, sizeof(struct User)) <= 0)
        {
            perror("Server: Failed to write updated user record");
            res.success = 0;
            strcpy(res.message, "Server error: Failed to save new role.");
        }
        else
        {
            res.success = 1;
            sprintf(res.message, "User ID %d role updated successfully.", target_user_id);
            printf("User ID %d role changed to %d by Admin %d.\n", target_user_id, new_role, req->user_id);
        }
    }

    if (user_rec_locked)
    {
        lock_record(user_fd, F_SETLK, F_UNLCK, user_offset, sizeof(struct User));
        printf("User record lock released.\n");
    }

cleanup_manage_roles:
    if (user_rec_locked)
    {
        lock_record(user_fd, F_SETLK, F_UNLCK, user_offset, sizeof(struct User));
    }
    if (user_fd != -1)
        close(user_fd);
    if (write(client_socket_fd, &res, sizeof(res)) < 0)
    {
        perror("Server: write() response failed");
    }
}
