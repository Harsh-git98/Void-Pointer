#include "handlers.h"
#include "../storage/persistence.h"
#include "../utils/json.h"
#include <iostream>

void handle_POST(std::string route, std::string &response, std::string body) {
    std::cout << "route: " << route << std::endl;
    std::cout << "body: " << body << std::endl;

    if (route == "/api") {
        std::string extracted = body;

        if (!extracted.empty() && extracted.front() == '"' && extracted.back() == '"') {
            extracted = extracted.substr(1, extracted.size() - 2);
        }
        if (!extracted.empty() && (extracted.back() == '\n' || extracted.back() == '\r')) {
            extracted.pop_back();
        }

        global.push_back(extracted);
        pushdata();

        std::string json = createjsonfile();
        response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: " + std::to_string(json.size()) + "\r\n"
            "Connection: close\r\n"
            "\r\n" +
            json;
    }
}
