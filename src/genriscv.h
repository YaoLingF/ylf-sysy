#pragma once
#include <iostream>
#include <cassert>
#include <cstring>
#include <map>
#include "koopa.h"
using namespace std;
int register_num = 0;
map<koopa_raw_value_t, int> map_reg;
// 函数声明略
// ...
void genriscv();



void Visit(const koopa_raw_program_t &program);
void Visit(const koopa_raw_slice_t &slice);
void Visit(const koopa_raw_function_t &func);
void Visit(const koopa_raw_basic_block_t &bb);
void Visit(const koopa_raw_value_t &value);
void Visit(const koopa_raw_return_t &ret);
void Visit(const koopa_raw_integer_t &integer);
void Visit_binary(const koopa_raw_value_t &value);
void Visit_bin_eq(const koopa_raw_value_t &value);
void Visit_bin_double_reg(const koopa_raw_value_t &value);

void genriscv(string koopaIR)
{ 
  const char* str = koopaIR.c_str();
  // 解析字符串 str, 得到 Koopa IR 程序
  koopa_program_t program;
  koopa_error_code_t ret2 = koopa_parse_from_string(str, &program);
  assert(ret2 == KOOPA_EC_SUCCESS);  // 确保解析时没有出错
  // 创建一个 raw program builder, 用来构建 raw program
  koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
  // 将 Koopa IR 程序转换为 raw program
  koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
  // 释放 Koopa IR 程序占用的内存
  koopa_delete_program(program);

  // 处理 raw program
  // ..

  Visit(raw);

   // 处理完成, 释放 raw program builder 占用的内存
  // 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
  // 所以不要在 raw program 处理完毕之前释放 builder
  koopa_delete_raw_program_builder(builder);
}
void Visit(const koopa_raw_program_t &program) 
{
  // 执行一些其他的必要操作
  cout << " .text\n";
  Visit(program.values);//全局变量列表
  Visit(program.funcs);//函数列表
}

// 访问 raw slice
void Visit(const koopa_raw_slice_t &slice) 
{
  for (size_t i = 0; i < slice.len; ++i) 
  {
    auto ptr = slice.buffer[i];
    // 根据 slice 的 kind 决定将 ptr 视作何种元素
    switch (slice.kind) 
    {
      case KOOPA_RSIK_FUNCTION:
        // 访问函数
        Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
        break;
      case KOOPA_RSIK_BASIC_BLOCK:
        // 访问基本块
        Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
        break;
      case KOOPA_RSIK_VALUE:
        // 访问指令
        Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
        break;
      default:
        // 我们暂时不会遇到其他内容, 于是不对其做任何处理
        assert(false);
    }
  }
}

// 访问函数
void Visit(const koopa_raw_function_t &func) 
{
  // 执行一些其他的必要操作
  cout << " .globl ";
  string name = func->name;
  cout << name.substr(1) << "\n";
  cout << name.substr(1) << ":\n";
  // cout << "function " << func->name << "visited" << endl;
  Visit(func->bbs);
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb) {
  // 执行一些其他的必要操作
  // ...
  // 访问所有指令
  Visit(bb->insts);
}

// 访问指令
void Visit(const koopa_raw_value_t &value) {
  // 根据指令类型判断后续需要如何访问
  const auto &kind = value->kind;
  switch (kind.tag) {
    case KOOPA_RVT_RETURN:
      // 访问 return 指令
      Visit(kind.data.ret);
      break;
    case KOOPA_RVT_INTEGER:
      // 访问 integer 指令
      Visit(kind.data.integer);
      break;
    case KOOPA_RVT_BINARY://运算指令,哈希value代表将结果所在的寄存器哈希,用于以后调用
      Visit_binary(value);
      break;
    default:
      // 其他类型暂时遇不到
      assert(false);
  }
}

// 访问对应类型指令的函数定义略
// 视需求自行实现
void Visit(const koopa_raw_return_t &ret)
{
    koopa_raw_value_t retval = ret.value;
    const auto &kind = retval->kind;
    if (kind.tag == KOOPA_RVT_INTEGER)
    {                            // value指向int 数值
        cout << "  li\ta0, "; //访问指令返回后面跟的为int数值, 直接放入a0中
        Visit(ret.value);
        cout << "\n";
    }
    else
    { // value指向了上一条指令的结果寄存器
        cout << "  mv\ta0, t" + to_string(map_reg[ret.value]) + "\n"; //将上一条指令的结果寄存器写入其中
        // 写入
        //cout << "return's last instrucions register=" << map_reg[ret.value] << endl;
    }
    cout << "  ret" << endl;
}

void Visit(const koopa_raw_integer_t &integer)
{
  cout << integer.value;
}


void Visit_binary(const koopa_raw_value_t &value)
{
    // 根据指令类型判断后续需要如何访问
    const auto &kind = value->kind;
    const auto binary = kind.data.binary;
    // 根据运算符类型判断后续如何翻译
    switch (binary.op)
    {
    case KOOPA_RBO_EQ:
        // outfile << "  # eq\n";
        Visit_bin_eq(value);
        break;
    case KOOPA_RBO_ADD:
        // outfile << "  # add\n";
        Visit_bin_double_reg(value);
        break;
    case KOOPA_RBO_SUB:
        // outfile << "  # sub\n";
        Visit_bin_double_reg(value);
        break;
    case KOOPA_RBO_MUL:
        // outfile << "  # mul\n";
        Visit_bin_double_reg(value);
        break;
    case KOOPA_RBO_DIV:
        // outfile << "  # div\n";
        Visit_bin_double_reg(value);
        break;
    case KOOPA_RBO_MOD:
        // outfile << "  # mod\n";
        Visit_bin_double_reg(value);
        break;
    default:
        // 其他类型暂时遇不到
        assert(false);
    }
    return;
}

void Visit_bin_eq(const koopa_raw_value_t &value)
{ 
    //cout << " eq 指令" << endl;
    const auto &kind = value->kind;
    const auto binary = kind.data.binary;
    string leftreg, rightreg;                            // 左右节点值
    map_reg[value] = register_num++;                     // 为当前指令分配一个寄存器
    string eqregister = "t" + to_string(map_reg[value]); // 定义当前指令的寄存器
    if (binary.lhs->kind.tag == KOOPA_RVT_INTEGER)
    { //先处理左节点,左节点为int值
        if (binary.lhs->kind.data.integer.value != 0)
        { //左节点为非0结点, 用li指令到当前寄存器
            cout << "  li\t" << eqregister << ", ";
            Visit(binary.lhs);
            cout << endl;
            leftreg = eqregister;
        }
        else
        { //左节点为0值
            leftreg = "x0";
        }
    }
    else
    {
        leftreg = "t" + to_string(map_reg[binary.lhs]); //获取左边的寄存器
    }
    if (binary.rhs->kind.tag == KOOPA_RVT_INTEGER)
    { //处理右节点,右节点为int值
        if (binary.rhs->kind.data.integer.value != 0)
        { //右节点为非0结点, 用xori立即数指令
            cout << "  xori\t" + eqregister + ", " + leftreg + ", ";
            Visit(binary.rhs);
            cout << endl;
        }
        else
        { //右节点为0值, 用x0
            cout << "  xor\t" + eqregister + ", " + leftreg + ", x0" + '\n';
        }
    }
    else
    {
        rightreg = "t" + to_string(map_reg[binary.lhs]); //获取右边的寄存器
        cout << "  xor\t" + eqregister + ", " + leftreg + ", " + rightreg + '\n';
    }
    cout << "  seqz\t" + eqregister + ", " + eqregister + '\n';
    return;
}

void Visit_bin_double_reg(const koopa_raw_value_t &value)
{
    //cout << " 二者均为寄存器的指令" << endl;
    const auto &kind = value->kind;
    const auto binary = kind.data.binary;
    string bin_op; //当前指令的运算符
    switch (binary.op)
    { // 根据指令类型判断当前指令的运算符
    case KOOPA_RBO_ADD:
        bin_op = "add";
        break;
    case KOOPA_RBO_SUB:
        bin_op = "sub";
        break;
    case KOOPA_RBO_MUL:
        bin_op = "mul";
        break;
    case KOOPA_RBO_DIV:
        bin_op = "div";
        break;
    case KOOPA_RBO_MOD:
        bin_op = "rem";
        break;
    default: // 其他类型暂时遇不到
        assert(false);
    }
    string leftreg, rightreg;                            // 左右节点值
    map_reg[value] = register_num++;                     // 为当前指令分配一个寄存器
    string eqregister = "t" + to_string(map_reg[value]); // 定义当前指令的寄存器

    if (binary.lhs->kind.tag == KOOPA_RVT_INTEGER)
    { //先处理左节点,左节点为int值
        if (binary.lhs->kind.data.integer.value == 0)
        { //左节点为0
            leftreg = "x0";
        }
        else
        { //左节点为非0 int, li到当前寄存器
            cout << "  li\t" << eqregister << ", ";
            Visit(binary.lhs);
            cout << endl;
            leftreg = eqregister;
        }
    }
    else
    {
        assert(binary.lhs->kind.tag != KOOPA_RVT_INTEGER); // assert左右节点不是int值
        leftreg = "t" + to_string(map_reg[binary.lhs]);    //获取左边的寄存器
    }

    if (binary.rhs->kind.tag == KOOPA_RVT_INTEGER)
    { //处理右节点,右节点为int值且为0
        if (binary.rhs->kind.data.integer.value == 0)
        { //右节点为0
            rightreg = "x0";
        }
        else
        { //右节点为非0 int, li到当前寄存器
            map_reg[value] = register_num++;              // 为当前指令再分配一个寄存器
            eqregister = "t" + to_string(map_reg[value]); // 定义当前指令的寄存器
            cout << "  li\t" << eqregister << ", ";
            Visit(binary.rhs);
            cout << endl;
            rightreg = eqregister;
        }
    }
    else
    {
        assert(binary.rhs->kind.tag != KOOPA_RVT_INTEGER); // assert左右节点不是int值
        rightreg = "t" + to_string(map_reg[binary.rhs]);   //获取右边的寄存器
    }
    cout << "  " + bin_op + '\t' + eqregister + ", " + leftreg + ", " + rightreg + "\n";
}