//控制流图
#include <iostream>
#include <memory>
#include <vector>
#include <map>
#include <algorithm>
#include <string>
#include <cstring>
#include <cassert>
#include <set>
using namespace std;
const int N = 1e6+10, M = 1e5+10;
bool used[M];
bool DCE[N];
string S[N];
map<string,int> B;//块的映射
int idx = 1;//行数
int blocknum = 0;//块的个数
int funcnum = 0;//函数个数
struct Func
{
    string s;
    int st,ed;
    vector<int> B;
}func[M];
struct Block
{
    string name;
    int st,ed;
    int l,r;
    set<string> use,def,in,out;
}block[M];
struct Inst
{
    string s;
    string type;
    string src,dest;//针对load store
    string op1,op2;//针对运算操作
    set<string> use,def,in,out;
}inst[N];
vector<int> st;//函数entry块
vector<int> v[M];//邻接表存图
set<string> globalvar;
pair<string,string> br(string s);
string pass(string s);
void DFG();
bool assign(string s);
void dfs(int x);
set<string> paramlist(string s);

set<string> paramlist(string s)
{
    set<string> ret;
    string ans="";
    for(int i=0;i<s.size();i++)
    {
        if(s[i]==',') ret.insert(ans),ans="";
        else ans+=s[i];
    }
    if(ans!="") ret.insert(ans);
    return ret;
}


bool assign(string s)
{
    for(int i=0;i<s.size();i++) if(s[i]=='=') return true;
    return false;
}
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
    //分割成一行一行
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
        if(S[i].substr(0,6)=="global") 
        {
            int x1;
            for(int j=0;j<len;j++)
            {
                if(S[i][j]=='=') x1=j;
            }
            globalvar.insert(S[i].substr(7,x1-7));
        }
        if(S[i].substr(0,3)=="fun")
        {
            funcnum++;
            func[funcnum].st=i;
            func[funcnum].s=S[i];
        }
        if(S[i]=="}")
        {
            func[funcnum].ed=i;
        }
        if(S[i][len-1]==':')//基本块入口
        {
            B[S[i].substr(0,len-1)] = ++blocknum;
            func[funcnum].B.push_back(blocknum);
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
    DFG();
    string ans="";
    for(int i=1;i<=idx;i++)
    {
        if(!DCE[i]) ans+=S[i]+"\n";
    }
    return ans;
    
}

void DFG()
{
    for(int i=1;i<=blocknum;i++)
    {
        for(int j=block[i].st+1;j<=block[i].ed;j++)
        {
            inst[j].s=S[j];
            if(assign(S[j]))// a=op b,c   a=load b
            {
                int x1,x2,x3;
                for(int z=0;z<S[j].size();z++)
                {
                    if(S[j][z]=='=') x1=z;//等号位置
                }
                if(S[j].substr(x1+1,4)=="load")
                {
                    for(int z=0;z<S[j].size();z++)
                    {
                        if(S[j][z]==' ') x2=z;
                    }
                    inst[j].type="load";
                    inst[j].src=S[j].substr(x2+1);
                    inst[j].dest=S[j].substr(0,x1);
                    inst[j].use.insert(S[j].substr(x2+1));
                    inst[j].def.insert(S[j].substr(0,x1));
                }
                else if(S[j].substr(x1+1,5)=="alloc")
                {
                    for(int z=0;z<S[j].size();z++)
                    {
                        if(S[j][z]==' ') x2=z;
                    }
                    inst[j].type="alloc";
                    inst[j].src=S[j].substr(0,x1);
                    inst[j].dest=S[j].substr(x2+1);
                }
                else if(S[j].substr(x1+1,4)=="call")
                {
                    for(int z=0;z<S[j].size();z++)
                    {
                        if(S[j][z]=='(') x2=z;
                        if(S[j][z]==')') x3=z;
                    }
                    inst[j].type="call";
                    inst[j].dest=S[j].substr(0,x1);
                    inst[j].def.insert(S[j].substr(0,x1));
                    inst[j].use=paramlist(S[j].substr(x2+1,x3-x2-1));
                }
                else
                {
                    for(int z=0;z<S[j].size();z++)
                    {
                        if(S[j][z]==' ') x2=z;
                        if(S[j][z]==',') x3=z;
                    }
                    inst[j].type=S[j].substr(x1+1,x2-x1-1);
                    inst[j].dest=S[j].substr(0,x1);
                    inst[j].op1=S[j].substr(x2+1,x3-x2-1);
                    inst[j].op2=S[j].substr(x3+1);
                    inst[j].use.insert(S[j].substr(x3+1));
                    inst[j].use.insert(S[j].substr(x2+1,x3-x2-1));
                    inst[j].def.insert(S[j].substr(0,x1));
                }
            }
            else if(S[j].substr(0,5)=="store")//store a,b
            {
                inst[j].type="store";
                int x1,x2;
                for(int z=0;z<S[j].size();z++)
                {
                    if(S[j][z]==' ') x1=z;
                    if(S[j][z]==',') x2=z;
                }
                inst[j].src=S[j].substr(x1+1,x2-x1-1);
                inst[j].dest=S[j].substr(x2+1);
                inst[j].use.insert(S[j].substr(x1+1,x2-x1-1));
                inst[j].def.insert(S[j].substr(x2+1));
            }
            else if(S[j].substr(0,4)=="call")
            {
                int x1,x2;
                for(int z=0;z<S[j].size();z++)
                {
                    if(S[j][z]=='(') x1=z;
                    if(S[j][z]==')') x2=z;
                }
                inst[j].type="call";
                inst[j].use=paramlist(S[j].substr(x1+1,x2-x1-1));
            }
            else if(S[j].substr(0,3) == "ret")
            {
                inst[j].type="ret";
                inst[j].dest=S[j].substr(4);
                inst[j].use.insert(S[j].substr(4));
                    
            }
            else if(S[j].substr(0,4) == "jump")
            {
                inst[j].type="jump";
                inst[j].dest=S[j].substr(5);
            }
            else if(S[j].substr(0,2) == "br")
            {
                int x1=0,x2=0,x3=0;
                for(int z=0;z<S[j].size();z++)
                {
                    if(S[j][z]==' ') x1=z;
                    else if(S[j][z]==',')
                    {
                        if(x2) x3=z;
                        else x2=z;
                    }
                }
                inst[j].type="br";
                inst[j].src=S[j].substr(x1+1,x2-x1-1);
                inst[j].op1=S[j].substr(x2+1,x3-x2-1);
                inst[j].op2=S[j].substr(x3+1);
                inst[j].use.insert(S[j].substr(x1+1,x2-x1-1));
            }
            else 
            {
                cerr<<"impossible???????";
                assert(true);
            }
        }
    }

    for(int i=1;i<=blocknum;i++)//获得每个块的use,def
    {
        for(int j=block[i].st+1;j<=block[i].ed;j++)
        {
            for(auto x:inst[j].use)
            {
                if(block[i].def.find(x)==block[i].def.end()) block[i].use.insert(x);
            }
            for(auto x:inst[j].def)
            {
                block[i].def.insert(x);
            }
        }
    }

    bool changed=true;//得到每个块in out集合
    while(changed)
    {
        changed=false;
        for(int i=1;i<=blocknum;i++)
        {
            set<string> newin,newout;
            for(auto suc:v[i])//后继
            {
                for(auto x:block[suc].in) newout.insert(x);
            }
            block[i].out=newout;
            for(auto x:block[i].use) newin.insert(x);
            for(auto x:block[i].out)
            {
                if(block[i].def.find(x)==block[i].def.end()) newin.insert(x);
            }
            if(newin!=block[i].in) 
            {
                changed=true;
                block[i].in=newin;
            }
        }
    }

    for(int i=1;i<=blocknum;i++)
    {
        for(int j=block[i].ed;j>block[i].st;j--)
        {
            //先得到每个语句的out
            if(j==block[i].ed)
            {
                inst[j].out=block[i].out;
                for(auto x:globalvar) inst[j].out.insert(x);
            }
            else
            {
                inst[j].out=inst[j+1].in;
            }

            //然后得到每个语句的in
            if(inst[j].type=="getelemptr")
            {
                inst[j].in=inst[j].out;
                for(auto x:inst[j].use) inst[j].in.insert(x);
            }
            else if(inst[j].type=="getptr")
            {
                inst[j].in=inst[j].out;
                for(auto x:inst[j].use) inst[j].in.insert(x);
            }
            else if(inst[j].type=="load")
            {
                string dest=inst[j].dest;
                if(inst[j].out.find(dest)==inst[j].out.end())
                {
                    DCE[j]=1;
                    inst[j].in=inst[j].out;
                }
                else 
                {
                    inst[j].in=inst[j].use;
                    for(auto x:inst[j].out)
                    {
                        if(inst[j].def.find(x)==inst[j].def.end()) inst[j].in.insert(x);
                    }
                }
            }
            else if(inst[j].type=="alloc")
            {
                inst[j].in=inst[j].out;
                //if(inst[j].out.find(inst[j].src)==inst[j].out.end()) DCE[j]=1;
            }
            else if(inst[j].type=="call")
            {
                if(inst[j].dest=="")//void
                {
                    inst[j].in=inst[j].out;
                    for(auto x:inst[j].use) inst[j].in.insert(x);
                }
                else
                {
                    
                    inst[j].in=inst[j].use;
                    for(auto x:inst[j].out)
                    {
                        if(inst[j].def.find(x)==inst[j].def.end()) inst[j].in.insert(x);
                    }
                    
                }
                
            }
            else if(inst[j].type=="store")
            {
                string src=inst[j].src,dest=inst[j].dest;
                if(dest[1]=='L'||dest.substr(1,4)=="list")//数组变量
                {
                    inst[j].in=inst[j].out;
                    inst[j].in.insert(src);
                }
                else//普通变量
                {
                    if(inst[j].out.find(dest)==inst[j].out.end())
                    {
                        DCE[j]=1;
                        inst[j].in=inst[j].out;
                    }
                    else
                    {
                        inst[j].in=inst[j].use;
                        for(auto x:inst[j].out)
                        {
                            if(inst[j].def.find(x)==inst[j].def.end()) inst[j].in.insert(x);
                        }
                    }
                }
            }
            else if(inst[j].type=="ret")
            {
                inst[j].in=inst[j].use;
                for(auto x:inst[j].out) inst[j].in.insert(x);
            }
            else if(inst[j].type=="jump")
            {
                inst[j].in=inst[j].out;
            }
            else if(inst[j].type=="br")
            {
                inst[j].in=inst[j].use;
                for(auto x:inst[j].out) inst[j].in.insert(x);
            }
            else//二元运算
            {
                string dest=inst[j].dest;
                if(inst[j].out.find(dest)==inst[j].out.end())
                {
                    DCE[j]=1;
                    inst[j].in=inst[j].out;
                }
                else 
                {
                    inst[j].in=inst[j].use;
                    for(auto x:inst[j].out)
                    {
                        if(inst[j].def.find(x)==inst[j].def.end()) inst[j].in.insert(x);
                    }
                }
            }



        }
    }
}