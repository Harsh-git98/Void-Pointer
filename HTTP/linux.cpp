#include <bits/stdc++.h>
#include <string>
#include <cstring>
#include <vector>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "request.cpp"
using namespace std;

void handle_REST(vector<string>& words, string body, string &response) {
    if (words[0] == "GET") {
        handle_GET(words[1], response);
    }
    if (words[0] == "POST") {
        handle_POST(words[1], response, body);
    }
    if (words[0] == "PUT") {
        handle_PUT(words[1], response);
    }
    if (words[0] == "DELETE") {
        handle_DELETE(words[1], response);
    }
}

void handle_incoming_request(char buffer[], string &response) {
    stringstream ss(buffer);
    string line;
    vector<string> incoming;

    while (getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        incoming.push_back(line);
    }

    if (incoming.empty()) return;

    istringstream iss(incoming[0]);
    vector<string> words;
    string word;
    while (iss >> word) {
        words.push_back(word);
    }

    string body = "";
    for (size_t i = 0; i < incoming.size(); i++) {
        if (incoming[i].empty()) { // blank line
            // everything after is body
            for (size_t j = i + 1; j < incoming.size(); j++) {
                body += incoming[j] + "\n";
            }
            break;
        }
    }

    handle_REST(words, body, response);

    if (response.empty()) {
        response =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: 13\r\n"
            "Connection: close\r\n"
            "\r\n"
            "<h1>404</h1>";
    }
}

void handle_client(int client_socket) {
    char buffer[6000] = {0};
    int valread = read(client_socket, buffer, sizeof(buffer) - 1);

    if (valread > 0) {
        buffer[valread] = '\0';
        string response;
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
    vector<thread> threads;

    while (true) {
        int client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (client_socket < 0) {
            perror("accept");
            continue;
        }
        threads.emplace_back(thread(handle_client, client_socket));
        threads.back().detach();
    }

    return 0;
}
