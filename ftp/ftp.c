#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "implement.c"


#define CONTROL_PORT 2121
#define MAX_COMMAND_LEN 256
#define BACKLOG 10
#define AUTH_USERNAME "User"
#define AUTH_PASSWORD "pass"

void *client_thread(void *arg) {
    int client_socket = *(int *)arg;
    free(arg);  
    handle_client(client_socket);
    close(client_socket);
    return NULL;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int opt = 1;
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(CONTROL_PORT);

    if (bind(server_fd, (struct sockaddr *)&address, addrlen) < 0) {
        perror("bind failed (Try running as root or changing CONTROL_PORT)");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("FTP C Server listening on port %d...\n", CONTROL_PORT);

    while (1) {
        printf("Waiting for a connection...\n");
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept failed");
            continue;
        }
        printf("New connection accepted from %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
        
        // Thread for each client
        int *client_sock = malloc(sizeof(int));
        *client_sock = new_socket;

        pthread_t tid;
        if (pthread_create(&tid, NULL, client_thread, client_sock) != 0) {
            perror("pthread_create failed");
            close(new_socket);
            free(client_sock);
        }
        pthread_detach(tid); 

    }

    close(server_fd);

    return 0;
}