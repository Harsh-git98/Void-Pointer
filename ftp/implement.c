#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>

#define CONTROL_PORT 2121
#define MAX_COMMAND_LEN 256
#define BACKLOG 10
#define AUTH_USERNAME "User"
#define AUTH_PASSWORD "pass"

void send_response(int client_socket, const char *message) {
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "%s\r\n", message);
    send(client_socket, buffer, strlen(buffer), 0);
}

int authenticate_user(int client_socket, char **username) {
    char command_buffer[MAX_COMMAND_LEN];
    ssize_t bytes_read;

    send_response(client_socket, "220 Welcome to Modular C-FTP Server");

    while (1) {
        memset(command_buffer, 0, MAX_COMMAND_LEN);
        bytes_read = recv(client_socket, command_buffer, MAX_COMMAND_LEN - 1, 0);
        if (bytes_read <= 0) return 0;

        command_buffer[bytes_read] = '\0';
        char *cmd_line = strtok(command_buffer, "\r\n");
        if (!cmd_line) continue;

        if (strncasecmp(cmd_line, "USER ", 5) == 0) {
            if (*username) free(*username);
            char *user_arg = cmd_line + 5;
            while (*user_arg == ' ') user_arg++;
            *username = strdup(user_arg);
            send_response(client_socket, "331 Username OK, need password");
        }
        else if (strncasecmp(cmd_line, "PASS ", 5) == 0) {
            char *pass_arg = cmd_line + 5;
            while (*pass_arg == ' ') pass_arg++;

            if (*username && strcmp(*username, AUTH_USERNAME) == 0 &&
                strcmp(pass_arg, AUTH_PASSWORD) == 0) {
                send_response(client_socket, "230 Login successful");
                return 1;
            } else {
                send_response(client_socket, "530 Login incorrect");
                return 0;
            }
        }
        else {
            send_response(client_socket, "503 Bad sequence of commands.");
        }
    }
}

// === FTP Command Handlers ===
void handle_pwd(int client_socket) {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        char msg[512];
        snprintf(msg, sizeof(msg), "257 \"%s\" is the current directory.", cwd);
        send_response(client_socket, msg);
    } else {
        send_response(client_socket, "550 Failed to get current directory.");
    }
}

void handle_cwd(int client_socket, const char *path) {
    if (chdir(path) == 0)
        send_response(client_socket, "250 Directory change successful.");
    else
        send_response(client_socket, "550 Failed to change directory.");
}

void handle_list(int client_socket) {
    // NOTE: Proper LIST needs data channel, simplified here
    DIR *dir = opendir(".");
    if (!dir) {
        send_response(client_socket, "550 Failed to list directory.");
        return;
    }

    struct dirent *entry;
    send_response(client_socket, "150 Opening ASCII mode data connection for file list.");
    while ((entry = readdir(dir)) != NULL) {
        char line[256];
        snprintf(line, sizeof(line), "%s\r\n", entry->d_name);
        send(client_socket, line, strlen(line), 0);
    }
    closedir(dir);
    send_response(client_socket, "226 Directory send OK.");
}

void handle_retr(int client_socket, const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        send_response(client_socket, "550 File not found.");
        return;
    }

    send_response(client_socket, "150 Opening data connection for file transfer.");
    char buffer[1024];
    ssize_t n;
    while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
        send(client_socket, buffer, n, 0);
    }
    close(fd);
    send_response(client_socket, "226 Transfer complete.");
}

void handle_stor(int client_socket, const char *filename) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        send_response(client_socket, "550 Cannot create file.");
        return;
    }

    send_response(client_socket, "150 Opening data connection for file upload.");
    char buffer[1024];
    ssize_t n;
    while ((n = recv(client_socket, buffer, sizeof(buffer), MSG_DONTWAIT)) > 0) {
        write(fd, buffer, n);
    }
    close(fd);
    send_response(client_socket, "226 Upload complete.");
}

// === Main Command Loop ===
void command_loop(int client_socket) {
    char command_buffer[MAX_COMMAND_LEN];
    ssize_t bytes_read;

    while (1) {
        memset(command_buffer, 0, MAX_COMMAND_LEN);
        bytes_read = recv(client_socket, command_buffer, MAX_COMMAND_LEN - 1, 0);
        if (bytes_read <= 0) break;

        command_buffer[bytes_read] = '\0';
        char *cmd_line = strtok(command_buffer, "\r\n");
        if (!cmd_line) continue;

        printf("Command: %s\n", cmd_line);

        if (strncasecmp(cmd_line, "PWD", 3) == 0) {
            handle_pwd(client_socket);
        }
        else if (strncasecmp(cmd_line, "CWD ", 4) == 0) {
            handle_cwd(client_socket, cmd_line + 4);
        }
        else if (strncasecmp(cmd_line, "LIST", 4) == 0) {
            handle_list(client_socket);
        }
        else if (strncasecmp(cmd_line, "RETR ", 5) == 0) {
            handle_retr(client_socket, cmd_line + 5);
        }
        else if (strncasecmp(cmd_line, "STOR ", 5) == 0) {
            handle_stor(client_socket, cmd_line + 5);
        }
        else if (strncasecmp(cmd_line, "TYPE ", 5) == 0) {
            send_response(client_socket, "200 Type set to Binary.");
        }
        else if (strncasecmp(cmd_line, "SYST", 4) == 0) {
            send_response(client_socket, "215 UNIX Type: L8");
        }
        else if (strncasecmp(cmd_line, "QUIT", 4) == 0) {
            send_response(client_socket, "221 Conection Closed");
            break;
        }
        else {
            send_response(client_socket, "500 Syntax error, command unrecognized.");
        }
    }
}

// === Handle Client Session ===
void handle_client(int client_socket) {
    char *username = NULL;
    int authenticated = authenticate_user(client_socket, &username);

    if (authenticated) {
        command_loop(client_socket);
    }

    if (username) free(username);
    close(client_socket);
    printf("Client session closed.\n");
}
