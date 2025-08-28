#include "handlers.h"
#include "../storage/persistence.h"
#include <iostream>

void handle_DELETE(std::string route, std::string &response) {
    std::cout << "route: " << route << std::endl;

    if (route.rfind("/api", 0) == 0) {
        std::string ans = "";
        bool f = 0;
        for (int i = 0; i < route.length(); i++) {
            if (route[i] == '=') f = 1;
            if (f && (route[i] >= '0' && route[i] <= '9'))
                ans += route[i];
        }
        int index = stoi(ans);

        global.erase(global.begin() + index);
        pushdata();

        response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: 2\r\n"
            "Connection: close\r\n"
            "\r\n"
            "{}";
    }
}
