#ifndef BANK_SYSTEM_H
#define BANK_SYSTEM_H

// --- 1. Common Libraries ---
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>

// --- 2. Constants ---
#define SERVER_PORT 8080
#define MAX_USERNAME_LEN 50
#define MAX_PASSWORD_LEN 50
#define MAX_NAME_LEN 100
#define MAX_FEEDBACK_LEN 256

// Data file paths
#define ACCOUNT_FILE "data/accounts.dat"
#define USER_FILE "data/users.dat"
#define LOAN_FILE "data/loans.dat"
#define TRANSACTION_FILE "data/transactions.dat"
#define CUSTOMER_FILE "data/customers.dat"
#define EMPLOYEE_FILE "data/employees.dat"
#define FEEDBACK_FILE "data/feedback.dat"

// --- 3. User Roles ---
typedef enum
{
    CUSTOMER = 1,
    EMPLOYEE = 2,
    MANAGER = 3,
    ADMIN = 4
} UserRole;

// --- 4. Operation Codes (Client-Server Commands) ---
typedef enum
{
    // General
    LOGIN,
    CHANGE_PASSWORD,
    LOGOUT,
    EXIT,

    // Customer
    VIEW_BALANCE,
    DEPOSIT,
    WITHDRAW,
    TRANSFER,
    APPLY_LOAN,
    VIEW_TRANSACTIONS,
    ADD_FEEDBACK,

    // Employee
    ADD_CUSTOMER,
    MODIFY_CUSTOMER,
    PROCESS_LOAN,
    VIEW_CUSTOMER_TXN,
    VIEW_ASSIGNED_LOANS,

    // Manager
    TOGGLE_ACCOUNT_STATUS,
    ASSIGN_LOAN,
    VIEW_FEEDBACK,
    VIEW_UNASSIGNED_LOANS, // <-- NEW

    // Admin
    ADD_EMPLOYEE,
    MODIFY_USER_DETAILS,
    MANAGE_ROLES

} OperationCode;

// --- 5. Core Data Structures ---

struct Account
{
    int account_id;
    double balance;
    int is_active;
};

struct User
{
    int user_id;
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];
    UserRole role;
    int customer_or_employee_id;
};

struct Customer
{
    int customer_id;
    char name[MAX_NAME_LEN];
    int account_id;
};

struct Employee
{
    int employee_id;
    char name[MAX_NAME_LEN];
};

struct Loan
{
    int loan_id;
    int customer_id;
    double amount;
    int assigned_to_employee_id; // -1 = unassigned
    int status;                  // 0=Pending, 1=Approved, 2=Rejected
};

struct Transaction
{
    int transaction_id;
    int account_id;
    OperationCode type;
    double amount;
    long timestamp;
};

struct Feedback
{
    int customer_id;
    char message[MAX_FEEDBACK_LEN];
    long timestamp;
};

// Sent from Client to Server
struct Request
{
    OperationCode op_code;
    int user_id;
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];
    UserRole user_role;
    int int_data_1;
    int int_data_2; // <-- NEW
    double double_data_1;
    char char_data_1[MAX_FEEDBACK_LEN];
};

// Sent from Server to Client
struct Response
{
    int success;
    char message[256];
    int user_id;
    double double_data_1; // <-- RENAMED (was 'balance')
    // We can add more fields later if needed
};

#endif // BANK_SYSTEM_H
