#include <bits/stdc++.h>
#include <openssl/sha.h>
using namespace std;

int HASHC= 8;

class BloomFilter {
private:
    vector<vector<uint8_t>> bloom;
    int rows, cols;

    string sha256(const string &input) const {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256((const unsigned char*)input.c_str(), input.size(), hash);

        stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
            ss << hex << setw(2) << setfill('0') << (int)hash[i];
        return ss.str();  // 64 hex chars
    }

    // Convert hex hash to a numeric index
    uint64_t hashToNum(const string &hash) const {
        uint64_t num = 0;
        for (int i = 0; i < 16; i++) { // use first 16 hex chars (64 bits)
            char c = toupper(hash[i]);
            int val = isdigit(c) ? c - '0' : 10 + (c - 'A');
            num = (num << 4) | val;
        }
        return num;
    }

public:
   
    BloomFilter(int rows_ = 131072, int cols_ = 64)
        : rows(rows_), cols(cols_), bloom(rows_, vector<uint8_t>(cols_, 0)) {}

    void add(const string &key) {
        for (int i = 0; i < HASHC; i++) {
            string salted = key + to_string(i);
            string hash = sha256(salted);
            uint64_t num = hashToNum(hash);

            int row = num % rows;
            int col = (num / rows) % cols; // distribute bits across columns

            bloom[row][col] = 1;
        }
    }

    bool possiblyContains(const string &key) const {
        for (int i = 0; i < HASHC; i++) {
            string salted = key + to_string(i);
            string hash = sha256(salted);
            uint64_t num = hashToNum(hash);

            int row = num % rows;
            int col = (num / rows) % cols;

            if (bloom[row][col] == 0)
                return false;  // definitely not present
        }
        return true;  // possibly present
    }

    double fillRatio() const {
        uint64_t ones = 0;
        uint64_t total = (uint64_t)rows * cols;
        for (auto &r : bloom)
            for (auto x : r)
                if (x) ones++;
        return (double)ones / total;
    }
};

int main() {
    BloomFilter bf;  


    for (int i = 0; i < 1000000; i=i+2)
        bf.add("user_" + to_string(i));

    cout << "Fill Ratio: " << fixed << setprecision(2)
         << bf.fillRatio() * 100 << "%\n";

    // Read input string
    while(1){
    string str;
    cout<<"enter string\n";
    cin >> str;

    cout << str << ": " << bf.possiblyContains(str) << endl;
    }
    return 0;
}
