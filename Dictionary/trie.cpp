#include<bits/stdc++.h>
using namespace std;



// TRIE Functions
class TrieNode{
    public:
    char ch;
    TrieNode* children[26];
    bool terminal=0;
    string value="";
    TrieNode(char c)
    {
        ch =c;
        for(int i=0;i<26;i++)
        {
            children[i]=NULL;
        }
        terminal = false;
    }
};

class Trie{
    public:
    TrieNode* root;

    Trie()
    {
        root= new TrieNode('0');
    }

    void recurinsert(TrieNode* root, string &word, int i, string& meaning)
    {
        if(i==word.length())
        {
            root->terminal= true;
            root->value=meaning;
            return;
        }
        TrieNode* child;

        int ind = word[i]-'a';

        if(root->children[ind]!=NULL)
        {
            child = root->children[ind];
        }
        else{
            child = new TrieNode(word[i]);
            root->children[ind]=child;
        }
        recurinsert(child,word,i+1,meaning);
    }

    void insert(string word, string meaning)
    {
        recurinsert(root, word,0,meaning);
    }

    string recursearch(TrieNode* root, string&word, int i)
    {
        if(i>=word.length())
        {
            if(root->terminal)
            {
                return root->value;
            }
            return "not_found";
        }
        int ind = word[i]-'a';
        if(root->children[ind]!=NULL)
        {
            return recursearch(root->children[ind], word,i+1);
        }
        else{
            return "Not_found";
        }
    }
    string search(string word)
    {
        return recursearch(root, word,0);
    }
};