%code requires {
  #include <memory>
  #include <string>
  #include "ast.h"
}

%{

#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include "ast.h"
// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

// 定义 parser 函数和错误处理函数的附加参数
// 我们需要返回一个字符串作为 AST, 所以我们把附加参数定义成字符串的智能指针
// 解析完成后, 我们要手动修改这个参数, 把它设置成解析得到的字符串
%parse-param { std::unique_ptr<BaseAST> &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
%union {
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
  vector<BaseAST*> *vec_val;
}

//终结符
// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT VOID RETURN CONST LE GE EQ NE LAND LOR LT GT IF ELSE WHILE BREAK CONTINUE
%token <str_val> IDENT
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> FuncDef Block Decl Stmt PrimaryExp UnaryExp Exp
%type <ast_val> AddExp MulExp LOrExp LAndExp EqExp RelExp
%type <ast_val> VarDecl VarDef InitVal 
%type <ast_val> ConstDecl ConstDef ConstInitVal ConstExp
%type <ast_val> BlockItem LVal
%type <ast_val> MatchStmt UnMatchStmt
%type <ast_val> FuncFParam
%type <int_val> Number
%type <str_val> UnaryOp MulOp RelOp EqOp BType
%type <vec_val> VarDef_ ConstDef_ BlockItem_ CompUnits FuncFParams FuncRParams
%type <vec_val> ConstExp_ InitVal_ ConstInitVal_ Exp__

%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值
CompUnit
    : CompUnits {
        auto comp_unit = make_unique<CompUnitAST>();
        comp_unit->compunits.assign(($1)->begin(),($1)->end());
        ast = move(comp_unit);
    }
    ;

CompUnits
    : Decl {
        auto vec = new vector<BaseAST*>;
        vec->push_back($1);
        $$ = vec;

    }
    | CompUnits Decl {
        auto vec = new vector<BaseAST*>;
        if($1) vec->assign(($1)->begin(),($1)->end());
        vec->push_back($2);
        $$ = vec;
    }
    | FuncDef{ 
        auto vec = new vector<BaseAST*>;
        vec->push_back($1);
        $$ = vec;
    }
    | CompUnits FuncDef{
        auto vec = new vector<BaseAST*>;
        if($1) vec->assign(($1)->begin(),($1)->end());
        vec->push_back($2);
        $$ = vec;
    };

Decl 
    : ConstDecl {
        auto ast = new Decl1AST();
        ast->constdecl = unique_ptr<BaseAST>($1);
        $$ = ast;

    }
    | VarDecl {
        
        auto ast = new Decl2AST();
        ast->vardecl = unique_ptr<BaseAST>($1);
        $$ = ast;
    };
    

ConstDecl
    : CONST BType ConstDef_ ';' {
        auto ast = new ConstDeclAST();
        ast->btype = *unique_ptr<string>($2);
        if($3) ast->constdef_.assign(($3)->begin(),($3)->end());
        $$ = ast;

    };

VarDecl
    : BType VarDef_ ';' {
        auto ast = new VarDeclAST();
        ast->btype = *unique_ptr<string>($1);
        if($2)ast->vardef_.assign(($2)->begin(),($2)->end());
        $$ = ast;

    };

VarDef_
    : VarDef_ ',' VarDef {
        auto vec = new vector<BaseAST*>;
        if($1) vec->assign(($1)->begin(),($1)->end());
        vec->push_back($3);
        $$ = vec;
    }
    | VarDef {
        auto vec = new vector<BaseAST*>;
        vec->push_back($1);
        $$ = vec;
    };

VarDef
    : IDENT ConstExp_ {//constexp_存在时是数组
        auto ast = new VarDef1AST();
        ast->ident = *unique_ptr<string>($1);
        if($2) ast->constexp_.assign(($2)->begin(), ($2)->end());
        $$ = ast;
    }
    | IDENT ConstExp_ '=' InitVal {

        auto ast = new VarDef2AST();
        ast->ident = *unique_ptr<string>($1);
        ast->initval = unique_ptr<BaseAST>($4);
        if($2) ast->constexp_.assign(($2)->begin(), ($2)->end());
        $$ = ast;
    };

ConstExp_
  : ConstExp_ '[' ConstExp ']' {//多维数组
    auto vec = new vector<BaseAST*>;
    if($1) vec->assign(($1)->begin(), ($1)->end());
    vec->push_back($3);
    $$ = vec;
  }
  | {
    $$ = NULL;
  };

InitVal
    : Exp {
        auto ast = new InitVal1AST();
        ast->exp = unique_ptr<BaseAST>($1);
        $$ = ast;
    }
    | '{' '}' {
        auto ast = new InitVal2AST();
        $$ = ast;
    }
    | '{' InitVal_ '}' {
        auto ast = new InitVal3AST();
        if($2) ast->initval_.assign(($2)->begin(),($2)->end());
        $$ = ast;
    };

InitVal_
    : InitVal_ ',' InitVal {
        auto vec = new vector<BaseAST*>;
        if($1) vec->assign(($1)->begin(), ($1)->end());
        vec->push_back($3);
        $$ = vec;
    }
    | InitVal {
        auto vec = new vector<BaseAST*>;
        vec->push_back($1);
        $$ = vec;
    };

BType
    : INT {
        $$ = new string("int");
    } 
    | VOID {
        $$ = new string("void");
    };

ConstDef_
    : ConstDef_ ',' ConstDef {
        auto vec = new vector<BaseAST*>;
        if($1) vec->assign(($1)->begin(),($1)->end());
        vec->push_back($3);
        $$ = vec;

    }
    | ConstDef {
        auto vec = new vector<BaseAST*>;
        vec->push_back($1);
        $$ = vec;

    };


ConstDef
    : IDENT ConstExp_ '=' ConstInitVal {
        auto ast = new ConstDefAST();
        ast->ident = *unique_ptr<string>($1);
        if($2) ast->constexp_.assign(($2)->begin(), ($2)->end());
        ast->constinitval = unique_ptr<BaseAST>($4);
        $$ = ast;

    };

ConstInitVal
    : ConstExp {
        auto ast = new ConstInitVal1AST();
        ast->constexp = unique_ptr<BaseAST>($1);
        $$ = ast;
    }
    | '{' '}' {
        auto ast = new ConstInitVal2AST();
        $$ = ast;

    }
    | '{' ConstInitVal_ '}' {
        auto ast = new ConstInitVal3AST();
        if($2) ast->constinitval_.assign(($2)->begin(),($2)->end());
        $$ = ast;
    };

ConstInitVal_
    : ConstInitVal_ ',' ConstInitVal {
        auto vec = new vector<BaseAST*>;
        if($1) vec->assign(($1)->begin(),($1)->end());
        vec->push_back($3);
        $$ = vec;
    }
    | ConstInitVal {
        auto vec = new vector<BaseAST*>;
        vec->push_back($1);
        $$ = vec;
    };

ConstExp
    : Exp {
        auto ast = new ConstExpAST();
        ast->exp = unique_ptr<BaseAST>($1);
        $$ = ast;

    };

// FuncDef ::= FuncType IDENT '(' ')' Block;
// 我们这里可以直接写 '(' 和 ')', 因为之前在 lexer 里已经处理了单个字符的情况
// 解析完成后, 把这些符号的结果收集起来, 然后拼成一个新的字符串, 作为结果返回
// $$ 表示非终结符的返回值, 我们可以通过给这个符号赋值的方法来返回结果
// 你可能会问, FuncType, IDENT 之类的结果已经是字符串指针了
// 为什么还要用 unique_ptr 接住它们, 然后再解引用, 把它们拼成另一个字符串指针呢
// 因为所有的字符串指针都是我们 new 出来的, new 出来的内存一定要 delete
// 否则会发生内存泄漏, 而 unique_ptr 这种智能指针可以自动帮我们 delete
// 虽然此处你看不出用 unique_ptr 和手动 delete 的区别, 但当我们定义了 AST 之后
// 这种写法会省下很多内存管理的负担
FuncDef
    : BType IDENT '(' ')' Block {
        auto ast = new FuncDef1AST();
        ast->func_type = *unique_ptr<string>($1);
        ast->ident = *unique_ptr<string>($2);
        ast->block = unique_ptr<BaseAST>($5);
        $$ = ast;
    };
    | BType IDENT '(' FuncFParams ')' Block {
        auto ast = new FuncDef2AST();
        ast->func_type = *unique_ptr<string>($1);
        ast->ident = *unique_ptr<string>($2);
        if($4)ast->funcfparams.assign(($4)->begin(),($4)->end());
        ast->block = unique_ptr<BaseAST>($6);
        $$ = ast;

    };

// 同上, 不再解释
/*FuncType
    : INT {
        auto ast = new FuncTypeAST();
        ast->functype = "int";
        $$ = ast;
    };
    | VOID {
        auto ast = new FuncTypeAST();
        ast->functype = "void";
        $$ = ast;
    };*/

FuncFParams
  : FuncFParam {
    auto vec = new vector<BaseAST*>;
    vec->push_back($1);
    $$ = vec;
  }
  | FuncFParams ',' FuncFParam {
    auto vec = new vector<BaseAST*>;
    if($1) vec->assign(($1)->begin(),($1)->end());
    vec->push_back($3);
    $$ = vec;
  };
  

FuncFParam//单个函数参数
  : BType IDENT {
    auto ast = new FuncFParam1AST();
    ast->type = *unique_ptr<string>($1);
    ast->ident = *unique_ptr<string>($2);
    $$ = ast;
  }
  | BType IDENT '[' ']' ConstExp_ {
    auto ast = new FuncFParam2AST();
    ast->type = *unique_ptr<string>($1);
    ast->ident = *unique_ptr<string>($2);
    if($5) ast->constexp_.assign(($5)->begin(),($5)->end());
    $$ = ast;
  };
  

Block
    : '{' BlockItem_ '}'{
        auto ast = new BlockAST();
        if($2) ast->blockitem_.assign(($2)->begin(),($2)->end());
        $$ = ast;
    };

BlockItem_
    : BlockItem_ BlockItem {
        auto vec = new vector<BaseAST*>;
        if($1) vec->assign(($1)->begin(),($1)->end());
        vec->push_back($2);
        $$ = vec;

    }
    | {
        $$ = NULL;
    };

BlockItem
    : Stmt {
        auto ast = new BlockItem1AST();
        ast->stmt = unique_ptr<BaseAST>($1);
        $$ = ast;
    }
    | Decl {
        
        auto ast = new BlockItem2AST();
        ast->decl = unique_ptr<BaseAST>($1);
        $$ = ast;

    };

Stmt
    : MatchStmt {
        auto ast = new Stmt1AST();
        ast->match = unique_ptr<BaseAST>($1);
        $$ = ast;

    }
    | UnMatchStmt {
        auto ast = new Stmt2AST();
        ast->unmatch = unique_ptr<BaseAST>($1);
        $$ = ast;
    };

UnMatchStmt
    : IF '(' Exp ')' Stmt {
        auto ast = new UnMatchStmt1AST();
        ast->exp = unique_ptr<BaseAST>($3);
        ast->stmt = unique_ptr<BaseAST>($5);
        $$ = ast;

    }
    | IF '(' Exp ')' MatchStmt ELSE UnMatchStmt {
        auto ast = new UnMatchStmt2AST();
        ast->exp = unique_ptr<BaseAST>($3);
        ast->match = unique_ptr<BaseAST>($5);
        ast->unmatch = unique_ptr<BaseAST>($7);
        $$ = ast;
    }
    | WHILE '(' Exp ')' UnMatchStmt {
        auto ast = new UnMatchStmt3AST();
        ast->exp = unique_ptr<BaseAST>($3);
        ast->unmatch = unique_ptr<BaseAST>($5);
        $$ = ast;
    };

MatchStmt
    : RETURN Exp ';'{
        auto ast = new MatchStmt1AST();
        ast->exp = unique_ptr<BaseAST>($2);
        $$ = ast;
    }
    | LVal '=' Exp ';' {
        auto ast = new MatchStmt2AST();
        ast->lval = unique_ptr<BaseAST>($1);
        ast->exp = unique_ptr<BaseAST>($3);
        $$ = ast;
    }
    | RETURN ';' {
        auto ast = new MatchStmt3AST();
        $$ = ast;
    }
    | Block {
        auto ast = new MatchStmt4AST();
        ast->block = unique_ptr<BaseAST>($1);
        $$ = ast;

    }
    | ';' {//无意义
        auto ast = new MatchStmt5AST();
        $$ = ast;
    }
    | Exp ';' {//无意义
        auto ast = new MatchStmt6AST();
        ast->exp = unique_ptr<BaseAST>($1);
        $$ = ast;
    }
    | IF '(' Exp ')' MatchStmt ELSE MatchStmt {
        auto ast = new MatchStmt7AST();
        ast->exp = unique_ptr<BaseAST>($3);
        ast->match1 = unique_ptr<BaseAST>($5);
        ast->match2 = unique_ptr<BaseAST>($7);
        $$ = ast;
    }
    | WHILE '(' Exp ')' MatchStmt {
        auto ast = new MatchStmt8AST();
        ast->exp = unique_ptr<BaseAST>($3);
        ast->match = unique_ptr<BaseAST>($5);
        $$ = ast;
    }
    | BREAK ';' {
        auto ast = new MatchStmt9AST();
        $$ = ast;

    }
    | CONTINUE ';' {
        auto ast = new MatchStmt10AST();
        $$ = ast;

    };

LVal
    : IDENT Exp__ {
        auto ast = new LValAST();
        ast->ident = *unique_ptr<string>($1);
        if(($2)) ast->exp__.assign(($2)->begin(),($2)->end());
        $$ = ast;
    };

Exp__
    : Exp__ '[' Exp ']' {
        auto vec = new vector<BaseAST*>;
        if(($1)) vec->assign(($1)->begin(),($1)->end());
        vec->push_back($3);
        $$ = vec;
    }
    | {
        $$ = NULL;
    };

Exp
    : LOrExp {
        //cerr<<"exp\n";
        auto ast = new ExpAST();
        ast->lorexp = unique_ptr<BaseAST>($1);
        $$ = ast;
    };

LOrExp
    : LAndExp {
        //cerr<<"lorexp\n";
        auto ast = new LOrExp1AST();
        ast->landexp = unique_ptr<BaseAST>($1);
        $$ = ast;
    }
    | LOrExp LOR LAndExp {
        auto ast = new LOrExp2AST();
        ast->lorexp = unique_ptr<BaseAST>($1);
        ast->landexp = unique_ptr<BaseAST>($3);
        $$ = ast;
    }
    ;

LAndExp
    : EqExp {
        //cerr<<"landexp\n";
        auto ast = new LAndExp1AST();
        ast->eqexp = unique_ptr<BaseAST>($1);
        $$ = ast;
    }
    | LAndExp LAND EqExp {
        auto ast = new LAndExp2AST();
        ast->landexp = unique_ptr<BaseAST>($1);
        ast->eqexp = unique_ptr<BaseAST>($3);
        $$ = ast;
    }
    ;

EqExp
    : RelExp {
        //cerr<<"eqexp\n";
        auto ast = new EqExp1AST();
        ast->relexp = unique_ptr<BaseAST>($1);
        $$ = ast;
    }
    | EqExp EqOp RelExp {
        auto ast = new EqExp2AST();
        ast->eqexp = unique_ptr<BaseAST>($1);
        ast->eqop = *unique_ptr<string>($2);
        ast->relexp = unique_ptr<BaseAST>($3);
        $$ = ast;
    }
    ;

RelExp
    : AddExp {
        //cerr<<"relexp\n";
        auto ast = new RelExp1AST();
        ast->addexp = unique_ptr<BaseAST>($1);
        $$ = ast;
    }
    | RelExp RelOp AddExp {
        auto ast = new RelExp2AST();
        ast->relexp = unique_ptr<BaseAST>($1);
        ast->relop = *unique_ptr<string>($2);
        ast->addexp = unique_ptr<BaseAST>($3);
        $$ = ast;
    }
    ;
  
AddExp
    : MulExp {
        //cerr<<"addexp\n";
        auto ast = new AddExp1AST();
        ast->mulexp = unique_ptr<BaseAST>($1);
        $$ = ast;
    }
    | AddExp UnaryOp MulExp {
        auto ast = new AddExp2AST();
        ast->addexp = unique_ptr<BaseAST>($1);
        ast->unaryop = *unique_ptr<string>($2);
        ast->mulexp = unique_ptr<BaseAST>($3);
        $$ = ast;
        
    };

MulExp
    : UnaryExp {
        //cerr<<"mulexp\n";
        auto ast = new MulExp1AST();
        ast->unaryexp = unique_ptr<BaseAST>($1);
        $$ = ast;

    }
    | MulExp MulOp UnaryExp {
        //cerr<<"mul op unary\n";
        auto ast = new MulExp2AST();
        ast->mulexp = unique_ptr<BaseAST>($1);
        ast->mulop = *unique_ptr<string>($2);
        ast->unaryexp = unique_ptr<BaseAST>($3);
        $$ = ast;

    };

MulOp
    : '*' {
        $$ = new string("*");

    }
    | '/' {
        $$ = new string("/");

    }
    | '%' {
        $$ = new string("%");

    };

UnaryExp
    : PrimaryExp{
        //cerr<<"unaryexp\n";
        auto ast = new UnaryExp1AST();
        ast->primaryexp = unique_ptr<BaseAST>($1);
        $$ = ast;
    }
    | UnaryOp UnaryExp {
        auto ast = new UnaryExp2AST();
        ast->unaryop = *unique_ptr<string>($1);
        ast->unaryexp = unique_ptr<BaseAST>($2);
        $$ = ast;
    }
    | IDENT '(' ')' {//无参数
        auto ast = new UnaryExp3AST();
        ast->ident = *unique_ptr<string>($1);
        $$ = ast;
    }
    | IDENT '(' FuncRParams ')' {//有参数
        auto ast = new UnaryExp4AST();
        ast->ident = *unique_ptr<string>($1);
        if($3)ast->funcrparams.assign(($3)->begin(),($3)->end());
        $$ = ast;
    };

FuncRParams
  : Exp {
    auto vec = new vector<BaseAST*>;
    vec->push_back($1);
    $$ = vec;
  }
  | FuncRParams ',' Exp {
    auto vec = new vector<BaseAST*>;
    if($1) vec->assign(($1)->begin(),($1)->end());
    vec->push_back($3);
    $$ = vec;
  };

PrimaryExp
    : '(' Exp ')' {
        auto ast = new PrimaryExp1AST();
        ast->exp = unique_ptr<BaseAST>($2);
        $$ = ast;
    } 
    | Number {
        //cerr<<"primaryexp\n";
        auto ast = new PrimaryExp2AST();
        ast->number = $1;
        $$ = ast;
       
    }
    | LVal {
        auto ast = new PrimaryExp3AST();
        ast->lval = unique_ptr<BaseAST>($1);
        $$ = ast;

    };

UnaryOp
    : '+' {
        $$ = new string("+");
    }
    | '-' {
        $$ = new string("-");
    }
    | '!' {
        $$ = new string("!");
    }

RelOp
  : LT {
    $$ = new string("<");
  }
  | GT {
    $$ = new string(">");
  }
  | LE {
    $$ = new string("<=");
  }
  | GE {
    $$ = new string(">=");
  }
  ;

EqOp
  : EQ {
    $$ = new string("==");
  }
  | NE {
    $$ = new string("!=");
  }
  ;

Number
    : INT_CONST {
        //cerr<<"int_const\n";
        $$ = ($1);
    };

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cout << "error: " << s << endl;
}

