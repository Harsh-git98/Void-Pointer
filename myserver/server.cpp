#include "server.h"
#include "router.h"
#include <iostream>
#include <thread>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

void handle_client(int client_socket) {
    char buffer[6000] = {0};
    int valread = read(client_socket, buffer, sizeof(buffer) - 1);

    if (valread > 0) {
        buffer[valread] = '\0';
        std::string response;
        handle_incoming_request(buffer, response);

        if (response.empty()) {
            response =
                "HTTP/1.1 500 Internal Server Error\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 21\r\n"
                "Connection: close\r\n"
                "\r\n"
                "Server error occurred";
        }

        send(client_socket, response.c_str(), response.size(), 0);
    }
    close(client_socket);
}

void start_server(int port) {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 4) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    std::vector<std::thread> threads;

    while (true) {
        int client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (client_socket < 0) {
            perror("accept");
            continue;
        }
        threads.emplace_back(std::thread(handle_client, client_socket));
        threads.back().detach();
    }
}
