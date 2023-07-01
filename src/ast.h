#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <map>
#include <algorithm>
using namespace std;
//基类
extern int entrynum;
extern int cnt;
extern int cnt2;
extern int STnum;
extern int IFnum;
extern int WHILEnum;
extern int SHORT;
extern int var_global_init;//用于全局数组初始化
extern int var_local_init;//局部数组初始化
extern bool check(string s);
extern map<string,string> globalF;
struct Symbol
{
  string type;
  int value;
};
typedef map<string,Symbol> Table;

struct ST
{
  int num;   //第几个符号表
  Table table;//表项
  ST *fa;//父节点
  vector<ST*> child;//儿子节点
};
extern ST *cur_st;

struct WT//while符号表
{
  int num;
  WT *fa;
};
extern WT *cur_wh;


class varlist//变量数组初始化列表
{
  public:
    bool single;//判断是单值还是{}
    string num;
    vector<varlist> v;
};

class constlist
{
  public:
    bool single;
    int num;
    vector<constlist> v;
};

void flatten(varlist init,vector<int> v,vector<string> &flat);
void flatten(constlist init,vector<int> v,vector<int> &flat);
int Compute_len(vector<int> boundary);
int Align(vector<int> boundary, int num_sum, int baseline);
void varglobalinit(string &koopaIR,vector<int> v,vector<int> flat);
void varlocalinit(bool init,string &koopaIR,string s,vector<int> v,vector<string> flat);



class BaseAST
{
 public:
  virtual ~BaseAST() = default;
  virtual void Dump(string& koopaIR) const = 0;
  virtual string cal(string& koopaIR) const = 0;
  virtual int compute() const = 0;
  virtual varlist computelist1(string& koopaIR){return varlist{};}
  virtual constlist computelist2(string& koopaIR){return constlist{};};
};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST 
{
 public:
  vector<BaseAST*> compunits;
  void Dump(string& koopaIR) const override 
  {
    //std::cout << "CompUnitAST { ";
    //func_def->Dump(koopaIR);
    //std::cout << " }";
  }
  string cal(string& koopaIR) const override
  {
    for (int i = 0; i < compunits.size(); i++)
    { //遍历每一个item
        compunits[i]->cal(koopaIR);
    }
    return "";
  }
  int compute() const override
  {
    return 0;
  }
};

class Decl1AST: public BaseAST//常量
{
    public:
        std::unique_ptr<BaseAST> constdecl;
        void Dump(string& koopaIR) const override
        {
            
        }
        string cal(string& koopaIR) const override
        {
          constdecl->cal(koopaIR);
          return "";
        }
        int compute() const override
        {
          return 0;
        }
        
};

class Decl2AST: public BaseAST//变量
{
    public:
        std::unique_ptr<BaseAST> vardecl;
        void Dump(string& koopaIR) const override
        {
            
        }
        string cal(string& koopaIR) const override
        {
          vardecl->cal(koopaIR);
          return "";
        }
        int compute() const override
        {
          return 0;
        }
};

class ConstDeclAST: public BaseAST
{
    public:
        std::string btype;
        vector<BaseAST*> constdef_;
        void Dump(string& koopaIR) const override
        {
            
        }
        string cal(string& koopaIR) const override
        {
          for (int i = 0; i < constdef_.size(); i++)
          { //遍历每一个const
              constdef_[i]->cal(koopaIR);
          }
          return "";
        }
        int compute() const override
        {
          return 0;
        }
};

class VarDeclAST: public BaseAST
{
    public:
        std::string btype;
        vector<BaseAST*> vardef_;
        void Dump(string& koopaIR) const override
        {
            
        }
        string cal(string& koopaIR) const override
        {
          for (int i = 0; i < vardef_.size(); i++)
          { //遍历每一个item
              vardef_[i]->cal(koopaIR);
          }
          return "";
        }
        int compute() const override
        {
          return 0;
        }
};

class VarDef1AST: public BaseAST//没有初值
{
    public:
        string ident;
        vector<BaseAST*> constexp_;
        
        void Dump(string& koopaIR) const override
        {
            
        }
        string cal(string& koopaIR) const override
        {
          if(constexp_.size() == 0)//非数组
          {
            Symbol symbol = {"undef",0};//??
            cur_st->table[ident] = symbol;
            if(cur_st->num == 0)
            {
              koopaIR += "global @" + ident + "_0=alloc i32,zeroinit\n";
            }
            else
            {
              koopaIR += "@" + ident + "_" + to_string(cur_st->num) + "=alloc i32\n"; 
            }
            return "";
          }
          else//数组无初值，默认0
          {
            string ans = "i32";
            vector<int> v;
            for(int i = constexp_.size() - 1; i >= 0; i --)
            {
              int res = constexp_[i]->compute();
              ans = "[" + ans + "," + to_string(res) + "]";
              v.push_back(res);
            }
            reverse(v.begin(),v.end());
            Symbol symbol = {"varlist",int(constexp_.size())};
            cur_st->table[ident] = symbol;
            if(cur_st->num == 0)//全局
            {
              koopaIR += "global @" + ident + "_0=alloc " + ans + ",zeroinit\n";
            }
            else//局部
            {
              koopaIR += "@" + ident + "_" + to_string(cur_st->num) + "=alloc " + ans + "\n";
              vector<string> flat;//空
              varlocalinit(false,koopaIR,"@"+ident+"_"+to_string(cur_st->num),v,flat);
            }
            return "";

          }
        }
        int compute() const override
        {
          return 0;
        }
};

class VarDef2AST: public BaseAST//有初始值 x = ?
{
    public:
        string ident;
        vector<BaseAST*> constexp_;
        std::unique_ptr<BaseAST> initval;
        void Dump(string& koopaIR) const override
        {
            
        }
        string cal(string& koopaIR) const override
        {
          if(constexp_.size() == 0)//非数组
          {
            Symbol symbol = {"var",0};
            cur_st->table[ident] = symbol;
            if(cur_st->num == 0)//全局变量,有初值,根据语言规范,全局变量的初值是常值表达式
            {
              int ans = initval->compute();
              koopaIR += "global @" + ident + "_0=alloc i32," + to_string(ans) + "\n";
            }
            else
            {
            koopaIR += "@" + ident + "_" + to_string(cur_st->num) + "=alloc i32\n";
            string re = initval->cal(koopaIR);
            if(re[0] == '@')
            {
              koopaIR += "%" + to_string(++cnt) + "=load " + re + "\n";
              re = "%" + to_string(cnt);
            }
            koopaIR += "store " + re + "," + "@" + ident + "_" +to_string(cur_st->num) + "\n";
            }
            return "";
          }
          else//数组
          {
            string ans = "i32";
            vector<int> v;
            for(int i = constexp_.size() - 1; i >= 0; i --)
            {
              int res = constexp_[i]->compute();
              ans = "[" + ans + "," + to_string(res) + "]";
              v.push_back(res);
            }
            reverse(v.begin(),v.end());//翻转为正常维度
            Symbol symbol = {"varlist",int(constexp_.size())};
            cur_st->table[ident] = symbol;
            
            if(cur_st->num == 0)//全局数组   用{{{}}}整体进行赋值
            {
              constlist init = initval->computelist2(koopaIR);//得到完整列表
              vector<int> flat;
              flatten(init,v,flat);//init:列表,v:各个维度的信息,flat:铺平后的一维数组
              //铺平之后，进行递归初始化
              var_global_init = 0;
              koopaIR += "global @" + ident + "_0=alloc " + ans + ",";
              if(init.single == false&&init.v.size() == 0) 
              {
                koopaIR += "zeroinit\n";
                return "";
              }
              koopaIR += "{";
              varglobalinit(koopaIR,v,flat);
              koopaIR += "}\n";
            }
            else//局部数组 采用一个一个赋值的方法
            {
              varlist init = initval->computelist1(koopaIR);//得到完整列表
              vector<string> flat;
              flatten(init,v,flat);//init:列表,v:各个维度的信息,flat:铺平后的一维数组
              for(int i = 0;i < flat.size(); i ++)//flat中的值可能是常数，中间值，变量，如果是变量先将它load到临时值%cnt中
              {
                if(flat[i][0] == '@')
                {
                  koopaIR += "%" + to_string(++cnt) + "=load " + flat[i] + "\n";
                  flat[i] = "%" + to_string(cnt);
                }
              }
            //铺平之后，进行递归初始化
              var_local_init = 0;
              koopaIR += "@" + ident + "_" + to_string(cur_st->num) + "=alloc " + ans + "\n";
              varlocalinit(true,koopaIR,"@"+ident+"_"+to_string(cur_st->num),v,flat);
            }
            return "";
          }
        }
        int compute() const override
        {
          return 0;
        }
};

class InitVal1AST: public BaseAST
{
    public:
        std::unique_ptr<BaseAST> exp;
        void Dump(string& koopaIR) const override
        {
            
        }
        string cal(string& koopaIR) const override
        {
          return exp->cal(koopaIR);
        }
        int compute() const override
        {
          return exp->compute();
        }
        varlist computelist1(string &koopaIR) override
        {
          varlist re;
          re.single = true;//单值,可能是变量,临时值,常数
          re.num = exp->cal(koopaIR);
          return re;
        }
        constlist computelist2(string &koopaIR) override
        {
          constlist re;
          re.single = true;//单值,可能是变量,临时值,常数
          re.num = exp->compute();
          return re;
        }
};

class InitVal2AST: public BaseAST
{
    public:
        void Dump(string& koopaIR) const override
        {
            
        }
        string cal(string& koopaIR) const override
        {
          return "";
        }
        int compute() const override
        {
          return 0;
        }
        varlist computelist1(string &koopaIR) override
        {
          varlist re;
          re.single = false;//空列表{}
          return re;
        }
        constlist computelist2(string &koopaIR) override
        {
          constlist re;
          re.single = false;//空列表{}
          return re;
        }
};

class InitVal3AST: public BaseAST
{
    public:
        vector<BaseAST*> initval_;
        void Dump(string& koopaIR) const override
        {
            
        }
        string cal(string& koopaIR) const override
        {
          return "";
        }
        int compute() const override
        {
          return 0;
        }
        varlist computelist1(string &koopaIR) override
        {
          varlist re;
          re.single = false;
          for(int i = 0; i <initval_.size(); i ++)
          {
            re.v.push_back(initval_[i]->computelist1(koopaIR));
          }
          return re;
        }
        constlist computelist2(string &koopaIR) override
        {
          constlist re;
          re.single = false;
          for(int i = 0; i <initval_.size(); i ++)
          {
            re.v.push_back(initval_[i]->computelist2(koopaIR));
          }
          return re;
        }
};

/*
class BTypeAST: public BaseAST
{
    public:
        string type;
        void Dump(string& koopaIR) const override
        {
            
        }
        string cal(string& koopaIR) const override
        {
          if(type == "int") koopaIR += ": i32 ";
          return "";
        }
        int compute() const override
        {
          return 0;
        }
};*/

class ConstDefAST: public BaseAST//常量定义，无任何语句产生
{
    public:
        string ident;
        vector<BaseAST*> constexp_;
        std::unique_ptr<BaseAST> constinitval;
        void Dump(string& koopaIR) const override
        {
            
        }
        string cal(string& koopaIR) const override
        {
          if(constexp_.size() == 0)//单值常量定义
          {
            int ans = constinitval->compute();
            Symbol symbol = {"const",ans};
            cur_st->table[ident] = symbol;
          }
          else//数组常量
          {
            string ans = "i32";
            vector<int> v;
            for(int i = constexp_.size() - 1; i >= 0; i --)
            {
              int res = constexp_[i]->compute();
              ans = "[" + ans + "," + to_string(res) + "]";
              v.push_back(res);
            }
            reverse(v.begin(),v.end());//翻转为正常维度
            Symbol symbol = {"constlist",int(constexp_.size())};
            cur_st->table[ident] = symbol;
            if(cur_st->num == 0)//全局
            {
              constlist init = constinitval->computelist2(koopaIR);//得到完整列表
              vector<int> flat;
              flatten(init,v,flat);//init:列表,v:各个维度的信息,flat:铺平后的一维数组
              //铺平之后，进行递归初始化
              var_global_init = 0;
              koopaIR += "global @" + ident + "_0=alloc " + ans + ", ";
              koopaIR += "{";
              varglobalinit(koopaIR,v,flat);
              koopaIR += "}\n";
            }
            else
            {
              constlist init = constinitval->computelist2(koopaIR);//得到完整列表
              vector<int> flat;
              flatten(init,v,flat);//init:列表,v:各个维度的信息,flat:铺平后的一维数组
              //铺平之后，进行递归初始化
              vector<string> ff;
              for(auto x:flat) ff.push_back(to_string(x));
              var_local_init = 0;
              koopaIR += "@" + ident + "_" + to_string(cur_st->num) + "=alloc " + ans + "\n";
              varlocalinit(true,koopaIR,"@"+ident+"_"+to_string(cur_st->num),v,ff);
            }
          }
          return "";
        }
        int compute() const override
        {
          return 0;
        }
};

class ConstInitVal1AST: public BaseAST
{
    public:
        std::unique_ptr<BaseAST> constexp;
        void Dump(string& koopaIR) const override
        {
            
        }
        string cal(string& koopaIR) const override
        {
          return "";
        }
        int compute() const override
        {
          return constexp->compute();
        }
        constlist computelist2(string &koopaIR) override
        {
          constlist re;
          re.single = true;//单值,可能是变量,临时值,常数
          re.num = constexp->compute();
          return re;
        }
        
};

class ConstInitVal2AST: public BaseAST
{
    public:
        void Dump(string& koopaIR) const override
        {
            
        }
        string cal(string& koopaIR) const override
        {
          return "";
        }
        int compute() const override
        {
          return 0;
        }
        constlist computelist2(string &koopaIR) override
        {
          constlist re;
          re.single = false;
          return re;
        }
};

class ConstInitVal3AST: public BaseAST
{
    public:
        vector<BaseAST*> constinitval_;
        void Dump(string& koopaIR) const override
        {
            
        }
        string cal(string& koopaIR) const override
        {
          return "";
        }
        int compute() const override
        {
          return 0;
        }
        constlist computelist2(string &koopaIR) override
        {
          constlist re;
          re.single = false;
          for(int i = 0; i < constinitval_.size(); i ++)
          {
            re.v.push_back(constinitval_[i]->computelist2(koopaIR));
          }
          return re;
        }
};

class ConstExpAST: public BaseAST
{
    public:
        std::unique_ptr<BaseAST> exp;
        void Dump(string& koopaIR) const override
        {
            
        }
        string cal(string& koopaIR) const override
        {
          return "";
        }
        int compute() const override
        { 
          return exp->compute();
        }
};

// FuncDef 也是 BaseAST
class FuncDef1AST : public BaseAST {//无参数
 public:
  std::string func_type;
  std::string ident;
  std::unique_ptr<BaseAST> block;

  void Dump(string& koopaIR) const override 
  {
    //std::cout << "FuncDefAST { ";
    //koopaIR += "fun @";
    //koopaIR += ident;
    //koopaIR += "(): ";
    //func_type->Dump(koopaIR);
    //std::cout << ", " << ident << ", ";
    //block->Dump(koopaIR);
    //std::cout << " }";
  }
  string cal(string& koopaIR) const override
  {
    globalF[ident] = func_type;
    //koopaIR += "\n" + ident + " " + func_type + "\n";
    STnum = 0;
    koopaIR += "fun @";
    koopaIR += ident;
    koopaIR += "()";
    if(func_type == "int") koopaIR += ":i32";
    koopaIR += "{\n";
    koopaIR += "%entry" + to_string(++entrynum) + ":\n";

    STnum++;
    ST *now_st = new ST;
    now_st->num = STnum;
    now_st->fa = cur_st;
    cur_st = now_st;
    block->cal(koopaIR);
    if(!check(koopaIR)) koopaIR += "ret \n";
    koopaIR += "}\n";
    cur_st = cur_st->fa;
    return "";
  }
  int compute() const override
  {
     return 0;
  }
};

class FuncDef2AST : public BaseAST {//有参数
 public:
  std::string func_type;
  std::string ident;
  vector<BaseAST*> funcfparams;
  std::unique_ptr<BaseAST> block;
  mutable vector<string> param;
  void Dump(string& koopaIR) const override 
  {
    //std::cout << "FuncDefAST { ";
    //koopaIR += "fun @";
    //koopaIR += ident;
    //koopaIR += "(): ";
    //func_type->Dump(koopaIR);
    //std::cout << ", " << ident << ", ";
    //block->Dump(koopaIR);
    //std::cout << " }";
  }
  string cal(string& koopaIR) const override
  {
    //cerr<<"\n\n";
    globalF[ident] = func_type;
    //koopaIR += "\n" + ident + " " + func_type + "\n";
    STnum = 0;
    koopaIR += "fun @";
    koopaIR += ident;
    koopaIR += "(";
    for (int i = 0; i < funcfparams.size(); i++)
    { //遍历每一个参数
        param.push_back(funcfparams[i]->cal(koopaIR));//参数名字(ident)
        if((i+1)!=funcfparams.size()) koopaIR += ",";
    }
    koopaIR += ")";
    if(func_type == "int") koopaIR += ":i32";
    koopaIR += "{\n";
    koopaIR += "%entry" + to_string(++entrynum) + ":\n";

    STnum++;
    ST *now_st = new ST;
    now_st->num = STnum;
    now_st->fa = cur_st;
    cur_st = now_st;
    for (int i = 0; i < param.size(); i++)//传递参数
    {
      if(param[i][0]>='0'&&param[i][0]<='9')//数组参数
      {
        int split1 = 0;
        for(int j = 0; j < param[i].size(); j ++)
        {
          if(param[i][j]>='0'&&param[i][j]<='9') split1 = j;
          else break;
        }
        int len = 0;
        for(int j = split1, p = 1; j >= 0; j --, p *= 10)
        {
          len += int(param[i][j]-'0')*p;
        }
        param[i] = param[i].substr(split1+1);
        int split2 = 0;
        for(int j = 0; j < param[i].size(); j++)
        {
          if(param[i][j] == '*') break;
          else split2 = j;
        }
        Symbol symbol = {"paramlist",len};
        string name = param[i].substr(0,split2+1);
        cur_st->table[name] = symbol;
        koopaIR += "@" + name + "_1=alloc " + param[i].substr(split2+1) + "\n";
        koopaIR += "store %" + name + ",@" + name + "_1\n";
      }
      else//普通单值
      {
        Symbol symbol = {"var",0};
        cur_st->table[param[i]] = symbol;
        koopaIR += "@" + param[i] + "_1=alloc i32\n";
        koopaIR += "store %" + param[i] + ",@" + param[i] + "_1\n";
      }
    }
    //cerr<<"\n"<<koopaIR<<"\n";
    block->cal(koopaIR);
    if(!check(koopaIR)) koopaIR += "ret \n";
    koopaIR += "}\n";
    cur_st = cur_st->fa;
    return "";
  }
  int compute() const override
  {
     return 0;
  }
};

// FuncTypeAST 也是 BaseAST
/*
class FuncTypeAST : public BaseAST
{
    public:
        std::string functype;//两种
        void Dump(string& koopaIR) const override
        {
            //std::cout << "FuncTypeAST { ";
            //std::cout << "int ";
            //std::cout << " }";
            //koopaIR += "i32";
        }
        string cal(string& koopaIR) const override
        {
          if(functype == "int") koopaIR += ": i32 ";
          return "";
        }
        int compute() const override
        {
          return 0;
        }
};*/

class FuncFParam1AST : public BaseAST
{
    public:
        string type;
        std::string ident;
        void Dump(string& koopaIR) const override
        {
            //std::cout << "FuncTypeAST { ";
            //std::cout << "int ";
            //std::cout << " }";
            //koopaIR += "i32";
        }
        string cal(string& koopaIR) const override
        {
          koopaIR += "%" + ident + ":i32";
          return ident;
        }
        int compute() const override
        {
          return 0;
        }
};


class FuncFParam2AST : public BaseAST//数组型参数
{
    public:
        string type;
        std::string ident;
        vector<BaseAST*> constexp_;
        void Dump(string& koopaIR) const override
        {
            //std::cout << "FuncTypeAST { ";
            //std::cout << "int ";
            //std::cout << " }";
            //koopaIR += "i32";
        }
        string cal(string& koopaIR) const override
        {
          vector<int> v;
          for(int i = 0; i < constexp_.size(); i++)
          {
            v.push_back(constexp_[i]->compute());
          }
          koopaIR += "%" + ident + ":";
          string Type = "*";
          for(int i = 0; i < v.size(); i++)
            Type += "[";
          Type += "i32";
          for(int i = v.size() - 1; i >= 0; i--)
            Type +="," + to_string(v[i]) + "]";
          koopaIR += Type;
          return to_string(constexp_.size()+1) + ident + Type;
        }
        int compute() const override
        {
          return 0;
        }
};


// BlockAST 也是 BaseAST
class BlockAST : public BaseAST
{
    public:
        vector<BaseAST*> blockitem_;
        void Dump(string& koopaIR) const override
        {
            //std::cout << "BlockAST { ";
            //koopaIR += "{\n";
            //koopaIR += "%entry:\n";
            //stmt->Dump(koopaIR);
            //std::cout << " }";
            //koopaIR +="\n}\n";
        }
        string cal(string& koopaIR) const override
        {
          //cerr<<koopaIR;
          for (int i = 0; i < blockitem_.size(); i++)
          { //遍历每一个item
              if(check(koopaIR)) break;
              blockitem_[i]->cal(koopaIR);
          }
          return "";
        }
        int compute() const override
        {
          return 0;
        }
};

class BlockItem1AST: public BaseAST
{
    public:
        std::unique_ptr<BaseAST> stmt;
        void Dump(string& koopaIR) const override
        {
            
        }
        string cal(string& koopaIR) const override
        {
          //cerr<<koopaIR;
          stmt->cal(koopaIR);
          return "";
        }
        int compute() const override
        {
          return 0;
        }
};

class BlockItem2AST: public BaseAST
{
    public:
        std::unique_ptr<BaseAST> decl;
        void Dump(string& koopaIR) const override
        {
            
        }
        string cal(string& koopaIR) const override
        {
          decl->cal(koopaIR);
          return "";
        }
        int compute() const override
        {
          return 0;
        }
};

class Stmt1AST :public BaseAST
{
    public:
        unique_ptr<BaseAST> match;
        void Dump(string& koopaIR) const override
        {

        }
        string cal(string& koopaIR) const override
        {
          match->cal(koopaIR);
          return "";

        }
        int compute() const override
        {
          return 0;
        }
};

class Stmt2AST :public BaseAST
{
    public:
        unique_ptr<BaseAST> unmatch;
        void Dump(string& koopaIR) const override
        {

        }
        string cal(string& koopaIR) const override
        {
          unmatch->cal(koopaIR);
          return "";

        }
        int compute() const override
        {
          return 0;
        }
};

class UnMatchStmt1AST :public BaseAST//if(exp) stmt;
{
    public:
        unique_ptr<BaseAST> exp;
        unique_ptr<BaseAST> stmt;
        void Dump(string& koopaIR) const override
        {

        }
        string cal(string& koopaIR) const override
        {
          IFnum++;
          string then_ = "%then_" + to_string(IFnum);
          string end_ = "%end_" + to_string(IFnum);
          string re = exp->cal(koopaIR);
          if(re[0]=='@')//变量
          {
            koopaIR += "%" + to_string(++cnt) + "=load " + re + "\n";
            koopaIR += "br %" + to_string(cnt) + "," + then_ + "," + end_ + "\n";
          }
          else
          {
            koopaIR += "br " + re + "," + then_ + "," + end_ + "\n";
          }
          koopaIR += then_ + ":\n";
          stmt->cal(koopaIR);
          if(!check(koopaIR)) koopaIR += "jump " + end_ + "\n";
          koopaIR += end_ + ":\n";
          return "";

        }
        int compute() const override
        {
          return 0;
        }
};

class UnMatchStmt2AST :public BaseAST //if(exp) match else unmatch
{
    public:
        unique_ptr<BaseAST> exp;
        unique_ptr<BaseAST> match;
        unique_ptr<BaseAST> unmatch;
        void Dump(string& koopaIR) const override
        {

        }
        string cal(string& koopaIR) const override
        {
          IFnum++;
          string then_ = "%then_" + to_string(IFnum);
          string else_ = "%else_" + to_string(IFnum);
          string end_ = "%end_" + to_string(IFnum); 
          string re = exp->cal(koopaIR);
          if(re[0]=='@')//变量
          {
            koopaIR += "%" + to_string(++cnt) + "=load " + re + "\n";
            koopaIR += "br %" + to_string(cnt) + "," + then_ + "," + else_ + "\n";
          }
          else
          {
            koopaIR += "br " + re + "," + then_ + "," + else_ + "\n";
          }
          koopaIR += then_ + ":\n";
          match->cal(koopaIR);
          if(!check(koopaIR)) koopaIR += "jump " + end_ + "\n";
          koopaIR += else_ + ":\n";
          unmatch->cal(koopaIR);
          if(!check(koopaIR)) koopaIR += "jump " + end_ + "\n";
          koopaIR += end_ + ":\n";
          return "";

        }
        int compute() const override
        {
          return 0;
        }
};

class UnMatchStmt3AST :public BaseAST//while(exp) unmatch
{
    public:
        unique_ptr<BaseAST> exp;
        unique_ptr<BaseAST> unmatch;
        void Dump(string& koopaIR) const override
        {

        }
        string cal(string& koopaIR) const override
        {
          WHILEnum++;
          WT *now_wh = new WT;
          now_wh->num = WHILEnum;
          now_wh->fa = cur_wh;
          cur_wh = now_wh;
          string whilecond_ = "%whilecond_" + to_string(WHILEnum);
          string whilebody_ = "%whilebody_" + to_string(WHILEnum);
          string end_ = "%whileend_" + to_string(WHILEnum);
          koopaIR += "jump " + whilecond_ + "\n";
          koopaIR += whilecond_ + ":\n";
          string re = exp->cal(koopaIR);
          if(re[0]=='@')//变量
          {
            koopaIR += "%" + to_string(++cnt) + "=load " + re + "\n";
            koopaIR += "br %" + to_string(cnt) + "," + whilebody_ + "," + end_ + "\n";
          }
          else
          {
            koopaIR += "br " + re + "," + whilebody_ + "," + end_ + "\n";
          }
          koopaIR += whilebody_ + ":\n";
          unmatch->cal(koopaIR);
          if(!check(koopaIR)) koopaIR += "jump " + whilecond_ + "\n";
          koopaIR += end_ + ":\n";
          cur_wh = cur_wh->fa;
          return "";

        }
        int compute() const override
        {
          return 0;
        }
};

// StmtAST 也是 BaseAST
class MatchStmt1AST : public BaseAST//return exp
{
    public:
        unique_ptr<BaseAST> exp;
        void Dump(string& koopaIR) const override
        {
            //string re = exp->cal(koopaIR);
            //koopaIR += "  ret ";
            //koopaIR += re;
            //std::cout << "StmtAST { ";
            //std::cout <<" "<< number <<" ";
            //std::cout << " }";
        }
        string cal(string& koopaIR) const override
        {
          //cerr<<koopaIR;
          string re = exp->cal(koopaIR);
          if(re[0]=='@')//变量
          {
            koopaIR += "%" + to_string(++cnt) + "=load " + re + "\n";
            koopaIR += "ret %" + to_string(cnt) + "\n";
          }
          else
          {
            koopaIR += "ret " + re + "\n";
          }
          return "";
        }
        int compute() const override
        {
          return 0;
        }
};

class MatchStmt2AST: public BaseAST//赋值 lval = exp 左面一定是变量
{
    public:
        std::unique_ptr<BaseAST> lval;
        std::unique_ptr<BaseAST> exp;
        void Dump(string& koopaIR) const override
        {
           
        }
        string cal(string& koopaIR) const override
        {
          string left = lval->cal(koopaIR);
          string re = exp->cal(koopaIR);
          if(re[0]=='@')//变量
          {
            koopaIR += "%" + to_string(++cnt) + "=load " + re + "\n";
            koopaIR += "store %" + to_string(cnt) + "," + left + "\n"; 
          }
          else//常量,临时值
          {
            koopaIR += "store " + re + "," + left + "\n";
          }

          return "";
        }
        int compute() const override
        {
          return 0;
        }
};

class MatchStmt3AST :public BaseAST
{
    public:
        
        void Dump(string& koopaIR) const override
        {

        }
        string cal(string& koopaIR) const override
        {
          koopaIR += "ret \n";
          return "";

        }
        int compute() const override
        {
          return 0;
        }
};

class MatchStmt4AST :public BaseAST
{
    public:
        unique_ptr<BaseAST> block;
        void Dump(string& koopaIR) const override
        {

        }
        string cal(string& koopaIR) const override
        {
          STnum++;
          ST *now_st = new ST;
          now_st->num = STnum;
          now_st->fa = cur_st;
          cur_st = now_st;

          block->cal(koopaIR);

          cur_st = cur_st->fa;
          return "";
        }
        int compute() const override
        {
          return 0;
        }
};

class MatchStmt5AST :public BaseAST
{
    public:
        
        void Dump(string& koopaIR) const override
        {

        }
        string cal(string& koopaIR) const override
        {
          return "";

        }
        int compute() const override
        {
          return 0;
        }
};

class MatchStmt6AST :public BaseAST
{
    public:
        unique_ptr<BaseAST> exp;
        void Dump(string& koopaIR) const override
        {

        }
        string cal(string& koopaIR) const override
        {
          exp->cal(koopaIR);
          return "";

        }
        int compute() const override
        {
          return 0;
        }
};

class MatchStmt7AST :public BaseAST//if(exp) match1 else match2
{
    public:
        unique_ptr<BaseAST> exp;
        unique_ptr<BaseAST> match1;
        unique_ptr<BaseAST> match2;
        void Dump(string& koopaIR) const override
        {

        }
        string cal(string& koopaIR) const override
        {
          IFnum++;
          string then_ = "%then_" + to_string(IFnum);
          string else_ = "%else_" + to_string(IFnum);
          string end_ = "%end_" + to_string(IFnum); 
          string re = exp->cal(koopaIR);
          if(re[0]=='@')//变量
          {
            koopaIR += "%" + to_string(++cnt) + "=load " + re + "\n";
            koopaIR += "br %" + to_string(cnt) + "," + then_ + "," + else_ + "\n";
          }
          else
          {
            koopaIR += "br " + re + "," + then_ + "," + else_ + "\n";
          }
          koopaIR += then_ + ":\n";
          match1->cal(koopaIR);
          if(!check(koopaIR)) koopaIR += "jump " + end_ + "\n";
          koopaIR += else_ + ":\n";
          match2->cal(koopaIR);
          if(!check(koopaIR)) koopaIR += "jump " + end_ + "\n";
          koopaIR += end_ + ":\n";
          return "";

        }
        int compute() const override
        {
          return 0;
        }
};

class MatchStmt8AST :public BaseAST//while(exp) match
{
    public:
        unique_ptr<BaseAST> exp;
        unique_ptr<BaseAST> match;
        void Dump(string& koopaIR) const override
        {

        }
        string cal(string& koopaIR) const override
        {
          WHILEnum++;
          WT *now_wh = new WT;
          now_wh->num = WHILEnum;
          now_wh->fa = cur_wh;
          cur_wh = now_wh;
          string whilecond_ = "%whilecond_" + to_string(WHILEnum);
          string whilebody_ = "%whilebody_" + to_string(WHILEnum);
          string end_ = "%whileend_" + to_string(WHILEnum);
          koopaIR += "jump " + whilecond_ + "\n";
          koopaIR += whilecond_ + ":\n";
          string re = exp->cal(koopaIR);
          if(re[0]=='@')//变量
          {
            koopaIR += "%" + to_string(++cnt) + "=load " + re + "\n";
            koopaIR += "br %" + to_string(cnt) + "," + whilebody_ + "," + end_ + "\n";
          }
          else
          {
            koopaIR += "br " + re + "," + whilebody_ + "," + end_ + "\n";
          }
          koopaIR += whilebody_ + ":\n";
          match->cal(koopaIR);
          if(!check(koopaIR)) koopaIR += "jump " + whilecond_ + "\n";
          koopaIR += end_ + ":\n";
          cur_wh = cur_wh->fa;
          return "";
        }
        int compute() const override
        {
          return 0;
        }
};

class MatchStmt9AST :public BaseAST//break;
{
    public:
        void Dump(string& koopaIR) const override
        {

        }
        string cal(string& koopaIR) const override
        {
          string end_ = "%whileend_" + to_string(cur_wh->num);
          koopaIR += "jump " + end_ + "\n";
          return "";

        }
        int compute() const override
        {
          return 0;
        }
};

class MatchStmt10AST :public BaseAST//continue
{
    public:
        void Dump(string& koopaIR) const override
        {

        }
        string cal(string& koopaIR) const override
        {
          string whilecond_ = "%whilecond_" + to_string(cur_wh->num);
          koopaIR += "jump " + whilecond_ + "\n";
          return "";

        }
        int compute() const override
        {
          return 0;
        }
};

class LValAST: public BaseAST//出现在赋值语句左边或者表达式中 常量或变量
{
    public:
        string ident;
        vector<BaseAST*> exp__;
        void Dump(string& koopaIR) const override
        {
            
        }
        string cal(string& koopaIR) const override
        {
            ST *now_st = cur_st;
            while(now_st->table.find(ident)==now_st->table.end()) now_st = now_st->fa;
            auto symbol = now_st->table[ident];
            string re;

            if(symbol.type == "const")//常量单值
            {
              re = to_string(symbol.value);
              return re;
            }
            else if(symbol.type == "paramlist")//参数数组
            {
              vector<string> v;
              for(int i = 0; i <exp__.size(); i ++)
              {
                v.push_back(exp__[i]->cal(koopaIR));
              }
              for(int i = 0; i < v.size(); i++)
              {
                if(v[i][0] == '@') 
                {
                  koopaIR += "%" + to_string(++cnt) + "=load " + v[i] + "\n";
                  v[i] = "%" + to_string(cnt);
                }
              }
              string pre = "@" + ident + "_" + to_string(now_st->num);
              koopaIR += "%" + to_string(++cnt) + "=load " + pre + "\n";
              pre = "%" + to_string(cnt);
              if(exp__.size() == symbol.value)//单值
              {
                koopaIR += "@L" + to_string(++cnt) + "=getptr " + pre + "," + v[0] + "\n";
                pre = "@L" + to_string(cnt);
                for(int i = 1; i < v.size(); i ++)
                {
                  koopaIR += "@L" + to_string(++cnt) + "=getelemptr " + pre + "," + v[i] + "\n";
                  pre = "@L" + to_string(cnt);
                }
                return pre;
              }
              else//数组参数
              {
                if(v.size() == 0) return pre;
                else
                {
                  koopaIR += "%" + to_string(++cnt) + "=getptr " + pre + "," + v[0] + "\n";
                  pre = "%" + to_string(cnt);
                  for(int i = 1; i < v.size(); i ++)
                  {
                    koopaIR += "%" + to_string(++cnt) + "=getelemptr " + pre + "," + v[i] + "\n";
                    pre = "%" + to_string(cnt);
                  }
                  koopaIR += "%" + to_string(++cnt) + "=getelemptr " + pre + ",0\n";
                  return "%" + to_string(cnt);
                }
              }

            }
            //数组要考虑是具体的值还是用来传参的
            else if(symbol.type == "varlist" || symbol.type == "constlist")
            {
              vector<string> v;
              for(int i = 0; i <exp__.size(); i ++)
              {
                v.push_back(exp__[i]->cal(koopaIR));
              }
              for(int i = 0; i < v.size(); i++)
              {
                if(v[i][0] == '@') 
                {
                  koopaIR += "%" + to_string(++cnt) + "=load " + v[i] + "\n";
                  v[i] = "%" + to_string(cnt);
                }
              }
              string pre = "@" + ident + "_" + to_string(now_st->num);
              if(exp__.size() == symbol.value)//单值
              {
                for(int i = 0; i < v.size(); i ++)
                {
                  koopaIR += "@L" + to_string(++cnt) + "=getelemptr " + pre + "," + v[i] + "\n";
                  pre = "@L" + to_string(cnt);
                }
                return pre;
              }
              else
              {
                for(int i = 0; i < v.size(); i ++)
                {
                  koopaIR += "%" + to_string(++cnt) + "=getelemptr " + pre + "," + v[i] + "\n";
                  pre = "%" + to_string(cnt);
                }
                koopaIR += "%" + to_string(++cnt) + "=getelemptr " + pre + ",0\n";
                return "%" + to_string(cnt);//%防止被load
              }
              
            }
            else//变量单值 var/undef
            {
              re = "@" + ident + "_" + to_string(now_st->num);
              return re;
            }
        }
        int compute() const override
        {
          ST *now_st = cur_st;
          while(now_st->table.find(ident)==now_st->table.end()) now_st = now_st->fa;
          auto symbol = now_st->table[ident];
          return symbol.value;
        }
};

class ExpAST :public BaseAST
{
    public:
        unique_ptr<BaseAST> lorexp;
        void Dump(string& koopaIR) const override
        {

        }
        string cal(string& koopaIR) const override
        {
          return lorexp->cal(koopaIR);

        }
        int compute() const override
        {
          return lorexp->compute();
        }
};

class LOrExp1AST : public BaseAST
{
    public:
      unique_ptr<BaseAST> landexp;
      void Dump(string& koopaIR) const override
      {

      }
      string cal(string& koopaIR) const override
      {
        return landexp->cal(koopaIR);
      }
      int compute() const override
      {
        return landexp->compute();
      }
};

class LOrExp2AST : public BaseAST
{
    public:
      unique_ptr<BaseAST> lorexp;
      unique_ptr<BaseAST> landexp;
      void Dump(string& koopaIR) const override
      {

      }
      string cal(string& koopaIR) const override
      {/*
         string L = lorexp->cal(koopaIR);
         string R = landexp->cal(koopaIR);
         if(L[0]=='@')
         {
           koopaIR += "  %" + to_string(++cnt) + " = load " + L + "\n";
           L = "%" + to_string(cnt);
         }
         if(R[0]=='@')
         {
           koopaIR += "  %" + to_string(++cnt) + " = load " + R + "\n";
           R = "%" + to_string(cnt);
         }
         koopaIR += "%" + to_string(++cnt) + " = or " + L +", " + R + "\n";
         cnt++;
         koopaIR += "%" + to_string(cnt) + " = ne 0, %" + to_string(cnt-1) + "\n";        
         return "%" + to_string(cnt);*/
         SHORT++;
         string ans = "@lorans_" + to_string(SHORT);
         string L = lorexp->cal(koopaIR);
         if(L[0]=='@')
         {
           koopaIR += "%" + to_string(++cnt) + "=load " + L + "\n";
           L = "%" + to_string(cnt);
         }
         IFnum++;
         string then_ = "%then_" + to_string(IFnum);
         string else_ = "%else_" + to_string(IFnum);
         string end_ = "%end_" + to_string(IFnum);
         koopaIR += ans + "=alloc i32\n";
         koopaIR += "br " + L + "," + then_ + "," + else_ + "\n";
         koopaIR += then_ + ":\n";
         koopaIR += "store 1," + ans + "\n";
         koopaIR += "jump " + end_ + "\n";
         koopaIR += else_ + ":\n";
         string R = landexp->cal(koopaIR);
         if(R[0]=='@')
         {
           koopaIR += "%" + to_string(++cnt) + "=load " + R + "\n";
           R = "%" + to_string(cnt);
         }
         koopaIR += "%" + to_string(++cnt) + "=ne 0," + R + "\n";
         koopaIR += "store %" + to_string(cnt) + "," + ans + "\n";
         koopaIR += "jump " + end_ + "\n";
         koopaIR += end_ + ":\n";
         return ans;

      }
      int compute() const override
      {
          int L = lorexp->compute();
          int R = landexp->compute();
          return L || R;
      }
};

class LAndExp1AST : public BaseAST
{
    public:
      unique_ptr<BaseAST> eqexp;
      void Dump(string& koopaIR) const override
      {

      }
      string cal(string& koopaIR) const override
      {
        return eqexp->cal(koopaIR);
      }
      int compute() const override
      {
        return eqexp->compute();
      }
};

class LAndExp2AST : public BaseAST
{
    public:
      unique_ptr<BaseAST> landexp;
      unique_ptr<BaseAST> eqexp;
      void Dump(string& koopaIR) const override
      {

      }
      string cal(string& koopaIR) const override
      {/*
       string L = landexp->cal(koopaIR);
       string R = eqexp->cal(koopaIR);
       if(L[0]=='@')
       {
          koopaIR += "  %" + to_string(++cnt) + " = load " + L + "\n";
          L = "%" + to_string(cnt);
       }
       if(R[0]=='@')
       {
          koopaIR += "  %" + to_string(++cnt) + " = load " + R + "\n";
          R = "%" + to_string(cnt);
       }
       koopaIR += "%" + to_string(++cnt) + " = ne 0, " + L + "\n";
       koopaIR += "%" + to_string(++cnt) + " = ne 0, " + R + "\n";
       cnt++;
       koopaIR += "%" + to_string(cnt) + " = and %" + to_string(cnt-2) +", %" + to_string(cnt-1) + "\n";       
       return "%" + to_string(cnt);*/
         SHORT++;
         string ans = "@landans_" + to_string(SHORT);
         string L = landexp->cal(koopaIR);
         if(L[0]=='@')
         {
           koopaIR += "%" + to_string(++cnt) + "=load " + L + "\n";
           L = "%" + to_string(cnt);
         }
         IFnum++;
         string then_ = "%then_" + to_string(IFnum);
         string else_ = "%else_" + to_string(IFnum);
         string end_ = "%end_" + to_string(IFnum);
         koopaIR += ans + "=alloc i32\n";
         koopaIR += "br " + L + "," + else_ + "," + then_ + "\n";
         koopaIR += then_ + ":\n";
         koopaIR += "store 0," + ans + "\n";
         koopaIR += "jump " + end_ + "\n";
         koopaIR += else_ + ":\n";
         string R = eqexp->cal(koopaIR);
         if(R[0]=='@')
         {
           koopaIR += "%" + to_string(++cnt) + "=load " + R + "\n";
           R = "%" + to_string(cnt);
         }
         koopaIR += "%" + to_string(++cnt) + "=ne 0," + R + "\n";
         koopaIR += "store %" + to_string(cnt) + "," + ans + "\n";
         koopaIR += "jump " + end_ + "\n";
         koopaIR += end_ + ":\n";
         return ans;
      }
      int compute() const override
      {
         int L = landexp->compute();
         int R = eqexp->compute();
         return L && R;
      }
};

class EqExp1AST : public BaseAST
{
    public:
      unique_ptr<BaseAST> relexp;
      void Dump(string& koopaIR) const override
      {

      }
      string cal(string& koopaIR) const override
      {
        return relexp->cal(koopaIR);
      }
      int compute() const override
      {
        return relexp->compute();
      }
};

class EqExp2AST : public BaseAST
{
    public:
      unique_ptr<BaseAST> eqexp;
      string eqop;
      unique_ptr<BaseAST> relexp;
      void Dump(string& koopaIR) const override
      {

      }
      string cal(string& koopaIR) const override
      {
        string L = eqexp->cal(koopaIR);
        string R = relexp->cal(koopaIR);
        if(L[0]=='@')
        {
          koopaIR += "%" + to_string(++cnt) + "=load " + L + "\n";
          L = "%" + to_string(cnt);
        }
        if(R[0]=='@')
        {
          koopaIR += "%" + to_string(++cnt) + "=load " + R + "\n";
          R = "%" + to_string(cnt);
        }
        switch(eqop[0])
        {
          case '=':
          koopaIR += "%" + to_string(++cnt) + "=eq " + L +"," + R + "\n";
          break;

          case '!':
          koopaIR += "%" + to_string(++cnt) + "=ne " + L +"," + R + "\n";
          break;

          default:
          break;
        }
        return "%" + to_string(cnt);
      }
      int compute() const override
      {
          int L = eqexp->compute();
          int R = relexp->compute();
          if(eqop == "==") return L == R;
          else return L != R;
      }
};

class RelExp1AST : public BaseAST
{
    public:
      unique_ptr<BaseAST> addexp;
      void Dump(string& koopaIR) const override
      {

      }
      string cal(string& koopaIR) const override
      {
        return addexp->cal(koopaIR);
      }
      int compute() const override
      {
        return addexp->compute();
      }
};

class RelExp2AST : public BaseAST
{
    public:
      unique_ptr<BaseAST> relexp;
      string relop;
      unique_ptr<BaseAST> addexp;
      void Dump(string& koopaIR) const override
      {

      }
      string cal(string& koopaIR) const override
      {
        string L = relexp->cal(koopaIR);
        string R = addexp->cal(koopaIR);
        if(L[0]=='@')
        {
          koopaIR += "%" + to_string(++cnt) + "=load " + L + "\n";
          L = "%" + to_string(cnt);
        }
        if(R[0]=='@')
        {
          koopaIR += "%" + to_string(++cnt) + "=load " + R + "\n";
          R = "%" + to_string(cnt);
        }
        if(relop == "<")
          koopaIR += "%" + to_string(++cnt) + "=lt " + L +"," + R + "\n";
        else if(relop == ">")
          koopaIR += "%" + to_string(++cnt) + "=gt " + L +"," + R + "\n";
        else if(relop == "<=")
          koopaIR += "%" + to_string(++cnt) + "=le " + L +"," + R + "\n";
        else 
          koopaIR += "%" + to_string(++cnt) + "=ge " + L +"," + R + "\n";
        
        return "%" + to_string(cnt);
      }
      int compute() const override
      {
        int L = relexp->compute();
        int R = addexp->compute();
        if(relop == "<") return L < R;
        else if(relop == ">") return L > R;
        else if(relop == ">=") return L >= R;
        else return L <= R;
      }
};

class UnaryExp1AST : public BaseAST
{
      public:
          unique_ptr<BaseAST> primaryexp;
          void Dump(string& koopaIR) const override
          {

          }
          string cal(string& koopaIR) const override
          {
            return primaryexp->cal(koopaIR);
          }
          int compute() const override
          {
            return primaryexp->compute();
          }
};

class UnaryExp2AST : public BaseAST
{
      public:
          string unaryop;
          unique_ptr<BaseAST> unaryexp;
          void Dump(string& koopaIR) const override
          {

          }
          string cal(string& koopaIR) const override
          {
             string R = unaryexp->cal(koopaIR);
             if(R[0]=='@')//先加载
             {
               koopaIR += "%" + to_string(++cnt) + "=load " + R + "\n";
               R = "%" + to_string(cnt);
             }
             switch(unaryop[0])
             {
              case '+':
                return R;
                break;
              case '-':
                koopaIR += "%" + to_string(++cnt) + "=sub 0," + R +"\n";
                break;
              case '!':
                koopaIR += "%" + to_string(++cnt) + "=eq " + R +",0\n";
                break;
              default :
                break;
             }
             return "%" + to_string(cnt);
          }
          int compute() const override
          {
            int R = unaryexp->compute();
            if(unaryop == "+") return R;
            else if(unaryop == "-") return -R;
            else return !R;
          }
};

class UnaryExp3AST : public BaseAST//无参数
{
    public:
        std::string ident;
        void Dump(string& koopaIR) const override
        {
            //std::cout << "FuncTypeAST { ";
            //std::cout << "int ";
            //std::cout << " }";
            //koopaIR += "i32";
        }
        string cal(string& koopaIR) const override
        {
          string ans = "";
          if(globalF[ident] == "void")
          {
            koopaIR +="call @" + ident + "()\n";
          }
          else
          {
            string now = "%" + to_string(++cnt);
            koopaIR += now + "=call @" + ident + "()\n";
            ans = now;
          }
          return ans;
        }
        int compute() const override
        {
          return 0;
        }
};
class UnaryExp4AST : public BaseAST//有参数函数调用
{
    public:
        std::string ident;
        vector<BaseAST*> funcrparams;
        mutable vector<string> param;
        void Dump(string& koopaIR) const override
        {
            //std::cout << "FuncTypeAST { ";
            //std::cout << "int ";
            //std::cout << " }";
            //koopaIR += "i32";
        }
        string cal(string& koopaIR) const override
        {
          for(int i = 0; i < funcrparams.size(); i++)
          {
            string re = funcrparams[i]->cal(koopaIR);
            if(re[0]=='@')//变量
            {
              koopaIR += "%" + to_string(++cnt) + "=load " + re + "\n";
              re = "%" + to_string(cnt);
            }
            param.push_back(re);
          }
          string ans = "";
          if(globalF[ident] == "void")
          {
            koopaIR += "call @" + ident + "(";
          }
          else
          {
           string now = "%" + to_string(++cnt);
           koopaIR += now + "=call @" + ident + "(";
           ans = now;
          }
          for(int i = 0; i < param.size(); i++)
          {
            koopaIR += param[i];
            if((i+1)!=param.size()) koopaIR += ",";
          }
          koopaIR += ")\n";
          return ans;
        }
        int compute() const override
        {
          return 0;
        }
};

class AddExp1AST : public BaseAST
{
    public:
      unique_ptr<BaseAST> mulexp;
      void Dump(string& koopaIR) const override
      {
        
      }
      string cal(string& koopaIR) const override
      {
        return mulexp->cal(koopaIR);
      }
      int compute() const override
      {
        return mulexp->compute();
      }
};

class AddExp2AST : public BaseAST
{
    public:
      unique_ptr<BaseAST> addexp;
      string unaryop;
      unique_ptr<BaseAST> mulexp;
      void Dump(string& koopaIR) const override
      {

      }
      string cal(string& koopaIR) const override
      {
        string L = addexp->cal(koopaIR);
        string R = mulexp->cal(koopaIR);
        if(L[0]=='@')
        {
          koopaIR += "%" + to_string(++cnt) + "=load " + L + "\n";
          L = "%" + to_string(cnt);
        }
        if(R[0]=='@')
        {
          koopaIR += "%" + to_string(++cnt) + "=load " + R + "\n";
          R = "%" + to_string(cnt);
        }
        switch(unaryop[0])
        {
          case '+':
          koopaIR += "%" + to_string(++cnt) + "=add " + L +"," + R + "\n";
          break;

          case '-':
          koopaIR += "%" + to_string(++cnt) + "=sub " + L +"," + R + "\n";
          break;

          default :
          break;
        }
        return "%" + to_string(cnt);
      }
      int compute() const override
      {
        int L = addexp->compute();
        int R = mulexp->compute();
        if(unaryop == "+") return L + R;
        else return L - R;
      }
};

class MulExp1AST : public BaseAST
{
    public:
      unique_ptr<BaseAST> unaryexp;
      void Dump(string& koopaIR) const override
      {

      }
      string cal(string& koopaIR) const override
      {
        return unaryexp->cal(koopaIR);
      }
      int compute() const override
      {
        return unaryexp->compute();
      }
};

class MulExp2AST : public BaseAST
{
    public:
      unique_ptr<BaseAST> mulexp;
      string mulop;
      unique_ptr<BaseAST> unaryexp;
      void Dump(string& koopaIR) const override
      {

      }
      string cal(string& koopaIR) const override
      {
        string L = mulexp->cal(koopaIR);
        string R = unaryexp->cal(koopaIR);
        if(L[0]=='@')
        {
          koopaIR += "%" + to_string(++cnt) + "=load " + L + "\n";
          L = "%" + to_string(cnt);
        }
        if(R[0]=='@')
        {
          koopaIR += "%" + to_string(++cnt) + "=load " + R + "\n";
          R = "%" + to_string(cnt);
        }
        switch(mulop[0])
        {
          case '*':
          koopaIR += "%" + to_string(++cnt) + "=mul " + L +"," + R + "\n";
          break;

          case '/':
          koopaIR += "%" + to_string(++cnt) + "=div " + L +"," + R + "\n";
          break;

          case '%':
          koopaIR += "%" + to_string(++cnt) + "=mod " + L +"," + R + "\n";
          break;

          default :
          break;
        }
        return "%" + to_string(cnt);
      }
      int compute() const override
      {
        int L = mulexp->compute();
        int R = unaryexp->compute();
        if(mulop == "*") return L * R;
        else if(mulop == "/") return L / R;
        else return L % R;
      }
};

class PrimaryExp1AST : public BaseAST
{
      public:
          unique_ptr<BaseAST> exp;
          void Dump(string& koopaIR) const override
          {

          }
          string cal(string& koopaIR) const override
          {
            return exp->cal(koopaIR);
          }
          int compute() const override
          {
            return exp->compute();
          }
};

class PrimaryExp2AST : public BaseAST
{
      public:
          int number;
          void Dump(string& koopaIR) const override
          {

          }
          string cal(string& koopaIR) const override
          {
            return to_string(number);
          }
          int compute() const override
          {
            return number;
          }
          
};

class PrimaryExp3AST: public BaseAST
{
    public:
        std::unique_ptr<BaseAST> lval;
        void Dump(string& koopaIR) const override
        {
            
        }
        string cal(string& koopaIR) const override
        {
          return lval->cal(koopaIR);
        }
        int compute() const override
        {
          return lval->compute();
        }
};







