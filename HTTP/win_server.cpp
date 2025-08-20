#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstring>
#include <bits/stdc++.h>
#include <string>
#include <vector>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include<bits/stdc++.h>
#include "request.cpp"
#pragma comment(lib, "ws2_32.lib")

void handle_REST(vector<string>& words, string body,string &response)
{
    // for(int i=0;i<words.size();i++)
    // {
    //     cout<<words[i]<<endl;
    // }
    // cout<<body<<endl;
    if(words[0]=="GET")
    {
        handle_GET(words[1],response);
    }
     if(words[0]=="POST")
    {
        handle_POST(words[1],response,body);
    }
     if(words[0]=="PUT")
    {
        handle_PUT(words[1],response);
    }
     if(words[0]=="DELETE")
    {
        handle_DELETE(words[1],response);
    }
}
void handle_incoming_request(char buffer[], string &response)
{
    std::stringstream ss(buffer);
std::string line;
std::vector<std::string> incoming;

while (std::getline(ss, line)) {
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();  // remove trailing \r
    }
    incoming.push_back(line);
}

   if (incoming.empty()) return;   

    std::istringstream iss(incoming[0]);
std::vector<std::string> words;
std::string word;
while (iss >> word) {
    words.push_back(word);
}



   std::string body = "";
for (size_t i = 0; i < incoming.size(); i++) {
    if (incoming[i].empty()) { // blank line
        // everything after is body
        for (size_t j = i+1; j < incoming.size(); j++) {
            body += incoming[j] + "\n";
        }
        break;
    }
}

    handle_REST(words,body,response);
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
    int valread = read(client_socket, buffer, sizeof(buffer)-1);
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
int main() {
    WSADATA wsa;
    SOCKET server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        std::cerr << "Socket creation failed\n";
        WSACleanup();
        return 1;
    }

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
        std::cerr << "Bind failed\n";
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    if (listen(server_fd, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed\n";
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    std::cout << "Server running on http://localhost:8080\n";

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
    closesocket(server_fd);
    WSACleanup();
    return 0;
}
