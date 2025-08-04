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
    int n;
    while (true)
{
    cout << "Enter the command" << endl;
    cout << "1 -> insertion" << endl;
    cout << "2 -> search" << endl;
    cout << "0 -> exit" << endl;

    cin >> n;
    
    if (n == 0) {
        break;  // 🛑 exit the loop
    }
    else if (n == 1)
    {
        string word;
        string meaning;
        cout << "Enter the word" << endl;
        cin >> word;
         cin.ignore();
          transform(word.begin(), word.end(), word.begin(), ::tolower);
        cout << "Enter its meaning" << endl;
        getline(cin, meaning);
         transform(meaning.begin(), meaning.end(), meaning.begin(), ::tolower);
        obj->insert(word, meaning);
    }
    else if (n == 2)
    {
        string searcher;
        cout << "Enter the word" << endl;
        cin >> searcher;
         transform(searcher.begin(), searcher.end(), searcher.begin(), ::tolower);
        cout << "meaning: " << obj->search(searcher) << endl;
    }
    else {
        cout << "Invalid input. Please try again." << endl;
    }
}
}