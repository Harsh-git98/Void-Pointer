#include<bits/stdc++.h>
#include <string>
#include <cctype>
#include "trie.cpp"
using namespace std;
/*

This project is about implementating a dictionary from in c++ using trie Data structure.
*/




int main()
{
    Trie* obj = new Trie();
    ifstream fin("words.txt");
    if (!fin.is_open()) {
        cerr << "Error: Could not open word.txt" << endl;
        return 1;
    }

    string word, meaning;
    while (getline(fin, word)) {
        if (getline(fin, meaning)) {


            transform(word.begin(), word.end(), word.begin(), ::tolower);
            transform(meaning.begin(), meaning.end(), meaning.begin(), ::tolower);

            obj->insert(word, meaning);
        } else {
            cerr << "Warning: No meaning found for word: " << word << endl;
        }
    }

    fin.close();

    // REPL for searching
    string searchWord;
    while (true) {
        cout << "Enter word to search (or type '0'): ";
        cin >> searchWord;
        transform(searchWord.begin(), searchWord.end(), searchWord.begin(), ::tolower);
        if (searchWord == "0") break;

        string result = obj->search(searchWord);
        if (result.empty()) {
            cout << "Word not found.\n";
        } else {
            cout << "Meaning: " << result << "\n";
        }
    }

    delete obj;
    return 0;
   
}