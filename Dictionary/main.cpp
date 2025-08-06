#include<bits/stdc++.h>
#include <string>
#include <cctype>
#include "dictionary_data.h"
#include "trie.cpp"
using namespace std;
/*

This project is about implementating a dictionary from in c++ using trie Data structure.
*/

void printBanner() {
    cout << R"(
 __        __   _                            _         
 \ \      / /__| | ___ ___  _ __ ___   ___  | |_ ___   
  \ \ /\ / / _ \ |/ __/ _ \| '_ ` _ \ / _ \ | __/ _ \  
   \ V  V /  __/ | (_| (_) | | | | | |  __/ | || (_) | 
    \_/\_/ \___|_|\___\___/|_| |_| |_|\___|  \__\___/  

                   CLI Dictionary!
                   
        Developer: https://github.com/Harsh-git98
        Type a word to get its meaning instantly.
        -----------------------------------------
    )" << endl;
    cout<<endl;
}

int main()
{
    Trie* obj = new Trie();
    
     for (int i = 0; i < dictionary_size; ++i) {
        obj->insert(dictionary_entries[i].word, dictionary_entries[i].meaning);
    }
    printBanner();
    // REPL for searching
    
    while (true) {
        cout << "Enter word to search (or Exit: type '0'): ";
        string searchWord;
        cin >> searchWord;
        transform(searchWord.begin(), searchWord.end(), searchWord.begin(), ::tolower);
        if (searchWord == "0") break;
        bool f=0;
        for(int i=0;i<searchWord.size();i++)
        {
            char s=searchWord[i];
            if(!((s<='Z' &&s>='A') || (s<='z' && s>='a')))
            {
                cout <<"Invalid Word"<<endl;
                f=1;
                break;
            }
        }
        if(f==1)
        {
            continue;
        }

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