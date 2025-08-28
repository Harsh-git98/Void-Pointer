#include "handlers.h"
#include "../utils/json.h"
#include "../utils/file.h"
#include "../storage/persistence.h"
#include <iostream>

void handle_GET(std::string route, std::string &response) {
    std::cout << "route: " << route << std::endl;

    if (route == "/") {
        std::string html = loadFile("index.html");
        response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: " + std::to_string(html.size()) + "\r\n"
            "Connection: close\r\n"
            "\r\n" +
            html;

    } else if (route == "/api") {
        std::string json = createjsonfile();
        response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: " + std::to_string(json.size()) + "\r\n"
            "Connection: close\r\n"
            "\r\n" +
            json;

    } else if (route == "/favicon.ico") {
        response =
            "HTTP/1.1 204 No Content\r\n"
            "Connection: close\r\n\r\n";
        return;

    } else {
        std::string html = loadFile("not.html");
        response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: " + std::to_string(html.size()) + "\r\n"
            "Connection: close\r\n"
            "\r\n" +
            html;
    }
}
