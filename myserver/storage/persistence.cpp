#include "persistence.h"
#include "cipher.h"
#include <fstream>
#include <iostream>

std::vector<std::string> global;

void fetchdata() {
    std::ifstream file("save.txt");
    if (!file.is_open()) {
        std::cerr << "Failed to open file\n";
        return;
    }

    global.clear();
    std::string line;
    while (std::getline(file, line)) {
        global.push_back(decode(line));
    }
    file.close();
}

void pushdata() {
    std::ofstream file("save.txt");
    if (!file.is_open()) {
        std::cerr << "Failed to open file\n";
        return;
    }

    for (int i = 0; i < global.size(); i++) {
        file << encode(global[i]) << "\n";
    }

    file.close();
}
