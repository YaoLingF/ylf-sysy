#pragma once

#include <iostream>
#include <memory>
using namespace std;
//基类
class BaseAST
{
 public:
  virtual ~BaseAST() = default;
  virtual void Dump(string& koopaIR) const = 0;
};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST 
{
 public:
  std::unique_ptr<BaseAST> func_def;

  void Dump(string& koopaIR) const override 
  {
    //std::cout << "CompUnitAST { ";
    func_def->Dump(koopaIR);
    //std::cout << " }";
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
    koopaIR += "fun @";
    koopaIR += ident;
    koopaIR += "(): ";
    func_type->Dump(koopaIR);
    //std::cout << ", " << ident << ", ";
    block->Dump(koopaIR);
    //std::cout << " }";
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
            koopaIR += "i32";
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
};

// StmtAST 也是 BaseAST
class StmtAST : public BaseAST
{
    public:
        int number;
        void Dump(string& koopaIR) const override
        {
            koopaIR += "  ret ";
            koopaIR += to_string(number);
            //std::cout << "StmtAST { ";
            //std::cout <<" "<< number <<" ";
            //std::cout << " }";
        }
};




