#include<bits/stdc++.h>
using namespace std;


std::string loadHtmlFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return "<h1>404 Not Found</h1>";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void handle_GET(string route, string &response)
{
    cout<<"route: "<<route<<endl;
    // DESIGNING ROUTING 
    if(route=="/")
    {
          string html = loadHtmlFile("index.html");
          response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: " + std::to_string(html.size()) + "\r\n"
            "Connection: close\r\n"
            "\r\n" +
            html;

    }
    if(route=="/favicon.ico") {
        response =
        "HTTP/1.1 204 No Content\r\n"
        "Connection: close\r\n\r\n";
        return;
    }
    else{
         response =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: 13\r\n"
            "Connection: close\r\n"
            "\r\n"
            "<h1>404</h1>";
    }
    
}


void handle_POST(string route, string &response)
{

}


void handle_PUT(string route, string &response)
{

}


void handle_DELETE(string route, string &response)
{

}
