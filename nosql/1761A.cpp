#include<bits/stdc++.h>

using namespace std;

int main()
{
    int t;
    cin>>t;
    while(t--)
    {
        int n,a,b;
        cin>>n>>a>>b;

        if(a==n &&b==n)
        {
            cout<<"YES"<<endl;

        }
        else if(n-(a+b)>=2)
        {
            cout<<"YES"<<endl;
        }
        else{
            cout<<"NO"<<endl;
        }

    }
}



/*


1 2 3 6 5 4
1 2 6 3 5 4


*/