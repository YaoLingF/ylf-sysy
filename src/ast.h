#pragma once

#include <iostream>
#include <memory>
using namespace std;
//基类
extern int cnt;
class BaseAST
{
 public:
  virtual ~BaseAST() = default;
  virtual void Dump(string& koopaIR) const = 0;
  virtual string cal(string& koopaIR) const = 0;
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
    block->cal(koopaIR);
    return "";
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
};


// BlockAST 也是 BaseAST
class BlockAST : public BaseAST
{
    public:
        std::unique_ptr<BaseAST> stmt;
        void Dump(string& koopaIR) const override
        {
            //std::cout << "BlockAST { ";
            koopaIR += "{\n";
            koopaIR += "%entry:\n";
            stmt->Dump(koopaIR);
            //std::cout << " }";
            koopaIR +="\n}\n";
        }
        string cal(string& koopaIR) const override
        {
          koopaIR += "{\n";
          koopaIR += "%entry:\n";
          stmt->cal(koopaIR);
          koopaIR += "\n}\n";
          return "";
        }
};

// StmtAST 也是 BaseAST
class StmtAST : public BaseAST
{
    public:
        unique_ptr<BaseAST> exp;
        void Dump(string& koopaIR) const override
        {
            string re = exp->cal(koopaIR);
            koopaIR += "  ret ";
            koopaIR += re;
            //std::cout << "StmtAST { ";
            //std::cout <<" "<< number <<" ";
            //std::cout << " }";
        }
        string cal(string& koopaIR) const override
        {
          string re = exp->cal(koopaIR);
          koopaIR += "  ret ";
          koopaIR += re;
          return "";
        }
};

class ExpAST :public BaseAST
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
             return "%"+to_string(cnt);
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
          
};






