#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <map>
using namespace std;
//基类
extern int cnt;
extern int STnum;
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


class BaseAST
{
 public:
  virtual ~BaseAST() = default;
  virtual void Dump(string& koopaIR) const = 0;
  virtual string cal(string& koopaIR) const = 0;
  virtual int compute() const = 0;
};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST 
{
 public:
  std::unique_ptr<BaseAST> func_def;

  void Dump(string& koopaIR) const override 
  {
    //std::cout << "CompUnitAST { ";
    //func_def->Dump(koopaIR);
    //std::cout << " }";
  }
  string cal(string& koopaIR) const override
  {
    func_def->cal(koopaIR);
    return "";
  }
  int compute() const override
  {
    return 0;
  }
};

class Decl1AST: public BaseAST
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

class Decl2AST: public BaseAST
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
        std::unique_ptr<BaseAST> btype;
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
        std::unique_ptr<BaseAST> btype;
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
        void Dump(string& koopaIR) const override
        {
            
        }
        string cal(string& koopaIR) const override
        {
          Symbol symbol = {"undef",0};
          cur_st->table[ident] = symbol;

          koopaIR += "@" + ident + "_" + to_string(cur_st->num) + " = alloc i32\n"; 
          return "";
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
        std::unique_ptr<BaseAST> initval;
        void Dump(string& koopaIR) const override
        {
            
        }
        string cal(string& koopaIR) const override
        {
          
          Symbol symbol = {"var",0};
          cur_st->table[ident] = symbol;
          koopaIR += "@" + ident + "_" + to_string(cur_st->num) + " = alloc i32\n";
          string re = initval->cal(koopaIR);
          if(re[0] == '@')
          {
             koopaIR += "  %" + to_string(++cnt) + " = load " + re + "\n";
             re = "%" + to_string(cnt);
          }
          koopaIR += "  store " + re + ", " + "@" + ident + "_" +to_string(cur_st->num) + "\n";
          return "";
        }
        int compute() const override
        {
          return 0;
        }
};

class InitValAST: public BaseAST
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
          return 0;
        }
};

class BTypeAST: public BaseAST
{
    public:
        string type;
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

class ConstDefAST: public BaseAST//常量定义，无任何语句产生
{
    public:
        string ident;
        std::unique_ptr<BaseAST> constinitval;
        void Dump(string& koopaIR) const override
        {
            
        }
        string cal(string& koopaIR) const override
        {
          int ans = constinitval->compute();
          
          Symbol symbol = {"const",ans};
          cur_st->table[ident] = symbol;
          return "";
        }
        int compute() const override
        {
          return 0;
        }
};

class ConstInitValAST: public BaseAST
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
class FuncDefAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> func_type;
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
    
    koopaIR += "fun @";
    koopaIR += ident;
    koopaIR += "(): ";
    func_type->cal(koopaIR);
    koopaIR += "{\n";
    koopaIR += "%entry:\n";
    block->cal(koopaIR);
    koopaIR += "\n}\n";
    return "";
  }
  int compute() const override
  {
     return 0;
  }
};

// FuncTypeAST 也是 BaseAST
class FuncTypeAST : public BaseAST
{
    public:
        std::string functype;
        void Dump(string& koopaIR) const override
        {
            //std::cout << "FuncTypeAST { ";
            //std::cout << "int ";
            //std::cout << " }";
            //koopaIR += "i32";
        }
        string cal(string& koopaIR) const override
        {
          koopaIR += "i32";
          return "";
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
          STnum++;
          ST *now_st = new ST;
          now_st->num = STnum;
          now_st->fa = cur_st;
          cur_st = now_st;
          for (int i = 0; i < blockitem_.size(); i++)
          { //遍历每一个item
              blockitem_[i]->cal(koopaIR);
          }
          cur_st = cur_st->fa;
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

// StmtAST 也是 BaseAST
class Stmt1AST : public BaseAST//return exp
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
          string re = exp->cal(koopaIR);
          if(re[0]=='@')//变量
          {
            koopaIR += "  %" + to_string(++cnt) + " = load " + re + "\n";
            koopaIR += "  ret %" + to_string(cnt) + "\n";
          }
          else
          {
            koopaIR += "  ret " + re;
          }
          return "";
        }
        int compute() const override
        {
          return 0;
        }
};

class Stmt2AST: public BaseAST//赋值 lval = exp 左面一定是变量
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
            koopaIR += "%" + to_string(++cnt) + " = load" + re + "\n";
            koopaIR += "  store %" + to_string(cnt) + ", " + left + "\n"; 
          }
          else//常量,临时值
          {
            koopaIR += "  store " + re + ", " + left + "\n";
          }

          return "";
        }
        int compute() const override
        {
          return 0;
        }
};

class Stmt3AST :public BaseAST
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

class Stmt4AST :public BaseAST
{
    public:
        unique_ptr<BaseAST> block;
        void Dump(string& koopaIR) const override
        {

        }
        string cal(string& koopaIR) const override
        {
          block->cal(koopaIR);
          return "";
        }
        int compute() const override
        {
          return 0;
        }
};

class Stmt5AST :public BaseAST
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

class Stmt6AST :public BaseAST
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

class LValAST: public BaseAST//出现在赋值语句左边或者表达式中 常量或变量
{
    public:
        string ident;
        void Dump(string& koopaIR) const override
        {
            
        }
        string cal(string& koopaIR) const override
        {
          ST *now_st = cur_st;
          while(now_st->table.find(ident)==now_st->table.end()) now_st = now_st->fa;

          auto symbol = now_st->table[ident];
          string re;
          
          if(symbol.type == "const")
          {
            re = to_string(symbol.value);
          }
          else
          {
            re = "@" + ident + "_" + to_string(now_st->num);
          }
          return re;
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
      {
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
         return "%" + to_string(cnt);
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
      {
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
       return "%" + to_string(cnt);
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
          koopaIR += "  %" + to_string(++cnt) + " = load " + L + "\n";
          L = "%" + to_string(cnt);
        }
        if(R[0]=='@')
        {
          koopaIR += "  %" + to_string(++cnt) + " = load " + R + "\n";
          R = "%" + to_string(cnt);
        }
        switch(eqop[0])
        {
          case '=':
          koopaIR += "%" + to_string(++cnt) + " = eq " + L +", " + R + "\n";
          break;

          case '!':
          koopaIR += "%" + to_string(++cnt) + " = ne " + L +", " + R + "\n";
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
          koopaIR += "  %" + to_string(++cnt) + " = load " + L + "\n";
          L = "%" + to_string(cnt);
        }
        if(R[0]=='@')
        {
          koopaIR += "  %" + to_string(++cnt) + " = load " + R + "\n";
          R = "%" + to_string(cnt);
        }
        if(relop == "<")
          koopaIR += "%" + to_string(++cnt) + " = lt " + L +", " + R + "\n";
        else if(relop == ">")
          koopaIR += "%" + to_string(++cnt) + " = gt " + L +", " + R + "\n";
        else if(relop == "<=")
          koopaIR += "%" + to_string(++cnt) + " = le " + L +", " + R + "\n";
        else 
          koopaIR += "%" + to_string(++cnt) + " = ge " + L +", " + R + "\n";
        
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
               koopaIR += "  %" + to_string(++cnt) + " = load " + R + "\n";
               R = "%" + to_string(cnt);
             }
             switch(unaryop[0])
             {
              case '+':
                return R;
                break;
              case '-':
                koopaIR += " %" + to_string(++cnt) + " = sub 0, " + R +"\n";
                break;
              case '!':
                koopaIR += " %" + to_string(++cnt) + " = eq " + R +", 0\n";
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
          koopaIR += "  %" + to_string(++cnt) + " = load " + L + "\n";
          L = "%" + to_string(cnt);
        }
        if(R[0]=='@')
        {
          koopaIR += "  %" + to_string(++cnt) + " = load " + R + "\n";
          R = "%" + to_string(cnt);
        }
        switch(unaryop[0])
        {
          case '+':
          koopaIR += "%" + to_string(++cnt) + " = add " + L +", " + R + "\n";
          break;

          case '-':
          koopaIR += "%" + to_string(++cnt) + " = sub " + L +", " + R + "\n";
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
          koopaIR += "  %" + to_string(++cnt) + " = load " + L + "\n";
          L = "%" + to_string(cnt);
        }
        if(R[0]=='@')
        {
          koopaIR += "  %" + to_string(++cnt) + " = load " + R + "\n";
          R = "%" + to_string(cnt);
        }
        switch(mulop[0])
        {
          case '*':
          koopaIR += "%" + to_string(++cnt) + " = mul " + L +", " + R + "\n";
          break;

          case '/':
          koopaIR += "%" + to_string(++cnt) + " = div " + L +", " + R + "\n";
          break;

          case '%':
          koopaIR += "%" + to_string(++cnt) + " = mod " + L +", " + R + "\n";
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






