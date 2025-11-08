#include "bank_system.h"
#include <errno.h> // For error checking

/**
 * This utility creates/overwrites 'users.dat'
 * and adds the first Administrator and Manager users.
 */
int main() {
    int fd;
    struct User admin_user, manager_user;

    // --- 1. Create the Admin User Struct ---
    memset(&admin_user, 0, sizeof(struct User));
    admin_user.user_id = 1; // First user ID
    admin_user.role = ADMIN;
    admin_user.customer_or_employee_id = -1; // Not linked
    strcpy(admin_user.username, "admin");
    strcpy(admin_user.password, "admin123"); // Consider changing default

    // --- 2. Create the Manager User Struct ---
    memset(&manager_user, 0, sizeof(struct User));
    manager_user.user_id = 2; // Second user ID
    manager_user.role = MANAGER;
    manager_user.customer_or_employee_id = -1; // Not linked yet
    strcpy(manager_user.username, "manager");
    strcpy(manager_user.password, "manager123"); // Consider changing default
    
    // --- 3. Open (or Create/Truncate) the User File ---
    fd = open(USER_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Error opening user file");
        return 1;
    }

    // --- 4. Write the Admin User ---
    if (write(fd, &admin_user, sizeof(struct User)) != sizeof(struct User)) {
        perror("Error writing admin user");
        close(fd);
        return 1;
    }
    
    // --- 5. Write the Manager User ---
    if (write(fd, &manager_user, sizeof(struct User)) != sizeof(struct User)) {
        perror("Error writing manager user");
        close(fd);
        return 1;
    }

    // --- 6. Close the File ---
    close(fd);
    
    printf("Successfully created/overwritten '%s'.\n", USER_FILE);
    printf("Added Admin -> Username: %s, Password: %s\n", admin_user.username, admin_user.password);
    printf("Added Manager -> Username: %s, Password: %s\n", manager_user.username, manager_user.password);
    
    return 0;
}
