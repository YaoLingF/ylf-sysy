#pragma once

#include <iostream>
#include <memory>
using namespace std;
//基类
class BaseAST
{
 public:
  virtual ~BaseAST() = default;
  virtual void Dump() const = 0;
};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST 
{
 public:
  std::unique_ptr<BaseAST> func_def;

  void Dump() const override 
  {
    std::cout << "CompUnitAST { ";
    func_def->Dump();
    std::cout << " }";
  }
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> func_type;
  std::string ident;
  std::unique_ptr<BaseAST> block;

  void Dump() const override 
  {
    std::cout << "FuncDefAST { ";
    func_type->Dump();
    std::cout << ", " << ident << ", ";
    block->Dump();
    std::cout << " }";
  }
};

// FuncTypeAST 也是 BaseAST
class FuncTypeAST : public BaseAST
{
    public:
        std::string functype;
        void Dump() const override
        {
            std::cout << "FuncTypeAST { ";
            std::cout << "int ";
            std::cout << " }";
        }
};


// BlockAST 也是 BaseAST
class BlockAST : public BaseAST
{
    public:
        std::unique_ptr<BaseAST> stmt;
        void Dump() const override
        {
            std::cout << "BlockAST { ";
            stmt->Dump();
            std::cout << " }";
        }
};

// StmtAST 也是 BaseAST
class StmtAST : public BaseAST
{
    public:
        int number;
        void Dump() const override
        {
            std::cout << "StmtAST { ";
            std::cout <<" "<< number <<" ";
            std::cout << " }";
        }
};




