#include <iostream>
#include <memory>
#include <vector>
#include <map>
#include <algorithm>
#include "ast.h"
using namespace std;



void dfs(string &koopaIR,string s,vector<int> v)
{
  if(v.size() == 1)
  {
    for(int i = 0; i < v[0]; i ++)
    {
      koopaIR += "@" + to_string(++cnt2) + " = getelemptr " + s + ", " + to_string(i) + "\n";
      koopaIR += "  store 0, @" + to_string(cnt2) + "\n";
    }

    return ;
  }
  vector<int> vv;
  vv.assign(v.begin() + 1, v.end());
  for(int i = 0; i < v[0]; i ++)
  {
    koopaIR += "@" + to_string(++cnt2) + " = getelemptr " + s + ", " + to_string(i) + "\n";
    dfs(koopaIR,"@"+to_string(cnt2),vv);
  }
  
}