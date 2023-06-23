#include <iostream>
#include <memory>
#include <vector>
#include <map>
#include <algorithm>
#include "ast.h"
using namespace std;

int var_global_init = 0;
int var_local_init = 0;




void flatten(varlist init,vector<int> v,vector<string> &flat)
{
    int num_sum = 0, should_sum = Compute_len(v); // 已经初始化的个数；需要初始化的个数
    int baseline = v[v.size()-1]; // 最后一维
    auto list = init.v;

    for(int i = 0; i < list.size(); i++)
    {
        if(list[i].single)
        { // 是单个
            num_sum++;
            flat.push_back(list[i].num);
        }
        else
        { // 新的初始化列表
            if(num_sum % baseline != 0)//默认不会出现这种情况
            {
                cout << "没对齐！！！" << endl;
                exit(1);
            }
            vector<int> new_boundary;
            new_boundary.assign(v.begin() + Align(v, num_sum, baseline), v.end());
            num_sum += Compute_len(new_boundary);
            flatten(list[i], new_boundary, flat);
        }
    }
    // 最后补充0
    for(; num_sum < should_sum; num_sum++) flat.push_back("0");
    return ;
}

void flatten(constlist init,vector<int> v,vector<int> &flat)
{
    int num_sum = 0, should_sum = Compute_len(v); // 已经初始化的个数；需要初始化的个数
    int baseline = v[v.size()-1]; // 最后一维
    auto list = init.v;

    for(int i = 0; i < list.size(); i++)
    {
        if(list[i].single)
        { // 是单个
            num_sum++;
            flat.push_back(list[i].num);
        }
        else
        { // 新的初始化列表
            if(num_sum % baseline != 0)//默认不会出现这种情况
            {
                cout << "没对齐！！！" << endl;
                exit(1);
            }
            vector<int> new_boundary;
            new_boundary.assign(v.begin() + Align(v, num_sum, baseline), v.end());
            num_sum += Compute_len(new_boundary);
            flatten(list[i], new_boundary, flat);
        }
    }
    // 最后补充0
    for(; num_sum < should_sum; num_sum++) flat.push_back(0);
    return ;
}

int Compute_len(vector<int> boundary){
    int ret = 1;
    for(int i = 0; i < boundary.size(); i++)
        ret *= boundary[i];
    return ret;
}


int Align(vector<int> boundary, int num_sum, int baseline){
    int len = boundary.size(), i = 1, j = len - 1;
    vector<int> tmp;
    tmp.push_back(baseline);
    for(; i < len; i++)
        tmp.push_back(tmp[i-1] * boundary[len - i - 1]);
    for(; j >= 0; j--)
        if(num_sum % tmp[j] == 0){
            if(j == len - 1) // 一上来没有数字就是初始化列表：直接从下一维开始
                return 1;
            break;
        }
    return len - 1 - j;
}

void varglobalinit(string &koopaIR,vector<int> v,vector<int> flat)
{
  if(v.size() == 1)
  {
    koopaIR += to_string(flat[var_global_init++]);
    for(int i = 1; i < v[0]; i++)
        koopaIR += "," + to_string(flat[var_global_init++]);
    return;
  }
  vector<int> tmp;
  tmp.assign(v.begin() + 1, v.end());
  koopaIR += "{";
  varglobalinit(koopaIR,tmp,flat);
  koopaIR += "}";
  for(int i = 1; i < v[0]; i++){
    koopaIR += ",{";
    varglobalinit(koopaIR,tmp,flat);
    koopaIR += "}";
  }
}


void varlocalinit(bool init,string &koopaIR,string s,vector<int> v,vector<string> flat)//
{
  if(v.size() == 1)
  {
    for(int i = 0; i < v[0]; i ++)
    {
      koopaIR += "@list" + to_string(++cnt2) + "=getelemptr " + s + "," + to_string(i) + "\n";
      if(init) koopaIR += "store " + flat[var_local_init++] + ",@list" + to_string(cnt2) + "\n";
      else koopaIR += "store 0,@list" + to_string(cnt2) + "\n";
    }

    return ;
  }
  vector<int> vv;
  vv.assign(v.begin() + 1, v.end());
  for(int i = 0; i < v[0]; i ++)
  {
    koopaIR += "@list" + to_string(++cnt2) + "=getelemptr " + s + "," + to_string(i) + "\n";
    varlocalinit(init,koopaIR,"@list"+to_string(cnt2),vv,flat);
  }
}