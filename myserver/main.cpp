#include "server.h"
#include <iostream>

int main() {
    std::cout << "Server running on http://localhost:8080\n";
    start_server(8080);
    return 0;
}
