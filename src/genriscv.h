#pragma once
#include <iostream>
#include <cassert>
#include <cstring>
#include <map>
#include "koopa.h"
using namespace std;
int register_num = 0;
map<koopa_raw_value_t, int> map_reg;

map<koopa_raw_function_t, int> func_sp; // 记录每个函数占据栈的大小
map<koopa_raw_value_t, int> inst_sp; // 记录栈中每条指令的位置
map<koopa_raw_function_t, int> func_ra;
//int cur_sp = 0;
koopa_raw_function_t cur_func;
// 函数声明略
// ...

void Visit(const koopa_raw_program_t &program);
void Visit(const koopa_raw_slice_t &slice);
void Visit(const koopa_raw_function_t &func);
void Visit(const koopa_raw_basic_block_t &bb);
void Visit(const koopa_raw_value_t &value);
void Visit(const koopa_raw_return_t &ret);
void Visit(const koopa_raw_integer_t &integer);
void Visit(const koopa_raw_binary_t &binary);
void genriscv(string koopaIR);

/*void Visit_binary(const koopa_raw_value_t &value);
void Visit_bin_con(const koopa_raw_value_t &value);
void Visit_bin_double_reg(const koopa_raw_value_t &value);*/

void Prologue(const koopa_raw_function_t &func);
void Epilogue(const koopa_raw_function_t &func);
void li_lw(const koopa_raw_value_t &value, string dest_reg);
void sw(const koopa_raw_value_t &dest, string src_reg);
void Visit(const koopa_raw_store_t &store);
void Visit(const koopa_raw_load_t &load);
//string get_reg(const koopa_raw_value_t &value);

void Visit(const koopa_raw_branch_t &branch);
void Visit(const koopa_raw_jump_t &jump);

//全局
void Visit(const koopa_raw_global_alloc_t &global_alloc);
//函数调用
void Visit(const koopa_raw_call_t &call);

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
  int cur_sp = 0; //有返回值的指令大小*4
  int cur_ra = 0; //是否需要保存返回地址
  int cur_param = 0; //参数个数
  int cur_param_size = 0; //参数需要占用内存空间大小 //前八个在寄存器a0-a7
  inst_sp.clear();
  if (func->bbs.len == 0)
  { //如果是函数声明则跳过
        return;
  }
  // 执行一些其他的必要操作
  cout << " .text\n";
  cout << " .globl ";
  string name = func->name;
  cout << name.substr(1) << "\n";
  cout << name.substr(1) << ":\n";
  // cout << "function " << func->name << "visited" << endl;
  for(int i = 0; i < func->bbs.len; ++ i)
  {
    koopa_raw_basic_block_t bb = (koopa_raw_basic_block_t) func->bbs.buffer[i];
    for(int j = 0; j < bb->insts.len; ++ j)
    {
      koopa_raw_value_t inst = (koopa_raw_value_t) bb->insts.buffer[j];
      if(inst->ty->tag != KOOPA_RTT_UNIT) 
      {
        inst_sp[inst] = cur_sp;
        cur_sp += 4;
      }
      if(inst->kind.tag == KOOPA_RVT_CALL)
      {
        func_ra[func] = 4;
        cur_ra = 4;
        int callee_args = inst->kind.data.call.args.len;
        cur_param = max(cur_param, callee_args);
      }
    }
  }

  cur_param_size = max(cur_param_size, (cur_param - 8) * 4);
  cur_sp += cur_param_size;
  cur_sp += cur_ra;

  cur_sp = (cur_sp + 15) / 16 * 16;
  func_sp[func] = cur_sp;
  cur_func = func;

  for(size_t i = 0; i < func->bbs.len; ++ i)
  {
    koopa_raw_basic_block_t bb = (koopa_raw_basic_block_t) func->bbs.buffer[i];
    for(size_t j = 0; j < bb->insts.len; ++ j)
    {
      koopa_raw_value_t inst = (koopa_raw_value_t) bb->insts.buffer[j];
      if(inst->ty->tag != KOOPA_RTT_UNIT) {
        inst_sp[inst] += cur_param_size;
      }
    }
  }//预计算

  Prologue(func);//addi sp,保存返回地址
  Visit(func->bbs);
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb) {
  // 执行一些其他的必要操作
  // ...
  // 访问所有指令
  string re = bb->name +1;
  if(re != "entry") cout<< re << ":\n";
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
     case KOOPA_RVT_BINARY:
      Visit(kind.data.binary);
      sw(value, "t0");
      break;
    case KOOPA_RVT_STORE:
      Visit(kind.data.store);
      break;
    case KOOPA_RVT_LOAD:
      Visit(kind.data.load);//%num = load @var
      sw(value, "t0");     //先把数加载到t0中,然后在sw到%num对应的内存地址中
      break;
    case KOOPA_RVT_ALLOC:
      break;
    case KOOPA_RVT_BRANCH:
      Visit(kind.data.branch);
      break;
    case KOOPA_RVT_JUMP:
      Visit(kind.data.jump);
      break;
    case KOOPA_RVT_GLOBAL_ALLOC:
      cout << "  .data\n" << "  .global " + string(value->name + 1) +  "\n" + string(value->name + 1) + ":\n";
      Visit(kind.data.global_alloc);
      break;
    case KOOPA_RVT_CALL://函数调用
      // call
      Visit(kind.data.call);//调用后返回值存储在a0中
      sw(value, "a0");
      break;
    default:// 其他类型暂时遇不到
      assert(false);
  }
}

void Visit(const koopa_raw_binary_t &binary) {
  li_lw(binary.lhs, "t0");
  li_lw(binary.rhs, "t1");
  if(binary.op == KOOPA_RBO_EQ){
    cout << " xor  t0, t0, t1" << endl;
    cout << " seqz t0, t0" << endl;
  }
  else if(binary.op == KOOPA_RBO_NOT_EQ){
    cout << " xor  t0, t0, t1" << endl;
    cout << " snez t0, t0" << endl;
  }
  else if(binary.op == KOOPA_RBO_GT){
    cout << " sgt  t0, t0, t1" << endl;
  }
  else if(binary.op == KOOPA_RBO_LT){
    cout << " slt  t0, t0, t1" << endl;
  }
  else if(binary.op == KOOPA_RBO_GE){
    cout << " slt  t0, t0, t1" << endl;
    cout << " seqz  t0, t0" << endl;
  }
  else if(binary.op == KOOPA_RBO_LE){
    cout << " sgt  t0, t0, t1" << endl;
    cout << " seqz  t0, t0" << endl;
  }
  else if(binary.op == KOOPA_RBO_ADD){
    cout << " add  t0, t0, t1" << endl;
  }
  else if(binary.op == KOOPA_RBO_SUB){
    cout << " sub  t0, t0, t1" << endl;
  }
  else if(binary.op == KOOPA_RBO_MUL){
    cout << " mul  t0, t0, t1" << endl;
  }
  else if(binary.op == KOOPA_RBO_DIV){
    cout << " div  t0, t0, t1" << endl;
  }
  else if(binary.op == KOOPA_RBO_MOD){
    cout << " rem  t0, t0, t1" << endl;
  }
  else if(binary.op == KOOPA_RBO_AND){
    cout << " and  t0, t0, t1" << endl;
  }
  else if(binary.op == KOOPA_RBO_OR){
    cout << " or  t0, t0, t1" << endl;
  }
 
}
// 访问对应类型指令的函数定义略
// 视需求自行实现
void Visit(const koopa_raw_return_t &ret)
{
    koopa_raw_value_t value = ret.value;
    if(value) li_lw(value, "a0");
    Epilogue(cur_func);
    cout << " ret" << endl;
}

void Visit(const koopa_raw_integer_t &integer)
{
  cout << integer.value;
}

/*
void Visit_binary(const koopa_raw_value_t &value)
{
    // 根据指令类型判断后续需要如何访问
    const auto &kind = value->kind;
    const auto binary = kind.data.binary;
    // 根据运算符类型判断后续如何翻译
    switch (binary.op)
    {
      case KOOPA_RBO_NOT_EQ:
      case KOOPA_RBO_EQ:
      case KOOPA_RBO_GT:
      case KOOPA_RBO_LT:
      case KOOPA_RBO_GE:
      case KOOPA_RBO_LE:
      case KOOPA_RBO_AND:
      case KOOPA_RBO_OR:
        Visit_bin_con(value);
        break;
      case KOOPA_RBO_ADD:
      case KOOPA_RBO_SUB:
      case KOOPA_RBO_MUL:
      case KOOPA_RBO_DIV:
      case KOOPA_RBO_MOD:
        Visit_bin_double_reg(value);
        break;
      default:
        assert(false);
    }
}

void Visit_bin_con(const koopa_raw_value_t &value)
{ 
    //cout << " condition 指令" << endl;
    const auto &kind = value->kind;
    const auto binary = kind.data.binary;
    string leftreg, rightreg;                            // 左右节点值
    //cout<<"\n现在"<<register_num<<"\n";
    map_reg[value] = register_num++;                     // 为当前指令分配一个寄存器
    string eqregister = get_reg(value); // 定义当前指令的寄存器
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
        assert(binary.lhs->kind.tag != KOOPA_RVT_INTEGER); // assert左节点不是int值
        leftreg = get_reg(binary.lhs);    //获取左边的寄存器
    }

    if (binary.rhs->kind.tag == KOOPA_RVT_INTEGER)
    { //处理右节点,右节点为int值且为0
        if (binary.rhs->kind.data.integer.value == 0)
        { //右节点为0
            rightreg = "x0";
        }
        else
        { //右节点为非0 int, li到当前寄存器
            if(binary.lhs->kind.tag == KOOPA_RVT_INTEGER && binary.lhs->kind.data.integer.value != 0)
            {
                map_reg[value] = register_num++;              // 为当前指令再分配一个寄存器
                eqregister = get_reg(value); // 定义当前指令的寄存器
            }
            cout << "  li\t" << eqregister << ", ";
            Visit(binary.rhs);
            cout << endl;
            rightreg = eqregister;
        }
    }
    else
    {
        assert(binary.rhs->kind.tag != KOOPA_RVT_INTEGER); // assert左右节点不是int值
        rightreg = get_reg(binary.rhs);   //获取右边的寄存器
    }

    switch (binary.op) // 根据指令类型判断当前指令的运算符
    {
    case KOOPA_RBO_EQ:
        //使用xor和seqz指令完成 等值 判断
        cout << "  xor\t" + eqregister + ", " + leftreg + ", " + rightreg + '\n';
        cout << "  seqz\t" + eqregister + ", " + eqregister + '\n';
        break;
    case KOOPA_RBO_NOT_EQ:
        //使用xor和snez指令完成 不等 判断
        cout << "  xor\t" + eqregister + ", " + leftreg + ", " + rightreg + '\n';
        cout << "  snez\t" + eqregister + ", " + eqregister + '\n';
        break;
    case KOOPA_RBO_OR:
        //使用or和snez指令完成 或 判断
        cout << "  or\t" + eqregister + ", " + leftreg + ", " + rightreg + '\n';
        //cout << "  snez\t" + eqregister + ", " + eqregister + '\n';
        break;
    case KOOPA_RBO_GT:
        //大于使用 slt 指令,交换两操作数位置,一条语句直接结束
        cout << "  slt\t" + eqregister + ", " + rightreg + ", " + leftreg + '\n';
        break;
    case KOOPA_RBO_LT:
        //小于使用 slt 指令,一条语句直接结束
        cout << "  slt\t" + eqregister + ", " + leftreg + ", " + rightreg + '\n';
        break;
    case KOOPA_RBO_GE:
        //>= 大于等于 先判断反命题: slt 判断 左 < 右, 再用异或 '1' 得到原命题
        cout << "  slt\t" + eqregister + ", " + leftreg + ", " + rightreg + '\n';
        cout << "  xori\t" + eqregister + ", " + eqregister + ", 1\n";
        break;
    case KOOPA_RBO_LE:
        //<= 小于等于,交换量操作数后, 与上面操作一致
        cout << "  slt\t" + eqregister + ", " + rightreg + ", " + leftreg + '\n';
        cout << "  xori\t" + eqregister + ", " + eqregister + ", 1\n";
        break;
    case KOOPA_RBO_AND:
        // and使用三条指令
        //cout << "  snez\t" + leftreg + ", " + leftreg + '\n';
        //cout << "  snez\t" + rightreg + ", " + rightreg + '\n';
        cout << "  and\t" + eqregister + ", " + leftreg + ", " + rightreg + '\n';
        break;
    default: // 其他类型暂时遇不到
        assert(false);
    }
    
}

void Visit_bin_double_reg(const koopa_raw_value_t &value)
{
    //cout << " 算术指令" << endl;
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
    //cout<<"\n现在"<<register_num<<"\n";
    map_reg[value] = register_num++;                     // 为当前指令分配一个寄存器
    string eqregister = get_reg(value); // 定义当前指令的寄存器

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
        assert(binary.lhs->kind.tag != KOOPA_RVT_INTEGER); // assert左节点不是int值
        leftreg = get_reg(binary.lhs);    //获取左边的寄存器
    }

    if (binary.rhs->kind.tag == KOOPA_RVT_INTEGER)
    { //处理右节点,右节点为int值且为0
        if (binary.rhs->kind.data.integer.value == 0)
        { //右节点为0
            rightreg = "x0";
        }
        else
        { //右节点为非0 int, li到当前寄存器
            if(binary.lhs->kind.tag == KOOPA_RVT_INTEGER && binary.lhs->kind.data.integer.value != 0)
            {
                map_reg[value] = register_num++;              // 为当前指令再分配一个寄存器
                eqregister = get_reg(value); // 定义当前指令的寄存器
            }
            cout << "  li\t" << eqregister << ", ";
            Visit(binary.rhs);
            cout << endl;
            rightreg = eqregister;
        }
    }
    else
    {
        assert(binary.rhs->kind.tag != KOOPA_RVT_INTEGER); // assert左右节点不是int值
        rightreg = get_reg(binary.rhs);   //获取右边的寄存器
    }
    cout << "  " + bin_op + '\t' + eqregister + ", " + leftreg + ", " + rightreg + "\n";
}



string get_reg(const koopa_raw_value_t &value)
{
    if(map_reg[value] > 6)
    {                                               //当t0~t6用完时
        return "a" + to_string(map_reg[value] - 7); // 用a0~a7
    }
    else
    {
        return "t" + to_string(map_reg[value]); // 获得当前指令的寄存器
    }
}*/

void Prologue(const koopa_raw_function_t &func){
  int sp = func_sp[func];
  int ra = func_ra[func];
  if(sp)
  {
    cout << " addi  sp, sp, " + to_string(-sp) << endl;
  }
  if(ra)
  {
    cout << " sw ra, " + to_string(sp-4) + "(sp)\n";
  }
}

void Epilogue(const koopa_raw_function_t &func){
  int sp = func_sp[func];
  int ra = func_ra[func];
  if(ra)
  {
      cout << " lw ra, " + to_string(sp-4) + "(sp)\n";
  }
  if(sp)
  {
    cout << " addi  sp, sp, " + to_string(sp) << endl;
  }
}

void li_lw(const koopa_raw_value_t &value, string dest_reg="t0"){
  if(inst_sp.find(value) == inst_sp.end())//常数或者全局变量
  {
    if(value->kind.tag == KOOPA_RVT_INTEGER)
    {
      int number;
      number = value->kind.data.integer.value;
      cout << " li " << dest_reg << ", " << to_string(number) << endl;
    }
    else if(value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
    {
      string var_name = value->name;
      cout << " la " << dest_reg << ", " << var_name.substr(1) << "\n";
      cout << " lw " << dest_reg << ", " << "0(" << dest_reg << ")" << endl;
    }
  }
  else
  { // 不是立即数，%0是指针指向对应指令
    int cur_inst_sp = inst_sp[value];
    cout << " lw " << dest_reg << ", " << to_string(cur_inst_sp) << "(sp)" << endl;
  }
}

void sw(const koopa_raw_value_t &dest, string src_reg="t0"){
  if(inst_sp.find(dest) == inst_sp.end())
  {
    if(dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
    {
      string var_name = dest->name;
      cout << " la t6, ";
      for(int i = 1; i < var_name.size(); ++ i)
      {
        cout << var_name[i];
      }
      cout << endl;
      cout << " sw " << src_reg << ", " << "0(t6)" << endl;
    }
  }
  else
  {
    int cur_inst_sp = inst_sp[dest];
    cout << " sw " << src_reg << ", " << to_string(cur_inst_sp) << "(sp)" << endl;
  }
}

void Visit(const koopa_raw_store_t &store){//store 临时/常量 变量
  koopa_raw_value_t value = store.value;
  koopa_raw_value_t dest = store.dest;
  if(value->kind.tag == KOOPA_RVT_FUNC_ARG_REF)
  {
    int arg_index = value->kind.data.func_arg_ref.index;
    if(arg_index < 8)
    {
      string src_reg = "a" + to_string(arg_index);
      sw(dest, src_reg);
    }
    else
    {
      // 高地址到低地址长，caller栈存的参数需要到callee栈顶（最大地址）+参数序号-8
      int arg_stack = func_sp[cur_func] + (arg_index - 8)*4;
      cout << " lw t0, " + to_string(arg_stack)+"(sp)\n";
      sw(dest, "t0");
    }
  }
  else
  {
    li_lw(value, "t0");
    sw(dest, "t0");
  }
}

void Visit(const koopa_raw_load_t &load){//临时 = load 变量
  koopa_raw_value_t src = load.src;
  li_lw(src, "t0");//先加载到t0
}

void Visit(const koopa_raw_branch_t &branch) {
  string label_true = branch.true_bb->name + 1;
  string label_false = branch.false_bb->name + 1;
  li_lw(branch.cond, "t0");
  cout << "  bnez t0, "  << label_true << endl;
  cout << "  j " << label_false << endl;
}

void Visit(const koopa_raw_jump_t &jump) {
  string label_target = jump.target->name + 1;
  cout << "  j " << label_target << endl;
}

void Visit(const koopa_raw_global_alloc_t &global_alloc){
  koopa_raw_value_t init = global_alloc.init;
  if(init->kind.tag == KOOPA_RVT_INTEGER){
    cout << " .word ";
    cout << to_string(init->kind.data.integer.value) << endl;
  }else if(init->kind.tag == KOOPA_RVT_ZERO_INIT){
    cout << " .zero ";
    cout << to_string(4) << endl;
  }
}

void Visit(const koopa_raw_call_t &call){
  koopa_raw_slice_t call_args = call.args;
  koopa_raw_function_t callee = call.callee;
  int args_num = call_args.len;
  for(int i = 0; i < args_num; ++ i){
    if(i < 8){
      string dest_reg = "a" + to_string(i);
      koopa_raw_value_t cur_arg = (koopa_raw_value_t) call_args.buffer[i];
      li_lw(cur_arg, dest_reg);
    }
    else{
      string dest_stack = to_string((i - 8) * 4) + "(sp)";
      koopa_raw_value_t cur_arg = (koopa_raw_value_t) call_args.buffer[i];
      li_lw(cur_arg, "t0");
      cout << " sw t0, " << dest_stack << endl;
    }
  }
  cout << " call " + string(callee->name + 1) + "\n";
}