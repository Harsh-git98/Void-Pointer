#include "cipher.h"
#include <cctype>

const std::string KEY = "HARSH";

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
