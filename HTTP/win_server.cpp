#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstring>

#pragma comment(lib, "ws2_32.lib")

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

    while (true) {
        new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
        if (new_socket == INVALID_SOCKET) {
            std::cerr << "Accept failed\n";
            break;
        }

        char buffer[8192] = {0};
        int bytes_received = recv(new_socket, buffer, sizeof(buffer)-1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::cout << "Received request:\n" << buffer << "\n";
        }

       std::string body = R"(Harsh Ranjan

 Howrah, West Bengal |  +91 980-159-8891 |  shriharshranjangupta@gmail.com
  https://github.com/Harsh-git98 | https://www.linkedin.com/in/harshrjn/
=====
# Education
B.Tech in Information Technology, IIEST Shibpur — CGPA: 8.18/10 (2022-2026)
Higher Secondary (BSEB), H.D. Jain College — 89.6% (2021-2022)
Secondary (CBSE), DAV Public School — 95.8% (2019-2020)

=====

Work Experience

App Developer — Repromptt (Jun 2025 - Aug 2025)
Built a production-ready mobile app (React Native, Express.js, MongoDB) deployed on Render for scaling to 100k+ users.
Integrated RevenueCat + Stripe webhooks for subscription management.
Designed authentication flows, telemetry, and REST APIs.

Frontend Developer — Lightscline AI (Jun 2024 - Jul 2024)
Rebuilt the company website with optimized performance, responsiveness, and modern UX.
Implemented JavaScript interactions + CSS animations and improved cross-browser compatibility.

Winter Research Intern — NIT Patna (Dec 2023 - Jan 2024)
Researched EWMA algorithms for IoT heartbeat anomaly detection.
Implemented multiple variants in Python (NumPy, Pandas) and validated tradeoffs on real datasets.

=====

Skills
Languages: C++, C, JavaScript (ES6+), Python (Basic), SQL
Frameworks: React.js, Node.js, Express.js, React Native, MongoDB
Tech & Tools: MERN, GenAI (OpenAI API, LangChain), Git, Docker, REST APIs, Stripe, RevenueCat, Render
Databases: MongoDB, MySQL
Coursework: DSA, OS, DBMS, OOP, CN, ML

=====

Projects
Research Paper Search Tool — ReactJS, MongoDB, FastAPI, Jupyter (KNN over embeddings for keyword-based retrieval).
Splitkaro MVP — Hackathon project, React Native + Generative AI (Google Gemini for expense parsing).
Trie Dictionary CLI App — C++ OOP project with efficient word lookups; packaged as .exe.

=====

Positions of Responsibility
Assistant General Secretary, EDC (2023-Present)
Technical Team Member, DEBSOC (2023-Present)
Media Lead, CODEIIEST (2023-Present)

=====

Achievements
LeetCode: 800+ solved (600+ Medium)
Codeforces: Max Rating 1239 (Pupil)
2nd Place: Instruo Hackathon (Sears India, 2023)
3rd Place: Revelation Hackathon (2025)
Top 1% IOQM Merit Certificate (2022)

=====

Extra-Curriculars
Canva design, video editing, debating, basketball.
)";

    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "Connection: close\r\n"
        "\r\n" +
        body;
        send(new_socket, response.c_str(), response.size(), 0);
        closesocket(new_socket);
    }

    closesocket(server_fd);
    WSACleanup();
    return 0;
}
