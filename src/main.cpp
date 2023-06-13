#include <cassert>
#include <cstdio>
#include <iostream>
//#include <map>
#include <memory>
#include <string>
#include <cstring>
#include "ast.h"
#include "genriscv.h"
using namespace std;
int cnt = -1;//虚拟寄存器个数
int cnt2 = -1;
int STnum = 0;//符号表个数
int IFnum = 0;//if个数
int WHILEnum = 0;//while个数
int SHORT = 0;
map<string,string> globalF;//函数类型
ST *cur_st = new ST;//当前符号表
WT *cur_wh = NULL;//while表
// 声明 lexer 的输入, 以及 parser 函数
// 为什么不引用 sysy.tab.hpp 呢? 因为首先里面没有 yyin 的定义
// 其次, 因为这个文件不是我们自己写的, 而是被 Bison 生成出来的
// 你的代码编辑器/IDE 很可能找不到这个文件, 然后会给你报错 (虽然编译不会出错)
// 看起来会很烦人, 于是干脆采用这种看起来 dirty 但实际很有效的手段
extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);
bool check(string s)
{
  for(int i = s.size()-1; s[i]!=':'; i--)
  {
    if(s.substr(i,3) == "ret") return true;
    if(s.substr(i,4) == "jump") return true;
  }
  return false;
}
int main(int argc, const char *argv[])
{
  cur_st->num = 0;
  cur_st->fa = NULL;

  // 解析命令行参数. 测试脚本/评测平台要求你的编译器能接收如下参数:
  // compiler 模式 输入文件 -o 输出文件
  assert(argc == 5);
  auto mode = argv[1];
  auto input = argv[2];
  auto output = argv[4];

  // 打开输入文件, 并且指定 lexer 在解析的时候读取这个文件
  yyin = fopen(input, "r");
  assert(yyin);
  // parse input file

  unique_ptr<BaseAST> ast;

  auto ret = yyparse(ast);
  assert(!ret);
  //return 0;
  // dump koopa

  string koopaIR = "";
  koopaIR += "decl @getint(): i32\n";
  koopaIR += "decl @getch(): i32\n";
  koopaIR += "decl @getarray(*i32): i32\n";
  koopaIR += "decl @putint(i32)\n";
  koopaIR += "decl @putch(i32)\n";
  koopaIR += "decl @putarray(i32, *i32)\n";
  koopaIR += "decl @starttime()\n";
  koopaIR += "decl @stoptime()\n";
  globalF["getint"] = "int";
  globalF["putint"] = "void";
  globalF["getch"]  = "int";
  globalF["putch"]  = "void";
  ast->cal(koopaIR);
  cout << endl;

  freopen(output, "w", stdout);
  if (mode[1] == 'k') // koopa
  {
    std::cout << koopaIR;
  }
  else // risc-v
  {
    genriscv(koopaIR);
  }
  fclose(stdout);
  return 0;
}
