//控制流图
#include <iostream>
#include <memory>
#include <vector>
#include <map>
#include <algorithm>
#include <string>
#include <cstring>
using namespace std;
const int N = 1e7+10, M = 1e5+10;
bool used[M];
bool DCE[N];
string S[N];
map<string,int> B;
int idx=1;
int blocknum = 0;
struct node
{
    string name;
    int st,ed;
    int l,r;
}block[M];
vector<int> st;
vector<int> v[M];
pair<string,string> br(string s);
string pass(string s);
void dfs(int x)
{
    used[x]=1;
    for(auto y:v[x])
    {
        if(!used[y]) dfs(y);
    }
}
pair<string,string> br(string s)
{
    int x1=0,x2=0;
    for(int i=0;i<s.size();i++)
    {
        if(s[i]==',')
        {
            if(x1==0) x1=i;
            else x2=i;
        }
    }
    return {s.substr(x1+1,x2-x1-1),s.substr(x2+1)};
}

string pass(string s)
{
    for(int i=0;i<s.size();i++)
    {
        if(s[i]=='\n')
        {
            idx++;
            continue;
        }
        S[idx]+=s[i];
    }
    idx--;

    for(int i=1;i<=idx;i++)
    {
        int len=S[i].size();
        if(S[i][len-1]==':')//基本块入口
        {
            B[S[i].substr(0,len-1)] = ++blocknum;
            if(S[i].substr(0,6)=="%entry") st.push_back(blocknum);
            block[blocknum].name=S[i].substr(0,len-1);
            block[blocknum].st=i;
        }
    }
    blocknum = 0;
    for(int i=1;i<=idx;i++)
    {
        if(S[i].substr(0,3) == "ret")
        {
            blocknum++;
            block[blocknum].ed=i;
            block[blocknum].l=0;
            block[blocknum].r=0;
        }
        else if(S[i].substr(0,4) == "jump")
        {
            blocknum++;
            block[blocknum].ed=i;
            block[blocknum].l=B[S[i].substr(5)];
            block[blocknum].r=0;
        }
        else if(S[i].substr(0,2) == "br")
        {
            blocknum++;
            block[blocknum].ed=i;
            block[blocknum].l=B[br(S[i]).first];
            block[blocknum].r=B[br(S[i]).second];
        }
    }

    for(int i=1;i<=blocknum;i++)
    {
        if(block[i].l)
        {
            v[i].push_back(block[i].l);
        }
        if(block[i].r)
        {
            v[i].push_back(block[i].r);
        }
    }

    for(auto x:st)
    {
        dfs(x);
    }
    for(int i=1;i<=blocknum;i++)
    {
        if(!used[i])//不可达基本块
        {
            for(int j=block[i].st;j<=block[i].ed;j++)
            {
                DCE[j]=1;//死代码
            }
        }
    }
    string ans="";
    for(int i=1;i<=idx;i++)
    {
        if(!DCE[i]) ans+=S[i]+"\n";
    }
    return ans;
    
    
}
