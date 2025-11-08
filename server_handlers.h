#ifndef SERVER_HANDLERS_H
#define SERVER_HANDLERS_H

#include "bank_system.h"
#include <time.h>   // For transaction timestamp, ctime
#include <stddef.h> // For offsetof()

// --- Prototypes for All Handlers ---

// common_handler.c
int handle_login_request(int client_socket_fd, struct Request *req); // <-- MODIFIED
void handle_change_password_request(int client_socket_fd, struct Request *req);

// customer_handler.c
void handle_view_balance_request(int client_socket_fd, struct Request *req);
void handle_deposit_request(int client_socket_fd, struct Request *req);
void handle_withdraw_request(int client_socket_fd, struct Request *req);
void handle_transfer_request(int client_socket_fd, struct Request *req);
void handle_view_transactions_request(int client_socket_fd, struct Request *req);
void handle_add_feedback_request(int client_socket_fd, struct Request *req);
void handle_apply_for_loan_request(int client_socket_fd, struct Request *req);

// employee_handler.c
void handle_add_customer_request(int client_socket_fd, struct Request *req);
void handle_modify_customer_request(int client_socket_fd, struct Request *req);
void handle_view_customer_txn_request(int client_socket_fd, struct Request *req);
void handle_view_assigned_loans_request(int client_socket_fd, struct Request *req);
void handle_process_loan_request(int client_socket_fd, struct Request *req);

// manager_handler.c
void handle_toggle_account_status_request(int client_socket_fd, struct Request *req);
void handle_assign_loan_request(int client_socket_fd, struct Request *req);
void handle_review_feedback_request(int client_socket_fd, struct Request *req);
void handle_view_unassigned_loans_request(int client_socket_fd, struct Request *req);

// admin_handler.c
void handle_add_employee_request(int client_socket_fd, struct Request *req);
void handle_modify_user_details_request(int client_socket_fd, struct Request *req);
void handle_manage_roles_request(int client_socket_fd, struct Request *req);

// --- Prototypes for Helper Functions (server_utils.c) ---
const char *get_transaction_type_string(OperationCode type);
int get_account_id_from_user_id(int user_id);
int find_account_record(int fd, int account_id, struct Account *acc_out, off_t *offset_out);
int find_customer_record(int fd, int customer_id, struct Customer *cust_out, off_t *offset_out);
int find_employee_record(int fd, int employee_id, struct Employee *emp_out, off_t *offset_out);
int find_loan_record(int fd, int loan_id, struct Loan *loan_out, off_t *offset_out);
int find_user_record(int fd, int user_id, struct User *user_out, off_t *offset_out);
int get_next_id(int fd, int struct_size, int id_offset);
int lock_file(int fd, int lock_type);
int unlock_file(int fd);
int lock_record(int fd, int cmd, int type, off_t offset, off_t len);

#endif // SERVER_HANDLERS_H
