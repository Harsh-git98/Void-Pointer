#include<bits/stdc++.h>
using namespace std;

vector<string> global;

string escapeForJson(const string &s) {
    string out;
    for (char c : s) {
        switch (c) {
            case '\"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    // Escape control chars as \u00XX
                    char buf[7];
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    return out;
}

const std::string KEY = "HARSH"; // secret keyword

// Encode using Vigenère cipher
std::string encode(const std::string& text) {
    std::string result = text;
    int keyLen = KEY.length();
    for (size_t i = 0, j = 0; i < result.length(); ++i) {
        if (isalpha(result[i])) {
            char base = islower(result[i]) ? 'a' : 'A';
            int shift = toupper(KEY[j % keyLen]) - 'A';
            result[i] = (result[i] - base + shift) % 26 + base;
            ++j;
        }
    }
    return result;
}

// Decode using Vigenère cipher
std::string decode(const std::string& text) {
    std::string result = text;
    int keyLen = KEY.length();
    for (size_t i = 0, j = 0; i < result.length(); ++i) {
        if (isalpha(result[i])) {
            char base = islower(result[i]) ? 'a' : 'A';
            int shift = toupper(KEY[j % keyLen]) - 'A';
            result[i] = (result[i] - base - shift + 26) % 26 + base;
            ++j;
        }
    }
    return result;
}

void fetchdata() {
    std::ifstream file("save.txt");
    if (!file.is_open()) {
        std::cerr << "Failed to open file\n";
        return;
    }

    global.clear();
    std::string line;
    while (std::getline(file, line)) {
        global.push_back(decode(line));  // decode while reading
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
        file << encode(global[i]) << "\n"; // encode while writing
    }

    file.close();
}


string createjsonfile()
{
    fetchdata();
    string res = "[";
    for (size_t i = 0; i < global.size(); i++)
    {
        string safeText = escapeForJson(global[i]);
        res += "{ \"text\": \"" + safeText + "\" }";
        if (i != global.size() - 1) {
            res += ","; // add comma between objects
        }
    }
    res += "]";
    cout<<res<<endl;
    return res;
}


std::string loadFile(const std::string& filename) {
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
          string html = loadFile("index.html");
          response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: " + std::to_string(html.size()) + "\r\n"
            "Connection: close\r\n"
            "\r\n" +
            html;

    }
    else if(route=="/api")
    {
        string json = createjsonfile();
        response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: " + std::to_string(json.size()) + "\r\n"
        "Connection: close\r\n"
        "\r\n" +
        json;
    }
    else if(route=="/favicon.ico") {
        response =
        "HTTP/1.1 204 No Content\r\n"
        "Connection: close\r\n\r\n";
        return;
    }
    else{
        string html = loadFile("not.html");
          response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: " + std::to_string(html.size()) + "\r\n"
            "Connection: close\r\n"
            "\r\n" +
            html;
    }
    
}


void handle_POST(string route, string &response, string body)
{
    cout<<"route: "<<route<<endl;
    cout<<"body: "<<body<<endl;
    if (route == "/api")
    {
        string extracted = body;

       
        if (!extracted.empty() && extracted.front() == '"' && extracted.back() == '"')
        {
            extracted = extracted.substr(1, extracted.size() - 2);
        }
        if(extracted.back()=='\n' ||extracted.back()=='\r')
        {
            extracted.pop_back();
        }

       
        global.push_back(extracted);
        pushdata();
         string json = createjsonfile();
        response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: " + std::to_string(json.size()) + "\r\n"
        "Connection: close\r\n"
        "\r\n" +
        json;
        
    }

}


void handle_PUT(string route, string &response)
{

}


void handle_DELETE(string route, string &response)
{
     cout<<"route: "<<route<<endl;
    

     if (route.rfind("/api", 0) == 0)
     {
        string ans="";
        bool f=0;
        for(int i=0;i<route.length();i++)
        {
            if(route[i]=='=')
            {
                f=1;
            }
            if(f==1)
            {
                if(route[i]<='9' && route[i]>='0')
                ans+=route[i];
            }
        }
        int index= stoi(ans);

        global.erase(global.begin() + index);
        pushdata();


          response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: " "\r\n"
        "Connection: close\r\n";

     }

}
