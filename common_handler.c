#include "server_handlers.h"

// --- Access global state from server.c ---
#define MAX_CONCURRENT_USERS 100 // Must match server.c
extern int active_user_ids[MAX_CONCURRENT_USERS];
extern int active_user_count;
extern pthread_mutex_t session_mutex;
// --- End Access ---

int handle_login_request(int client_socket_fd, struct Request *req) // <-- NEW SIGNATURE
{
    printf("Processing LOGIN request for user '%s'...\n", req->username);
    struct Response res;
    struct User user_from_file;
    int found = 0;
    int fd = -1;
    memset(&res, 0, sizeof(res));
    res.success = 0; // Default to failure

    fd = open(USER_FILE, O_RDONLY);
    if (fd < 0)
    {
        perror("Server: Error opening user file");
        strcpy(res.message, "Server error: Could not check credentials.");
    }
    else
    {
        if (lock_file(fd, F_RDLCK) == -1)
        {
            perror("Server: Failed to get read lock on user file");
            strcpy(res.message, "Server busy. Please try again.");
        }
        else
        {
            ssize_t read_bytes;
            while ((read_bytes = read(fd, &user_from_file, sizeof(user_from_file))) == sizeof(user_from_file))
            {
                if (strcmp(req->username, user_from_file.username) == 0 &&
                    strcmp(req->password, user_from_file.password) == 0 &&
                    req->user_role == user_from_file.role)
                {
                    found = 1;

                    // --- BEGIN NEW SESSION CHECK ---
                    pthread_mutex_lock(&session_mutex);
                    int already_logged_in = 0;
                    for (int i = 0; i < active_user_count; i++)
                    {
                        if (active_user_ids[i] == user_from_file.user_id)
                        {
                            already_logged_in = 1;
                            break;
                        }
                    }

                    if (already_logged_in)
                    {
                        res.success = 0;
                        strcpy(res.message, "Login Failed: User is already logged in.");
                    }
                    else if (active_user_count >= MAX_CONCURRENT_USERS)
                    {
                        res.success = 0;
                        strcpy(res.message, "Login Failed: Server is full.");
                    }
                    else
                    {
                        // Add user to session list
                        active_user_ids[active_user_count] = user_from_file.user_id;
                        active_user_count++;
                        res.success = 1;
                        res.user_id = user_from_file.user_id;
                        strcpy(res.message, "Login Successful!");
                    }
                    pthread_mutex_unlock(&session_mutex);
                    // --- END NEW SESSION CHECK ---

                    break; // Break from the while(read...) loop
                }
            }
            if (read_bytes < 0)
                perror("Server: Error reading user file");
            unlock_file(fd);
        }
        close(fd);

        if (!found && res.success == 0 && strlen(res.message) == 0) // Only set if no other error was set
        {
            strcpy(res.message, "Invalid username, password, or role.");
        }
    }

    if (write(client_socket_fd, &res, sizeof(res)) < 0)
    {
        perror("Server: write() response failed");
        // If write fails, we can't log them in, so return -1
        if (res.success)
        { // If login *was* successful but write failed, we need to undo the login
            pthread_mutex_lock(&session_mutex);
            for (int i = 0; i < active_user_count; i++)
            {
                if (active_user_ids[i] == res.user_id)
                {
                    active_user_ids[i] = active_user_ids[active_user_count - 1];
                    active_user_count--;
                    break;
                }
            }
            pthread_mutex_unlock(&session_mutex);
        }
        return -1;
    }

    // --- ADD RETURN VALUES ---
    if (res.success)
    {
        return res.user_id;
    }
    else
    {
        return -1;
    }
}

void handle_change_password_request(int client_socket_fd, struct Request *req)
{
    printf("Processing CHANGE_PASSWORD request for User ID %d...\n", req->user_id);
    struct Response res;
    struct User user_record;
    int user_fd = -1;
    off_t user_offset = -1;
    int user_rec_locked = 0;
    int found = 0;

    memset(&res, 0, sizeof(res));
    res.success = 1;
    user_fd = open(USER_FILE, O_RDWR);
    if (user_fd < 0)
    {
        perror("Server: Error opening user file");
        res.success = 0;
        strcpy(res.message, "Server error: Could not access user data.");
        goto cleanup_changepw;
    }

    ssize_t read_bytes;
    off_t current_offset = 0;
    while ((read_bytes = read(user_fd, &user_record, sizeof(user_record))) == sizeof(user_record))
    {
        if (user_record.user_id == req->user_id)
        {
            found = 1;
            user_offset = current_offset;
            printf("Attempting lock user offset %ld\n", user_offset);
            if (lock_record(user_fd, F_SETLKW, F_WRLCK, user_offset, sizeof(user_record)) == -1)
            {
                perror("Server: Failed lock user record");
                res.success = 0;
                strcpy(res.message, "Server busy (user record in use).");
            }
            else
            {
                user_rec_locked = 1;
            }
            break;
        }
        current_offset += sizeof(user_record);
    }

    if (read_bytes < 0)
    {
        perror("Server: Error reading user file");
        res.success = 0;
        strcpy(res.message, "Server error: Failed read user data.");
    }
    if (!found && res.success != 0)
    {
        res.success = 0;
        strcpy(res.message, "Error: User record not found.");
    }
    if (res.success == 0)
    {
        goto cleanup_changepw;
    }

    printf("User record lock acquired.\n");
    if (strcmp(user_record.password, req->password) != 0)
    {
        res.success = 0;
        strcpy(res.message, "Password change failed: Incorrect current password.");
    }
    else
    {
        strncpy(user_record.password, req->char_data_1, MAX_PASSWORD_LEN);
        user_record.password[MAX_PASSWORD_LEN - 1] = '\0';
        lseek(user_fd, user_offset, SEEK_SET);
        if (write(user_fd, &user_record, sizeof(user_record)) <= 0)
        {
            perror("Server: Failed write updated user record");
            res.success = 0;
            strcpy(res.message, "Server error: Failed save new password.");
        }
        else
        {
            res.success = 1;
            strcpy(res.message, "Password changed successfully.");
            printf("User ID %d changed password.\n", req->user_id);
        }
    }

    if (user_rec_locked)
    {
        lock_record(user_fd, F_SETLK, F_UNLCK, user_offset, sizeof(user_record));
        user_rec_locked = 0;
        printf("User record lock released.\n");
    }

cleanup_changepw:
    if (user_rec_locked)
    {
        lock_record(user_fd, F_SETLK, F_UNLCK, user_offset, sizeof(user_record));
        printf("User record lock released cleanup.\n");
    }
    if (user_fd != -1)
        close(user_fd);
    if (write(client_socket_fd, &res, sizeof(res)) < 0)
    {
        perror("Server: write() response failed");
    }
}
