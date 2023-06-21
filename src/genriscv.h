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
map<koopa_raw_function_t, int> func_ra; //函数是否需要保存ra
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
void Visit_global_alloc(const koopa_raw_value_t &value);
void visit_aggregate(const koopa_raw_value_t &aggregate);
//函数调用
void Visit(const koopa_raw_call_t &call);
void visit_getptr(const koopa_raw_value_t &getptr);
void visit_getelemptr(const koopa_raw_value_t &getelemptr);

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
  { //如果是函数声明则跳过 Sysy库函数
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
      if(inst->ty->tag != KOOPA_RTT_UNIT) //有返回 alloc load 算术运算
      {
        inst_sp[inst] = cur_sp;
        if(inst->kind.tag == KOOPA_RVT_ALLOC) //可能是 *i32 *[i32,5] 甚至多维
        {
          auto kind = inst->ty->data.pointer.base; //获取当前 指针的base
          int arraysize = 1;                        //初始化数组长度
          while (kind->tag == KOOPA_RTT_ARRAY)
          { //初始化数值时此部分跳过
            //如果当前 kind 指向的为数组
            int cursize = kind->data.array.len; //获取当前维度的长度
            arraysize *= cursize;
            kind = kind->data.array.base; //获取当前数组的base
          }
          cur_sp += 4 * arraysize;
        }
        else//非alloc
        {
          cur_sp += 4;
        }
      }
      if(inst->kind.tag == KOOPA_RVT_CALL)//函数调用,涉及到传参数
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
    case KOOPA_RVT_ALLOC://局部alloc 所占空间已经；预处理了，这里没有语句
      break;
    case KOOPA_RVT_BRANCH:
      Visit(kind.data.branch);
      break;
    case KOOPA_RVT_JUMP:
      Visit(kind.data.jump);
      break;
    case KOOPA_RVT_GLOBAL_ALLOC:
      cout << "  .data\n" << "  .global " + string(value->name + 1) +  "\n" + string(value->name + 1) + ":\n";
      Visit_global_alloc(value);
      break;
    case KOOPA_RVT_CALL://函数调用
      Visit(kind.data.call);//调用后返回值存储在a0中
      sw(value, "a0");
      break;
    case KOOPA_RVT_AGGREGATE:
      visit_aggregate(value);
      break;
    case KOOPA_RVT_GET_ELEM_PTR:
      visit_getelemptr(value);
      break;
    case KOOPA_RVT_GET_PTR:
      visit_getptr(value);
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
    if (cur_inst_sp < 2047)
    { //如果srcstack 的size 小于 12 位 最大为+2047
        cout << "  lw " + dest_reg + ", " + to_string(cur_inst_sp) + "(sp)" + "\n";
    }
    else
    {
        cout << "  li " + dest_reg + ", " + to_string(cur_inst_sp) + "\n";
        cout << "  add " + dest_reg + ", sp, " + dest_reg + "\n";
        cout << "  lw " + dest_reg + ", " + "0(" + dest_reg + ")\n";
    }
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
    if (cur_inst_sp < 2047)
    { //如果srcstack 的size 小于 12 位 最大为+2047
        cout << "  sw " + src_reg + ", " + to_string(cur_inst_sp) + "(sp)" + "\n";
    }
    else
    {
        cout << "  li t3, " + to_string(cur_inst_sp) + "\n";
        cout << "  add t3, sp, t3\n";
        cout << "  sw " + src_reg + ", 0(t3)\n";
    }
  }
}

void Visit(const koopa_raw_store_t &store){//store 临时/常量 地址
  koopa_raw_value_t value = store.value;
  koopa_raw_value_t dest = store.dest;
  string store_value_reg = "t0";
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
    //sw(dest, "t0");
    string global_name, dest_stack;
    string dest_reg = "t1";
    switch (store.dest->kind.tag)
    {
    case KOOPA_RVT_GLOBAL_ALLOC:
        //如果存入的位置是全局变量
        // 1. 再申请一个变量计算存入的内存位置
        // 先获取全局变量名称
        sw(dest, "t0");
        break;
    case KOOPA_RVT_ALLOC:
        //如果存入的位置是局部变量
        sw(dest, "t0");
        break;
    case KOOPA_RVT_GET_ELEM_PTR:
        //如果是 getelemptr 型
        //将指针对应stack中存放的内容取出 ： 指针指向的地址
        li_lw(store.dest,dest_reg);
        //将内容从存入 地址对应的内存位置
        cout << "  sw " + store_value_reg + ", 0(" + dest_reg + ")" << endl;
        break;
    case KOOPA_RVT_GET_PTR:
        //如果是getptr型
        li_lw(store.dest,dest_reg);
        //将内容从存入 地址对应的内存位置
        cout << "  sw " + store_value_reg + ", 0(" + dest_reg + ")" << endl;
        break;
    default:
        cout << "store.dest->kind.tag: " << store.dest->kind.tag << endl;

        std::cerr << "程序错误：store dest 类型不符合预期" << std::endl;
        break;
    }

  }
}

void Visit(const koopa_raw_load_t &load){//临时 = load 变量
    koopa_raw_value_t src = load.src;
    switch (load.src->kind.tag)
    {
    case KOOPA_RVT_INTEGER:
        //如果是integer型
        li_lw(src, "t0");
        break;
    case KOOPA_RVT_ALLOC:
        //如果是alloc型
        li_lw(src, "t0");
    case KOOPA_RVT_GLOBAL_ALLOC:
        //如果是global alloc型
        li_lw(src, "t0");
        break;
    case KOOPA_RVT_GET_ELEM_PTR:
        //如果是 getelemptr 型
        // 1. 将其指针的地址中的内容取出
        li_lw(src, "t0");
        //再将其load进来
        cout << "  lw t0, 0(t0)\n";
        break;
    case KOOPA_RVT_GET_PTR:
        //如果是 getptr 型
        // 1. 将其指针的地址中的内容取出
        li_lw(src, "t0");
        //再将其load进来
        cout << "  lw t0, 0(t0)\n";
        break;
    default:
        cout << "load.src->kind.tag = " << load.src->kind.tag << endl;
        std::cerr << "程序错误：load 指令的目标类型不符合预期" << std::endl;
        break;
    }
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

void Visit_global_alloc(const koopa_raw_value_t &value)
{
    const auto &init = value->kind.data.global_alloc.init;
    // cout << init->kind.tag << endl;
    if (init->kind.tag == KOOPA_RVT_ZERO_INIT)
    { //如果初始值为zeroinit
        if (value->ty->tag == KOOPA_RTT_POINTER)//??
        {                      //如果用zeroinit初始化全0数组
            int arraysize = 1; //初始化数组长度
            auto kind = value->ty->data.pointer.base;
            while (kind->tag == KOOPA_RTT_ARRAY)
            { //初始化数值时此部分跳过
                //如果当前 kind 指向的为数组
                int cursize = kind->data.array.len; //获取当前维度的长度
                arraysize *= cursize;
                kind = kind->data.array.base; //获取当前数组的base
            }
            arraysize *= 4;
            // int sizeofarray = 4 * (value->ty->data.pointer.base->data.array.len);
            cout << "  .zero " + to_string(arraysize) + "\n";
        }
        else
        {
            cout << ".zero" << value->ty->tag << endl;
            cout << "  .zero 4\n\n";
        }
    }
    else if (init->kind.tag == KOOPA_RVT_INTEGER)//非数组并且非0
    { //是数值初始
        int num = init->kind.data.integer.value;
        cout << "  .word " + to_string(num) + "\n";
    }
    else if (init->kind.tag == KOOPA_RVT_AGGREGATE)//一堆{{}}
    { // 用 aggregate初始化数组
        // 暂时处理一维数组情况
        // 访问aggregate
        Visit(init);
    }
    cout << "\n";

}

void visit_aggregate(const koopa_raw_value_t &aggregate)
{
    auto elems = aggregate->kind.data.aggregate.elems;
    for (size_t i = 0; i < elems.len; ++i)
    {
        auto ptr = elems.buffer[i];
        // 根据 elems 的 kind 决定将 ptr 视作何种元素，同一 elems 中存放着所有 elem 为同一类型
        if (elems.kind == KOOPA_RSIK_VALUE)
        { //如果当前的 elem 为 value 类型
            auto elem = reinterpret_cast<koopa_raw_value_t>(ptr);
            if (elem->kind.tag == KOOPA_RVT_AGGREGATE)
            { //如果是 aggregate 型则循环嵌套访问
                visit_aggregate(elem);
            }
            else if (elem->kind.tag == KOOPA_RVT_INTEGER)
            { //如果是 integer 型 则用 .word + int
                cout << "  .word ";
                cout << to_string(elem->kind.data.integer.value) << "\n";
            }
            else if (elem->kind.tag == KOOPA_RVT_ZERO_INIT)
            { //如果是zeroinit类型
                auto kind = elem->ty->data.pointer.base;
                int arraysize = 1; //初始化数组长度
                while (kind->tag == KOOPA_RTT_ARRAY)
                { //初始化数值时此部分跳过
                    //如果当前 kind 指向的为数组
                    int cursize = kind->data.array.len; //获取当前维度的长度
                    arraysize *= cursize;
                    kind = kind->data.array.base; //获取当前数组的base
                }
                arraysize *= 4;
                cout << "  .zero " + to_string(arraysize) << endl;
            }
        }
        else
        {
            cout << "elem.kind = " << elems.kind << endl;
            std::cerr << "程序错误： aggregate 的elem 类型不符合预期" << std::endl;
        }
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

void Prologue(const koopa_raw_function_t &func){
  int sp = func_sp[func];
  int ra = func_ra[func];
  if(sp)
  {
    if(sp > 2047)
    {
      cout << "  li t0, " + to_string(-sp) + "\n";
      cout << "  add sp, sp, t0\n";
    }
    else
    {
      cout << " addi  sp, sp, " + to_string(-sp) << endl;
    }
  }
  if(ra)
  {
    if(sp > 2047)
    {
      cout << "  li t0, " + to_string(sp - 4) + "\n";
      cout << "  add t0, sp, t0\n";
      cout << "  sw ra, 0(t0)\n";
    }
    else
    {
      cout << " sw ra, " + to_string(sp-4) + "(sp)\n";
    }
  }
}

void Epilogue(const koopa_raw_function_t &func){
  int sp = func_sp[func];
  int ra = func_ra[func];
  if(ra)
  {
    if(sp > 2047)
    {
      cout << "  li t0, " + to_string(sp - 4) + "\n";
      cout << "  add t0, sp, t0\n";
      cout << "  lw ra, 0(t0)\n";
    }
    else
    {
      cout << " lw ra, " + to_string(sp-4) + "(sp)\n";
    }
  }
  if(sp)
  {
    if(sp > 2047)
    {
      cout << "  li t0, " + to_string(sp) + "\n";
      cout << "  add sp, sp, t0\n";
    }
    else
    {
      cout << " addi  sp, sp, " + to_string(sp) << endl;
    }
  }
  
}

void visit_getelemptr(const koopa_raw_value_t &getelemptr)
{
    auto src = getelemptr->kind.data.get_elem_ptr.src;
    auto index = getelemptr->kind.data.get_elem_ptr.index;

    auto kind = getelemptr->ty->data.pointer.base;
    int arraysize = 1; //初始化数组长度
    while (kind->tag == KOOPA_RTT_ARRAY)
    { //初始化数值时此部分跳过
        //如果当前 kind 指向的为数组
        int cursize = kind->data.array.len; //获取当前维度的长度
        arraysize *= cursize;
        kind = kind->data.array.base; //获取当前数组的base
    }

    string src_reg = "t0"; //本行的src地址所用的寄存器
    // 1. 计算 src 的地址
    if (src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
    { //如果当前src为全局变量分配
        // 先获取全局变量名称
        string global_name = src->name;
        // 去掉@符号
        global_name = global_name.substr(1);
        cout << "  la\t" + src_reg + ", " + global_name << endl;
    }
    else if (src->kind.tag == KOOPA_RVT_ALLOC) //如果当前指向的是局部变量
    {
        int srcstack = inst_sp[src]; // src 对应的stack位置
        if (srcstack < 2047)
        { //如果srcstack 的size 小于 12 位 最大为+2047
            cout << "  addi\t" + src_reg + ", sp, " + to_string(srcstack) << endl;
        }
        else
        {
            cout << "  li\t" + src_reg + ", " + to_string(srcstack) << endl;
            cout << "  add\t" + src_reg + ", sp, " + src_reg << endl;
        }
    }
    else if (src->kind.tag == KOOPA_RVT_GET_ELEM_PTR)
    { //如果当前指向的是指针 getelemptr 将里面的内容load进来
        int srcstack = inst_sp[src]; // src 对应的stack位置
        if (srcstack < 2047)
        { //如果srcstack 的size 小于 12 位 最大为+2047
            cout << "  lw " + src_reg + ", " + to_string(srcstack) + "(sp)" + "\n";
        }
        else
        {
            cout << "  li " + src_reg + ", " + to_string(srcstack) + "\n";
            cout << "  add " + src_reg + ", sp, " + src_reg + "\n";
            cout << "  lw " + src_reg + ", " + "0(" + src_reg + ")\n";
        }
    }
    else if (src->kind.tag == KOOPA_RVT_GET_PTR)
    { //如果当前指向的是指针 getptr 将里面的内容load进来
        int srcstack = inst_sp[src]; // src 对应的stack位置
        if (srcstack < 2047)
        { //如果srcstack 的size 小于 12 位 最大为+2047
            cout << "  lw " + src_reg + ", " + to_string(srcstack) + "(sp)" + "\n";
        }
        else
        {
            cout << "  li " + src_reg + ", " + to_string(srcstack) + "\n";
            cout << "  add " + src_reg + ", sp, " + src_reg + "\n";
            cout << "  lw " + src_reg + ", " + "0(" + src_reg + ")\n";
        }
    }
    else
    {
        cout << "src->kind.tag = " << src->kind.tag << endl;
        std::cerr << "程序错误： getelemptr 的 src 类型不符合预期" << std::endl;
    }

    // 2. 获得 index 的大小
    string indexreg = "t1";
    li_lw(index,indexreg);

    string size_reg = "t2";
    arraysize *= 4;
    cout << "  li " + size_reg + ", " + to_string(arraysize) + "\n"; //当前指针的大小
    cout << "  mul " + indexreg + ", " + indexreg + ", " + size_reg + "\n";

    // 3. 计算 getelemptr 的结果
    cout << "  add " + src_reg + ", " + src_reg + ", " + indexreg + "\n";
    sw(getelemptr,src_reg);
}

void visit_getptr(const koopa_raw_value_t &getptr)//出现在函数有参数的情况下
{
    // TODO
    auto src = getptr->kind.data.get_ptr.src;
    auto index = getptr->kind.data.get_ptr.index;
    // cout << "src->ty->data.pointer.base->tag:" << src->ty->data.pointer.base->tag << endl;

    auto kind = src->ty->data.pointer.base; // src为指针，获取指针的基地址，以此来求 src指向内容的size
    int arraysize = 1;                      //初始化数组长度
    while (kind->tag == KOOPA_RTT_ARRAY)
    { //初始化数值时此部分跳过
        //如果当前 kind 指向的为数组
        int cursize = kind->data.array.len; //获取当前维度的长度
        arraysize *= cursize;
        kind = kind->data.array.base; //获取当前数组的base
    }
    // cout << "arraysize:" << arraysize << endl;

    string src_reg = "t0"; //本行的 src 地址所用的寄存器

    // 1. 计算 src 的地址 TODO 这一部分的 src 应该只是从 load 获得的
    if (src->kind.tag == KOOPA_RVT_LOAD)
    {
        li_lw(src,src_reg);
    }
    else
    {
        cout << "src->kind.tag = " << src->kind.tag << endl;
        std::cerr << "程序错误： getptr 的 src 类型不符合预期" << std::endl;
    }

    // 2. 获得 index 的大小
    string indexreg = "t1";
    li_lw(index,indexreg);

    string size_reg = "t2";
    arraysize *= 4;
    cout << "  li " + size_reg + ", " + to_string(arraysize) + "\n"; //当前指针的大小
    cout << "  mul " + indexreg + ", " + indexreg + ", " + size_reg + "\n";

    // 3. 计算 getptr 的结果
    cout << "  add " + src_reg + ", " + src_reg + ", " + indexreg + "\n";
    sw(getptr,src_reg);
}