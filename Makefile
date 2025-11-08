# Compiler
CC = gcc

# Compiler flags
# -g: Add debug information
# -Wall: Turn on all warnings
# -pthread: Link the pthreads library
CFLAGS = -g -Wall -pthread

# All server .c files
SERVER_SRCS = \
    server.c \
    common_handler.c \
    customer_handler.c \
    employee_handler.c \
    manager_handler.c \
    admin_handler.c \
    server_utils.c

# Server object files (auto-generated from .c files)
SERVER_OBJS = $(SERVER_SRCS:.c=.o)

# Client source file
CLIENT_SRC = client.c

# Admin utility source file
ADMIN_UTIL_SRC = admin.c

# Target executables
SERVER_EXE = server
CLIENT_EXE = client
ADMIN_UTIL_EXE = admin_util

# Default rule: build all executables
all: $(SERVER_EXE) $(CLIENT_EXE) $(ADMIN_UTIL_EXE)

# Rule to link the server executable
$(SERVER_EXE): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $(SERVER_EXE) $(SERVER_OBJS)

# Rule to link the client executable
$(CLIENT_EXE): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT_EXE) $(CLIENT_SRC)

# Rule to link the admin utility
$(ADMIN_UTIL_EXE): $(ADMIN_UTIL_SRC)
	$(CC) $(CFLAGS) -o $(ADMIN_UTIL_EXE) $(ADMIN_UTIL_SRC)

# Generic rule to compile any .c file into a .o file
%.o: %.c server_handlers.h bank_system.h
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(SERVER_EXE) $(CLIENT_EXE) $(ADMIN_UTIL_EXE) $(SERVER_OBJS) data/*.dat
