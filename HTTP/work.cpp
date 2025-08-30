#include <string>
#include <cstring>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <bits/stdc++.h>

using namespace std;

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Reuse address and port
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;  // Listen on all interfaces
    address.sin_port = htons(8080);        // Port 8080

    // Bind socket to IP:Port
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Start listening
    if (listen(server_fd, 4) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    cout << "Server running on http://localhost:8080\n";

    while (true) {
        int client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (client_socket < 0) {
            perror("accept");
            continue;
        }

        char buffer[3000] = {0};
        int valread = read(client_socket, buffer, sizeof(buffer) - 1);

        cout << buffer << endl;

        string body = "<h1>Your C++ server is live and running</h1>";

        string response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: " + to_string(body.size()) + "\r\n"
            "Connection: close\r\n"
            "\r\n" +
            body;

            cout<<response<<endl;
        send(client_socket, response.c_str(), response.size(), 0);


        close(client_socket);
    }

    close(server_fd);
    return 0;
}
