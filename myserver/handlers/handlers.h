#pragma once
#include <string>

void handle_GET(std::string route, std::string &response);
void handle_POST(std::string route, std::string &response, std::string body);
void handle_PUT(std::string route, std::string &response);
void handle_DELETE(std::string route, std::string &response);
