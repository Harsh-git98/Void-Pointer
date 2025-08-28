#include "router.h"
#include "handlers/handlers.h"
#include <sstream>
#include <vector>

void handle_REST(std::vector<std::string>& words, std::string body, std::string &response) {
    if (words[0] == "GET") {
        handle_GET(words[1], response);
    } else if (words[0] == "POST") {
        handle_POST(words[1], response, body);
    } else if (words[0] == "PUT") {
        handle_PUT(words[1], response);
    } else if (words[0] == "DELETE") {
        handle_DELETE(words[1], response);
    }
}

void handle_incoming_request(char buffer[], std::string &response) {
    std::stringstream ss(buffer);
    std::string line;
    std::vector<std::string> incoming;

    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
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
        if (incoming[i].empty()) {
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
