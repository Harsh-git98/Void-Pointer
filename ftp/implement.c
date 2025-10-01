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

char client_ip[64];
int client_data_port = 0;


// BACK TO CLIENT RESPONSE
void send_response(int client_socket, const char *message) {
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "%s\r\n", message);
    send(client_socket, buffer, strlen(buffer), 0);
}


int authenticate_user(int client_socket, char **username) {
    char command_buffer[MAX_COMMAND_LEN];
    ssize_t bytes_read;

    send_response(client_socket, "220 Welcome to C-FTP Server");

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

void handle_port(int client_socket, const char *arg) {
    int h1,h2,h3,h4,p1,p2;
    if (sscanf(arg, "%d,%d,%d,%d,%d,%d", 
               &h1,&h2,&h3,&h4,&p1,&p2) != 6) {
        send_response(client_socket, "501 Syntax error in parameters.");
        return;
    }

    snprintf(client_ip, sizeof(client_ip), "%d.%d.%d.%d", h1,h2,h3,h4);
    client_data_port = p1 * 256 + p2;

    send_response(client_socket, "200 PORT command successful.");
}
// Creating Active Connection.
int open_data_connection() {
    int data_fd;
    struct sockaddr_in data_addr;

    if (client_data_port == 0) return -1;

    data_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (data_fd < 0) return -1;

    memset(&data_addr, 0, sizeof(data_addr));
    data_addr.sin_family = AF_INET;
    data_addr.sin_port = htons(client_data_port);
    inet_pton(AF_INET, client_ip, &data_addr.sin_addr);
    printf("open data connection on: %s %d\n", client_ip, ntohs(data_addr.sin_port));

    if (connect(data_fd, (struct sockaddr *)&data_addr, sizeof(data_addr)) < 0) {
        perror("Data connection failed");
        close(data_fd);
        return -1;
    }
    return data_fd;
}



// print the current Directory
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

// change the directory to path
void handle_cwd(int client_socket, const char *path) {
    if (chdir(path) == 0) {
        char response[512];

        // Get current directory after chdir
        char cwd[256];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            snprintf(response, sizeof(response), "250 Directory changed to %s", cwd);
        } else {
            snprintf(response, sizeof(response), "250 Directory change successful.");
        }

        send_response(client_socket, response);
    } else {
        send_response(client_socket, "550 Failed to change directory.");
    }
}

// List all the directories and file
void handle_list(int client_socket) {
    int data_fd = open_data_connection();
    if (data_fd < 0) {
        send_response(client_socket, "425 Can't open data connection.");
        return;
    }
    send_response(client_socket, "150 Opening data connection for LIST");

    DIR *dir = opendir(".");
    if (!dir) {
        send_response(client_socket, "550 Failed to list directory.");
        close(data_fd);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        char line[256];
        snprintf(line, sizeof(line), "- %s\r\n", entry->d_name);
        send(data_fd, line, strlen(line), 0);
    }
    closedir(dir);
    close(data_fd);
    send_response(client_socket, "220 Transfer complete.");
}

// RETRIVING THE FILE FROM THE REMOTE SERVER
void handle_retr(int client_socket, const char *filename) {
    int data_fd = open_data_connection();
    if (data_fd < 0) {
        send_response(client_socket, "425 Can't open data connection.");
        return;
    }
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        send_response(client_socket, "550 File not found.");
        close(data_fd);
      
        return;
    }
    send_response(client_socket, "150 Opening data connection for RETR");
    char buffer[1024];
    ssize_t n;
    while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
        send(data_fd, buffer, n, 0);
    }
    close(fd);
    close(data_fd);
    
    send_response(client_socket, "226 Transfer complete.");
}

// === Main Command Loop (Active Mode, strict commands) ===
// ONLY RETRVAL FROM REMOTE SERVER IMPLEMENTED. 
void command_loop(int client_socket) {
    char command_buffer[MAX_COMMAND_LEN];
    ssize_t bytes_read;

    while (1) {
        memset(command_buffer, 0, MAX_COMMAND_LEN);
        bytes_read = recv(client_socket, command_buffer, MAX_COMMAND_LEN - 1, 0);
       // printf("-FTP Client sends: %s\n",command_buffer);
        if (bytes_read <= 0) break;

        command_buffer[bytes_read] = '\0';
        char *cmd_line = strtok(command_buffer, "\r\n");
        if (!cmd_line) continue;

        printf("Command: %s\n", cmd_line);

        if (strncasecmp(cmd_line, "PWD", 3) == 0) {
            handle_pwd(client_socket);
        }
        else if (strncasecmp(cmd_line, "CWD", 3) == 0) {
            char *arg = cmd_line + 3;
            while (*arg == ' ') arg++; 
            if (*arg)
                handle_cwd(client_socket, arg);
            else
                send_response(client_socket, "501 Missing directory name.");
        }
        else if (strncasecmp(cmd_line, "PORT ", 5) == 0) {
            handle_port(client_socket, cmd_line + 5);
        }
        else if (strncasecmp(cmd_line, "LIST", 4) == 0 || strncasecmp(cmd_line, "LS", 2) == 0) {
            handle_list(client_socket);
        }
        else if (strncasecmp(cmd_line, "RETR ", 5) == 0) {
            handle_retr(client_socket, cmd_line + 5);
        }
        else if (strncasecmp(cmd_line, "TYPE ", 5) == 0) {
            send_response(client_socket, "200 Type set to Binary.");
        }
        else if (strncasecmp(cmd_line, "SYST", 4) == 0) {
            send_response(client_socket, "215 UNIX Type: L8");
        }
        else if (strncasecmp(cmd_line, "QUIT", 4) == 0) {
            send_response(client_socket, "221 Goodbye.");
            break;
        }
         else if (strncasecmp(cmd_line, "EPSV", 4) == 0) {
            send_response(client_socket, "EPSV call");
        }
         else if (strncasecmp(cmd_line, "PASV", 4) == 0) {
            send_response(client_socket, "PASV call");
        }
        else {
            send_response(client_socket, "502 Command not implemented.");
        }
    }
}
    
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