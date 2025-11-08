#include "bank_system.h"
#include "server_handlers.h" // <-- Include the new header

void *client_handler(void *socket_descriptor); // Prototype for client_handler

// --- BEGIN NEW SESSION STATE ---
#define MAX_CONCURRENT_USERS 100
int active_user_ids[MAX_CONCURRENT_USERS];
int active_user_count = 0;
pthread_mutex_t session_mutex;
// --- END NEW SESSION STATE ---

// Main Server Function
int main()
{
    int server_fd, new_socket_fd;
    struct sockaddr_in server_address, client_address;
    int addr_len = sizeof(client_address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("Server: socket() failed");
        exit(EXIT_FAILURE);
    }

    // Allow reusing the address to avoid "Address already in use" error
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("Server: setsockopt(SO_REUSEADDR) failed");
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(SERVER_PORT);

    if (bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("Server: bind() failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0)
    {
        perror("Server: listen() failed");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d...\n", SERVER_PORT);

    // --- INITIALIZE THE MUTEX ---
    pthread_mutex_init(&session_mutex, NULL);

    // Main accept loop
    while (1)
    {
        new_socket_fd = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t *)&addr_len);
        if (new_socket_fd < 0)
        {
            perror("Server: accept() failed");
            continue; // Continue to next connection
        }

        printf("New client connected! Socket FD: %d\n", new_socket_fd);

        // Create a new thread for each client
        pthread_t client_thread;
        int *client_sock_ptr = (int *)malloc(sizeof(int));
        *client_sock_ptr = new_socket_fd;

        if (pthread_create(&client_thread, NULL, client_handler, (void *)client_sock_ptr) < 0)
        {
            perror("Server: pthread_create() failed");
            close(new_socket_fd);
        }

        pthread_detach(client_thread); // Detach thread to auto-release resources
    }

    close(server_fd);
    return 0;
}

// Main Client Handler Thread
void *client_handler(void *socket_descriptor)
{
    int client_socket_fd = *(int *)socket_descriptor;
    free(socket_descriptor);

    printf("Client thread started for socket %d\n", client_socket_fd);
    struct Request client_req;
    ssize_t read_bytes;

    int logged_in_user_id = -1; // <-- NEW: -1 means not logged in

    // Loop to read requests from this client
    while (1)
    {
        read_bytes = read(client_socket_fd, &client_req, sizeof(struct Request));
        if (read_bytes <= 0)
        {
            printf("Client on socket %d disconnected.\n", client_socket_fd);
            break; // Exit loop, jump to cleanup
        }

        // Route the request to the correct handler function
        switch (client_req.op_code)
        {
        // Common
        case LOGIN:
        { // <-- Add braces
            // Store the ID on successful login
            int login_id = handle_login_request(client_socket_fd, &client_req);
            if (login_id != -1)
            {
                logged_in_user_id = login_id;
            }
        } // <-- Add braces
        break;
        case CHANGE_PASSWORD:
            handle_change_password_request(client_socket_fd, &client_req);
            break;

        // Customer
        case VIEW_BALANCE:
            handle_view_balance_request(client_socket_fd, &client_req);
            break;
        case DEPOSIT:
            handle_deposit_request(client_socket_fd, &client_req);
            break;
        case WITHDRAW:
            handle_withdraw_request(client_socket_fd, &client_req);
            break;
        case TRANSFER:
            handle_transfer_request(client_socket_fd, &client_req);
            break;
        case VIEW_TRANSACTIONS:
            handle_view_transactions_request(client_socket_fd, &client_req);
            break;
        case ADD_FEEDBACK:
            handle_add_feedback_request(client_socket_fd, &client_req);
            break;
        case APPLY_LOAN:
            handle_apply_for_loan_request(client_socket_fd, &client_req);
            break;

        // Employee
        case ADD_CUSTOMER:
            handle_add_customer_request(client_socket_fd, &client_req);
            break;
        case MODIFY_CUSTOMER:
            handle_modify_customer_request(client_socket_fd, &client_req);
            break;
        case VIEW_CUSTOMER_TXN:
            handle_view_customer_txn_request(client_socket_fd, &client_req);
            break;
        case VIEW_ASSIGNED_LOANS:
            handle_view_assigned_loans_request(client_socket_fd, &client_req);
            break;
        case PROCESS_LOAN:
            handle_process_loan_request(client_socket_fd, &client_req);
            break;

        // Manager
        case TOGGLE_ACCOUNT_STATUS:
            handle_toggle_account_status_request(client_socket_fd, &client_req);
            break;
        case VIEW_FEEDBACK:
            handle_review_feedback_request(client_socket_fd, &client_req);
            break;
        case VIEW_UNASSIGNED_LOANS:
            handle_view_unassigned_loans_request(client_socket_fd, &client_req);
            break;
        case ASSIGN_LOAN:
            handle_assign_loan_request(client_socket_fd, &client_req);
            break;

        // Admin
        case ADD_EMPLOYEE:
            handle_add_employee_request(client_socket_fd, &client_req);
            break;
        case MODIFY_USER_DETAILS:
            handle_modify_user_details_request(client_socket_fd, &client_req);
            break;
        case MANAGE_ROLES:
            handle_manage_roles_request(client_socket_fd, &client_req);
            break;

        // General
        case EXIT:
            printf("Client on socket %d requested EXIT.\n", client_socket_fd);
            goto cleanup_session; // <-- JUMP TO CLEANUP
        default:
            printf("Received unknown operation code from socket %d\n", client_socket_fd);
            struct Response err_res;
            memset(&err_res, 0, sizeof(err_res));
            err_res.success = 0;
            strcpy(err_res.message, "Error: Unknown command received by server.");
            write(client_socket_fd, &err_res, sizeof(struct Response));
        }
    }

cleanup_session: // <-- NEW LABEL
    if (logged_in_user_id != -1)
    {
        printf("Logging out user ID %d from session.\n", logged_in_user_id);
        // --- BEGIN LOGOUT LOGIC ---
        pthread_mutex_lock(&session_mutex);
        for (int i = 0; i < active_user_count; i++)
        {
            if (active_user_ids[i] == logged_in_user_id)
            {
                // Found user: remove by swapping with the last element
                active_user_ids[i] = active_user_ids[active_user_count - 1];
                active_user_count--;
                printf("User ID %d removed from active session.\n", logged_in_user_id);
                break;
            }
        }
        pthread_mutex_unlock(&session_mutex);
        // --- END LOGOUT LOGIC ---
    }

    close(client_socket_fd);
    return NULL;
}
