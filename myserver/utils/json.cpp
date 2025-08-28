#include "json.h"
#include "../storage/persistence.h"
#include <sstream>
#include <iostream>

std::string escapeForJson(const std::string &s) {
    std::string out;
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

std::string createjsonfile() {
    fetchdata();
    std::string res = "[";
    for (size_t i = 0; i < global.size(); i++) {
        std::string safeText = escapeForJson(global[i]);
        res += "{ \"text\": \"" + safeText + "\" }";
        if (i != global.size() - 1) {
            res += ",";
        }
    }
    res += "]";
    std::cout << res << std::endl;
    return res;
}
