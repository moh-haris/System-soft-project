#include "bank_system.h"

// Prototype for Process Loan 
void handle_process_loan(int socket_fd, int employee_id);
// ----------------------------------------

// Prototypes
void handle_view_assigned_loans(int socket_fd, int employee_id);
void handle_apply_for_loan(int socket_fd, int customer_id);
void handle_view_customer_txn(int socket_fd, int employee_id);
void handle_modify_user_details(int socket_fd, int admin_id);
void handle_review_feedback(int socket_fd, int manager_id);
void handle_add_feedback(int socket_fd, int customer_id);
void handle_modify_customer(int socket_fd, int employee_id);
void handle_change_password(int socket_fd, int user_id);
void handle_view_transactions(int socket_fd, int customer_id);
void handle_transfer(int socket_fd, int customer_id);
void customer_menu(int socket_fd, int customer_id);
void handle_view_balance(int socket_fd, int customer_id);
void handle_deposit(int socket_fd, int customer_id);
void handle_withdraw(int socket_fd, int customer_id);
void employee_menu(int socket_fd, int employee_id);
void handle_add_customer(int socket_fd, int employee_id);
void admin_menu(int socket_fd, int admin_id);
void handle_add_employee(int socket_fd, int admin_id);
void manager_menu(int socket_fd, int manager_id);
void handle_toggle_account_status(int socket_fd, int manager_id);
void handle_assign_loan(int socket_fd, int manager_id);
int connect_to_server();
void handle_login(int socket_fd);
void get_input(char *buffer, int size);

int main()
{ 
    int socket_fd = connect_to_server();
    if (socket_fd == -1)
    {
        perror("Client: Could not connect to server");
        exit(EXIT_FAILURE);
    }
    printf("Successfully connected to the bank server!\n");
    handle_login(socket_fd);
    close(socket_fd);
    printf("Disconnected from server.\n");
    return 0;
}
int connect_to_server()
{ 
    int socket_fd;
    struct sockaddr_in server_address;
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        perror("Client: socket() failed");
        return -1;
    }
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0)
    {
        perror("Client: Invalid address");
        close(socket_fd);
        return -1;
    }
    if (connect(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("Client: connect() failed");
        close(socket_fd);
        return -1;
    }
    return socket_fd;
}
void handle_login(int socket_fd)
{
    struct Request req;
    struct Response res;
    int user_choice;
    memset(&req, 0, sizeof(req));
    req.op_code = LOGIN;
    printf("\n Welcome to the Bank  \n");
    printf("1. Customer Login\n2. Employee Login\n3. Manager Login\n4. Admin Login\n");
    printf("Enter your choice: ");
    while (scanf("%d", &user_choice) != 1 || user_choice < 1 || user_choice > 4)
    {
        printf("Invalid input. (1-4): ");
        while (getchar() != '\n')
            ;
    }
    req.user_role = (UserRole)user_choice;
    while (getchar() != '\n')
        ;
    printf("Enter Username: ");
    get_input(req.username, MAX_USERNAME_LEN);
    printf("Enter Password: ");
    get_input(req.password, MAX_PASSWORD_LEN);
    if (write(socket_fd, &req, sizeof(req)) < 0)
    {
        perror("Client: write() failed");
        return;
    }
    if (read(socket_fd, &res, sizeof(res)) < 0)
    {
        perror("Client: read() failed");
        return;
    }
    printf("\n[Server Response] %s\n", res.message);
    if (res.success)
    {
        printf("Login Successful!\n");
        switch (req.user_role)
        {
        case CUSTOMER:
            customer_menu(socket_fd, res.user_id);
            break;
        case EMPLOYEE:
            employee_menu(socket_fd, res.user_id);
            break;
        case MANAGER:
            manager_menu(socket_fd, res.user_id);
            break;
        case ADMIN:
            admin_menu(socket_fd, res.user_id);
            break;
        default:
            printf("No menu available for this role.\n");
        }
    }
    else
    {
        printf("Login Failed.\n");
    }
}

//  Admin 
void admin_menu(int socket_fd, int admin_id)
{ 
    int choice = 0;
    while (1)
    {
        printf("\n Admin Menu \n");
        printf("1. Add New Bank Employee/Manager\n");
        printf("2. Change Password\n");
        printf("3. Modify User Details (Customer/Employee)\n");
        printf("10. Logout\n");
        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1)
            choice = 0;
        while (getchar() != '\n')
            ;
        switch (choice)
        {
        case 1:
            handle_add_employee(socket_fd, admin_id);
            break;
        case 2:
            handle_change_password(socket_fd, admin_id);
            break;
        case 3:
            handle_modify_user_details(socket_fd, admin_id);
            break;
        case 10:
            printf("Logging out\n");
            return;
        default:
            printf("Invalid choice. Please try again.\n");
        }
    }
}
void handle_add_employee(int socket_fd, int admin_id)
{ 
    struct Request req;
    struct Response res;
    int role_choice;
    memset(&req, 0, sizeof(req));
    req.op_code = ADD_EMPLOYEE;
    req.user_id = admin_id;
    printf("\n Add New Employee/Manager \n");
    printf("Enter new user's full name: ");
    get_input(req.char_data_1, sizeof(req.char_data_1));
    printf("Enter new user's username (for login): ");
    get_input(req.username, MAX_USERNAME_LEN);
    printf("Enter new user's password: ");
    get_input(req.password, MAX_PASSWORD_LEN);
    printf("Enter role (2 for EMPLOYEE, 3 for MANAGER): ");
    while (scanf("%d", &role_choice) != 1 || (role_choice != EMPLOYEE && role_choice != MANAGER))
    {
        printf("Invalid role. (2 or 3): ");
        while (getchar() != '\n')
            ;
    }
    while (getchar() != '\n')
        ;
    req.user_role = (UserRole)role_choice;
    if (write(socket_fd, &req, sizeof(req)) < 0)
    {
        perror("Client: write() failed");
        return;
    }
    if (read(socket_fd, &res, sizeof(res)) < 0)
    {
        perror("Client: read() failed");
        return;
    }
    printf("\n[Server Response] %s\n", res.message);
}
void handle_modify_user_details(int socket_fd, int admin_id)
{ 
    struct Request req;
    struct Response res;
    int role_choice = 0;
    int target_user_id = 0;
    memset(&req, 0, sizeof(req));
    req.op_code = MODIFY_USER_DETAILS;
    req.user_id = admin_id;
    printf("\n Modify User Details \n");
    printf("Which type of user?\n1. Customer\n2. Employee\n3. Manager\n");
    printf("Enter choice (1-3): ");
    while (scanf("%d", &role_choice) != 1 || role_choice < 1 || role_choice > 3)
    {
        printf("Invalid choice. (1-3): ");
        while (getchar() != '\n')
            ;
    }
    req.user_role = (UserRole)role_choice;
    printf("Enter the User ID of the user to modify: ");
    while (scanf("%d", &target_user_id) != 1 || target_user_id <= 0)
    {
        printf("Invalid ID: ");
        while (getchar() != '\n')
            ;
    }
    req.int_data_1 = target_user_id;
    while (getchar() != '\n')
        ;
    printf("Enter the user's NEW full name: ");
    get_input(req.char_data_1, sizeof(req.char_data_1));
    if (write(socket_fd, &req, sizeof(req)) < 0)
    {
        perror("Client: write() failed");
        return;
    }
    if (read(socket_fd, &res, sizeof(res)) < 0)
    {
        perror("Client: read() failed");
        return;
    }
    printf("\n[Server Response] %s\n", res.message);
}

// Employee Menu (MODIFIED)
void employee_menu(int socket_fd, int employee_id)
{
    int choice = 0;
    while (1)
    {
        printf("\n Employee Menu \n");
        printf("1. Add New Customer\n");
        printf("2. Modify Customer Details\n");
        printf("3. Change Password\n");
        printf("4. View Customer Transactions\n");
        printf("5. View Assigned Loans\n");
        printf("6. Process Loan Application\n"); //  NEW
        printf("10. Logout\n");
        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1)
            choice = 0;
        while (getchar() != '\n')
            ;
        switch (choice)
        {
        case 1:
            handle_add_customer(socket_fd, employee_id);
            break;
        case 2:
            handle_modify_customer(socket_fd, employee_id);
            break;
        case 3:
            handle_change_password(socket_fd, employee_id);
            break;
        case 4:
            handle_view_customer_txn(socket_fd, employee_id);
            break;
        case 5:
            handle_view_assigned_loans(socket_fd, employee_id);
            break;
        case 6:
            handle_process_loan(socket_fd, employee_id);
            break; //  NEW
        case 10:
            printf("Logging out\n");
            return;
        default:
            printf("Invalid choice. Please try again.\n");
        }
    }
}
void handle_add_customer(int socket_fd, int employee_id)
{ 
    struct Request req;
    struct Response res;
    double initial_deposit = 0;
    memset(&req, 0, sizeof(req));
    req.op_code = ADD_CUSTOMER;
    req.user_id = employee_id;
    printf("\n Add New Customer \n");
    printf("Enter new customer's full name: ");
    get_input(req.char_data_1, sizeof(req.char_data_1));
    printf("Enter new customer's username (for login): ");
    get_input(req.username, MAX_USERNAME_LEN);
    printf("Enter new customer's password: ");
    get_input(req.password, MAX_PASSWORD_LEN);
    printf("Enter initial deposit amount: $");
    while (scanf("%lf", &initial_deposit) != 1 || initial_deposit < 0)
    {
        printf("Invalid amount: ");
        while (getchar() != '\n')
            ;
    }
    while (getchar() != '\n')
        ;
    req.double_data_1 = initial_deposit;
    req.user_role = CUSTOMER;
    if (write(socket_fd, &req, sizeof(req)) < 0)
    {
        perror("Client: write() failed");
        return;
    }
    if (read(socket_fd, &res, sizeof(res)) < 0)
    {
        perror("Client: read() failed");
        return;
    }
    printf("\n[Server Response] %s\n", res.message);
}
void handle_modify_customer(int socket_fd, int employee_id)
{ 
    struct Request req;
    struct Response res;
    int customer_id_to_modify = 0;
    memset(&req, 0, sizeof(req));
    req.op_code = MODIFY_CUSTOMER;
    req.user_id = employee_id;
    printf("\n Modify Customer Details \n");
    printf("Enter the Customer ID (User ID) to modify: ");
    while (scanf("%d", &customer_id_to_modify) != 1 || customer_id_to_modify <= 0)
    {
        printf("Invalid ID: ");
        while (getchar() != '\n')
            ;
    }
    req.int_data_1 = customer_id_to_modify;
    while (getchar() != '\n')
        ;
    printf("Enter the customer's NEW full name: ");
    get_input(req.char_data_1, sizeof(req.char_data_1));
    if (write(socket_fd, &req, sizeof(req)) < 0)
    {
        perror("Client: write() failed");
        return;
    }
    if (read(socket_fd, &res, sizeof(res)) < 0)
    {
        perror("Client: read() failed");
        return;
    }
    printf("\n[Server Response] %s\n", res.message);
}
void handle_view_customer_txn(int socket_fd, int employee_id)
{ 
    struct Request req;
    struct Response res;
    int target_customer_id = 0;
    memset(&req, 0, sizeof(req));
    req.op_code = VIEW_CUSTOMER_TXN;
    req.user_id = employee_id;
    printf("\nEnter the Customer ID to view transactions for: ");
    while (scanf("%d", &target_customer_id) != 1 || target_customer_id <= 0)
    {
        printf("Invalid ID: ");
        while (getchar() != '\n')
            ;
    }
    req.int_data_1 = target_customer_id;
    while (getchar() != '\n')
        ;
    if (write(socket_fd, &req, sizeof(req)) < 0)
    {
        perror("Client: write() failed");
        return;
    }
    printf("\n Transaction History for Customer %d \n", target_customer_id);
    int count = 0;
    while (1)
    {
        if (read(socket_fd, &res, sizeof(res)) <= 0)
        {
            perror("Client: read() failed");
            return;
        }
        if (res.success == 1)
        {
            count++;
            printf("%d. %s\n", count, res.message);
        }
        else
        {
            if (count == 0)
                printf("%s\n", res.message);
            break;
        }
    }
    printf(" End of History \n");
}
void handle_view_assigned_loans(int socket_fd, int employee_id)
{ 
    struct Request req;
    struct Response res;
    memset(&req, 0, sizeof(req));
    req.op_code = VIEW_ASSIGNED_LOANS;
    req.user_id = employee_id;
    if (write(socket_fd, &req, sizeof(req)) < 0)
    {
        perror("Client: write() failed");
        return;
    }
    printf("\n Your Assigned Loans (Pending) \n");
    int count = 0;
    while (1)
    {
        if (read(socket_fd, &res, sizeof(res)) <= 0)
        {
            perror("Client: read() failed");
            return;
        }
        if (res.success == 1)
        {
            count++;
            printf("%s\n", res.message);
        }
        else
        {
            if (count == 0)
                printf("%s\n", res.message);
            break;
        }
    }
    printf(" End of List \n");
}

//  NEW: Employee Process Loan 
void handle_process_loan(int socket_fd, int employee_id)
{
    struct Request req;
    struct Response res;
    int loan_id_to_process = 0;
    int action = -1; // 1 for approve, 2 for reject

    // 1. First, show the employee their assigned loans
    handle_view_assigned_loans(socket_fd, employee_id);

    // 2. Ask which one to process
    printf("\nEnter Loan ID to process (or 0 to cancel): ");
    while (scanf("%d", &loan_id_to_process) != 1 || loan_id_to_process < 0)
    {
        printf("Invalid ID: ");
        while (getchar() != '\n')
            ;
    }
    if (loan_id_to_process == 0)
    {
        return; // Canceled
    }

    printf("Enter action (1 to APPROVE, 2 to REJECT): ");
    while (scanf("%d", &action) != 1 || (action != 1 && action != 2))
    {
        printf("Invalid action. Please enter 1 or 2: ");
        while (getchar() != '\n')
            ;
    }
    while (getchar() != '\n')
        ; // Clear newline

    // 3. Send the PROCESS_LOAN request
    memset(&req, 0, sizeof(struct Request));
    req.op_code = PROCESS_LOAN;
    req.user_id = employee_id;           // Employee processing
    req.int_data_1 = loan_id_to_process; // Loan ID
    req.int_data_2 = action;             // Action (1=approve, 2=reject)

    if (write(socket_fd, &req, sizeof(struct Request)) < 0)
    {
        perror("Client: write() failed");
        return;
    }
    if (read(socket_fd, &res, sizeof(struct Response)) < 0)
    {
        perror("Client: read() failed");
        return;
    }

    printf("\n[Server Response] %s\n", res.message);
}

//  Manager 
void manager_menu(int socket_fd, int manager_id)
{ 
    int choice = 0;
    while (1)
    {
        printf("\n Manager Menu \n");
        printf("1. Activate/Deactivate Customer Account\n");
        printf("2. Review Customer Feedback\n");
        printf("3. Change Password\n");
        printf("4. Assign Loan Application\n");
        printf("10. Logout\n");
        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1)
            choice = 0;
        while (getchar() != '\n')
            ;
        switch (choice)
        {
        case 1:
            handle_toggle_account_status(socket_fd, manager_id);
            break;
        case 2:
            handle_review_feedback(socket_fd, manager_id);
            break;
        case 3:
            handle_change_password(socket_fd, manager_id);
            break;
        case 4:
            handle_assign_loan(socket_fd, manager_id);
            break;
        case 10:
            printf("Logging out\n");
            return;
        default:
            printf("Invalid choice. Please try again.\n");
        }
    }
}
void handle_toggle_account_status(int socket_fd, int manager_id)
{ 
    struct Request req;
    struct Response res;
    int target_account_id = 0;
    int action = -1;
    memset(&req, 0, sizeof(req));
    req.op_code = TOGGLE_ACCOUNT_STATUS;
    req.user_id = manager_id;
    printf("\n Activate/Deactivate Account \n");
    printf("Enter the Account ID to modify: ");
    while (scanf("%d", &target_account_id) != 1 || target_account_id <= 0)
    {
        printf("Invalid Account ID: ");
        while (getchar() != '\n')
            ;
    }
    req.int_data_1 = target_account_id;
    printf("Enter action (0 to Deactivate, 1 to Activate): ");
    while (scanf("%d", &action) != 1 || (action != 0 && action != 1))
    {
        printf("Invalid action (0 or 1): ");
        while (getchar() != '\n')
            ;
    }
    req.double_data_1 = (double)action;
    while (getchar() != '\n')
        ;
    if (write(socket_fd, &req, sizeof(req)) < 0)
    {
        perror("Client: write() failed");
        return;
    }
    if (read(socket_fd, &res, sizeof(res)) < 0)
    {
        perror("Client: read() failed");
        return;
    }
    printf("\n[Server Response] %s\n", res.message);
}
void handle_review_feedback(int socket_fd, int manager_id)
{ 
    struct Request req;
    struct Response res;
    memset(&req, 0, sizeof(req));
    req.op_code = VIEW_FEEDBACK;
    req.user_id = manager_id;
    if (write(socket_fd, &req, sizeof(req)) < 0)
    {
        perror("Client: write() failed");
        return;
    }
    printf("\n Customer Feedback \n");
    int count = 0;
    while (1)
    {
        if (read(socket_fd, &res, sizeof(res)) <= 0)
        {
            perror("Client: read() failed");
            return;
        }
        if (res.success == 1)
        {
            count++;
            printf(" Feedback %d \n%s\n", count, res.message);
        }
        else
        {
            if (count == 0)
                printf("%s\n", res.message);
            break;
        }
    }
    printf(" End of Feedback \n");
}
void handle_assign_loan(int socket_fd, int manager_id)
{ 
    struct Request req;
    struct Response res;
    int loan_id_to_assign = 0;
    int employee_id_to_assign = 0;
    memset(&req, 0, sizeof(req));
    req.op_code = VIEW_UNASSIGNED_LOANS;
    req.user_id = manager_id;
    if (write(socket_fd, &req, sizeof(req)) < 0)
    {
        perror("Client: write() failed");
        return;
    }
    printf("\n Unassigned Loan Applications \n");
    int count = 0;
    while (1)
    {
        if (read(socket_fd, &res, sizeof(res)) <= 0)
        {
            perror("Client: read() failed");
            return;
        }
        if (res.success == 1)
        {
            count++;
            printf("%s\n", res.message);
        }
        else
        {
            if (count == 0)
            {
                printf("%s\n", res.message);
                return;
            }
            break;
        }
    }
    printf(" End of List \n");
    printf("\nEnter Loan ID to assign: ");
    while (scanf("%d", &loan_id_to_assign) != 1 || loan_id_to_assign <= 0)
    {
        printf("Invalid ID: ");
        while (getchar() != '\n')
            ;
    }
    printf("Enter Employee ID to assign to: ");
    while (scanf("%d", &employee_id_to_assign) != 1 || employee_id_to_assign <= 0)
    {
        printf("Invalid ID: ");
        while (getchar() != '\n')
            ;
    }
    while (getchar() != '\n')
        ;
    memset(&req, 0, sizeof(req));
    req.op_code = ASSIGN_LOAN;
    req.user_id = manager_id;
    req.int_data_1 = loan_id_to_assign;
    req.int_data_2 = employee_id_to_assign;
    if (write(socket_fd, &req, sizeof(req)) < 0)
    {
        perror("Client: write() failed");
        return;
    }
    if (read(socket_fd, &res, sizeof(res)) < 0)
    {
        perror("Client: read() failed");
        return;
    }
    printf("\n[Server Response] %s\n", res.message);
}

//  Customer 
void customer_menu(int socket_fd, int customer_id)
{ 
    int choice = 0;
    while (1)
    {
        printf("\n Customer Menu \n");
        printf("1. View Account Balance\n");
        printf("2. Deposit Money\n");
        printf("3. Withdraw Money\n");
        printf("4. Transfer Funds\n");
        printf("5. View Transaction History\n");
        printf("6. Change Password\n");
        printf("7. Add Feedback\n");
        printf("8. Apply for a Loan\n");
        printf("10. Logout\n");
        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1)
            choice = 0;
        while (getchar() != '\n')
            ;
        switch (choice)
        {
        case 1:
            handle_view_balance(socket_fd, customer_id);
            break;
        case 2:
            handle_deposit(socket_fd, customer_id);
            break;
        case 3:
            handle_withdraw(socket_fd, customer_id);
            break;
        case 4:
            handle_transfer(socket_fd, customer_id);
            break;
        case 5:
            handle_view_transactions(socket_fd, customer_id);
            break;
        case 6:
            handle_change_password(socket_fd, customer_id);
            break;
        case 7:
            handle_add_feedback(socket_fd, customer_id);
            break;
        case 8:
            handle_apply_for_loan(socket_fd, customer_id);
            break;
        case 10:
            printf("Logging out\n");
            return;
        default:
            printf("Invalid choice. Please try again.\n");
        }
    }
}
void handle_view_balance(int socket_fd, int customer_id)
{ /* uses res.double_data_1  */
    struct Request req;
    struct Response res;
    memset(&req, 0, sizeof(req));
    req.op_code = VIEW_BALANCE;
    req.user_id = customer_id;
    if (write(socket_fd, &req, sizeof(req)) < 0)
    {
        perror("Client: write() failed");
        return;
    }
    if (read(socket_fd, &res, sizeof(res)) < 0)
    {
        perror("Client: read() failed");
        return;
    }
    if (res.success)
        printf("\n[Server Response] Your current balance is: $%.2f\n", res.double_data_1);
    else
        printf("\n[Server Response] %s\n", res.message);
}
void handle_deposit(int socket_fd, int customer_id)
{ /* uses res.double_data_1  */
    struct Request req;
    struct Response res;
    double amount = 0;
    memset(&req, 0, sizeof(req));
    req.op_code = DEPOSIT;
    req.user_id = customer_id;
    printf("\nEnter amount to deposit: $");
    while (scanf("%lf", &amount) != 1 || amount <= 0)
    {
        printf("Invalid amount: ");
        while (getchar() != '\n')
            ;
    }
    while (getchar() != '\n')
        ;
    req.double_data_1 = amount;
    if (write(socket_fd, &req, sizeof(req)) < 0)
    {
        perror("Client: write() failed");
        return;
    }
    if (read(socket_fd, &res, sizeof(res)) < 0)
    {
        perror("Client: read() failed");
        return;
    }
    if (res.success)
        printf("\n[Server Response] Deposit successful. Your new balance is: $%.2f\n", res.double_data_1);
    else
        printf("\n[Server Response] %s\n", res.message);
}
void handle_withdraw(int socket_fd, int customer_id)
{ /* uses res.double_data_1  */
    struct Request req;
    struct Response res;
    double amount = 0;
    memset(&req, 0, sizeof(req));
    req.op_code = WITHDRAW;
    req.user_id = customer_id;
    printf("\nEnter amount to withdraw: $");
    while (scanf("%lf", &amount) != 1 || amount <= 0)
    {
        printf("Invalid amount: ");
        while (getchar() != '\n')
            ;
    }
    while (getchar() != '\n')
        ;
    req.double_data_1 = amount;
    if (write(socket_fd, &req, sizeof(req)) < 0)
    {
        perror("Client: write() failed");
        return;
    }
    if (read(socket_fd, &res, sizeof(res)) < 0)
    {
        perror("Client: read() failed");
        return;
    }
    if (res.success)
        printf("\n[Server Response] Withdrawal successful. Your new balance is: $%.2f\n", res.double_data_1);
    else
        printf("\n[Server Response] %s\n", res.message);
}
void handle_transfer(int socket_fd, int customer_id)
{ /*  uses res.double_data_1  */
    struct Request req;
    struct Response res;
    int target_account_id = 0;
    double amount = 0;
    memset(&req, 0, sizeof(req));
    req.op_code = TRANSFER;
    req.user_id = customer_id;
    printf("\nEnter the Account ID to transfer to: ");
    while (scanf("%d", &target_account_id) != 1 || target_account_id <= 0)
    {
        printf("Invalid Account ID: ");
        while (getchar() != '\n')
            ;
    }
    req.int_data_1 = target_account_id;
    printf("Enter amount to transfer: $");
    while (scanf("%lf", &amount) != 1 || amount <= 0)
    {
        printf("Invalid amount: ");
        while (getchar() != '\n')
            ;
    }
    while (getchar() != '\n')
        ;
    req.double_data_1 = amount;
    if (write(socket_fd, &req, sizeof(req)) < 0)
    {
        perror("Client: write() failed");
        return;
    }
    if (read(socket_fd, &res, sizeof(res)) < 0)
    {
        perror("Client: read() failed");
        return;
    }
    if (res.success)
        printf("\n[Server Response] Transfer successful. Your new balance is: $%.2f\n", res.double_data_1);
    else
        printf("\n[Server Response] %s\n", res.message);
}
void handle_view_transactions(int socket_fd, int customer_id)
{ 
    struct Request req;
    struct Response res;
    memset(&req, 0, sizeof(req));
    req.op_code = VIEW_TRANSACTIONS;
    req.user_id = customer_id;
    if (write(socket_fd, &req, sizeof(req)) < 0)
    {
        perror("Client: write() failed");
        return;
    }
    printf("\n Transaction History \n");
    int count = 0;
    while (1)
    {
        if (read(socket_fd, &res, sizeof(res)) <= 0)
        {
            perror("Client: read() failed");
            return;
        }
        if (res.success == 1)
        {
            count++;
            printf("%d. %s\n", count, res.message);
        }
        else
        {
            if (count == 0)
                printf("%s\n", res.message);
            break;
        }
    }
    printf(" End of History \n");
}
void handle_change_password(int socket_fd, int user_id)
{ 
    struct Request req;
    struct Response res;
    char old_password[MAX_PASSWORD_LEN];
    char new_password[MAX_PASSWORD_LEN];
    memset(&req, 0, sizeof(req));
    req.op_code = CHANGE_PASSWORD;
    req.user_id = user_id;
    printf("\nEnter current password: ");
    get_input(old_password, MAX_PASSWORD_LEN);
    printf("Enter new password: ");
    get_input(new_password, MAX_PASSWORD_LEN);
    strncpy(req.password, old_password, MAX_PASSWORD_LEN);
    strncpy(req.char_data_1, new_password, sizeof(req.char_data_1));
    if (write(socket_fd, &req, sizeof(req)) < 0)
    {
        perror("Client: write() failed");
        return;
    }
    if (read(socket_fd, &res, sizeof(res)) < 0)
    {
        perror("Client: read() failed");
        return;
    }
    printf("\n[Server Response] %s\n", res.message);
}
void handle_add_feedback(int socket_fd, int customer_id)
{ 
    struct Request req;
    struct Response res;
    memset(&req, 0, sizeof(req));
    req.op_code = ADD_FEEDBACK;
    req.user_id = customer_id;
    printf("\nEnter your feedback (max %d characters):\n", MAX_FEEDBACK_LEN - 1);
    get_input(req.char_data_1, sizeof(req.char_data_1));
    if (write(socket_fd, &req, sizeof(req)) < 0)
    {
        perror("Client: write() failed");
        return;
    }
    if (read(socket_fd, &res, sizeof(res)) < 0)
    {
        perror("Client: read() failed");
        return;
    }
    printf("\n[Server Response] %s\n", res.message);
}
void handle_apply_for_loan(int socket_fd, int customer_id)
{ 
    struct Request req;
    struct Response res;
    double amount = 0;
    memset(&req, 0, sizeof(req));
    req.op_code = APPLY_LOAN;
    req.user_id = customer_id;
    printf("\nEnter loan amount: $");
    while (scanf("%lf", &amount) != 1 || amount <= 0)
    {
        printf("Invalid amount: ");
        while (getchar() != '\n')
            ;
    }
    while (getchar() != '\n')
        ;
    req.double_data_1 = amount;
    if (write(socket_fd, &req, sizeof(req)) < 0)
    {
        perror("Client: write() failed");
        return;
    }
    if (read(socket_fd, &res, sizeof(res)) < 0)
    {
        perror("Client: read() failed");
        return;
    }
    printf("\n[Server Response] %s\n", res.message);
}

//  Utility: Get Input Safely 
void get_input(char *buffer, int size)
{
    fgets(buffer, size, stdin);
    buffer[strcspn(buffer, "\n")] = 0;
}
