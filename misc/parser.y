%{
  #include <stdio.h>
  #include <string>
  #include <cstring>
  #include <stdlib.h>
  
  #include <iostream>
  #include <fstream> 
  #include <vector>
  #include <cstdint>
  #include "../inc/TES.hpp"
  #include "../inc/Asembler.hpp"
  using namespace std;
  extern FILE* yyin;

  void yyerror(const char* s){cout<<s<<endl;}
  extern int yylex();
  extern int yyparse();

  vector<string> sim_list;
  vector<WordNode> list;

  Asembler* a=new Asembler();
  Operand op=Operand(IMMED_LIT,0,0,"");
  static int cnt=0;
%}

%union{
  long num;
  char* txt;
}

%token EOL PERCENT DOLLAR LEFT_BRACKET RIGHT_BRACKET COMMA PLUS MINUS STAR COLON KOMENTAR DOT
%token GLOBAL EXTERN SECTION WORD SKIP ASCII END EQU
%token HALT INT IRET RET
%token JMP CALL 
%token<num> BNE BGT BEQ
%token PUSH POP NOT
%token<num> XCHG ADD SUB MUL DIV AND OR XOR SHL SHR
%token LD ST
%token<num> CSRRD CSRWR
%token<num> CSR
%token<num> REG
%token<num> LITERAL_HEX
%token<num> LITERAL_DEC
%token<txt> SYMBOL
%type<num> literal
%type<txt> operand
%type<num> branch_instruction
%type<num> two_reg_instruction

%%
input:
  line 
  | input line
  ;

line: 
  exp EOL
  | EOL
  | KOMENTAR
  | labela line
  | exp KOMENTAR
  | labela KOMENTAR
  ;

exp: 
  instr
  | dir
  ;

labela:
  SYMBOL COLON {a->add_symbol($1);}

instr:
  HALT {a->halt_int_instruction(0);}
  | RET {a->pop_instruction(15);}
  | INT {a->halt_int_instruction(0x10);}
  | IRET {a->iret_instruction();}

  | PUSH REG {a->push_instruction($2);}
  | POP REG {a->pop_instruction($2);}

  | NOT REG {a->ari_log_sh_instruction(0x60, 0, $2);}

  | XCHG REG COMMA REG{a->xchg_instruction($2, $4);}
  | two_reg_instruction REG COMMA REG {a->ari_log_sh_instruction($1, $2, $4);}

  | CSRRD CSR COMMA REG {a->csr_instruction(0x90, $4, $2);}
  | CSRWR REG COMMA CSR {a->csr_instruction(0x94, $4, $2);}

  | branch_instruction REG COMMA REG COMMA literal {a->jump_instructions($1, $2, $4, $6, "");}
  | branch_instruction REG COMMA REG COMMA SYMBOL {a->jump_instructions($1, $2, $4, 0, $6);}

  | JMP SYMBOL{ a->jump_instructions(0, 0, 0, 0, $2);}
  | JMP literal{ a->jump_instructions(0, 0, 0, $2, "");}

  | CALL SYMBOL{a->call_instruction(0, $2);}
  | CALL literal{a->call_instruction($2, "");}

  | LD operand COMMA REG {a->load_instruction(op, $4);}
  | ST REG COMMA operand {a->store_instruction(op, $2);}
  ;

dir:
  GLOBAL sym_list {
      a->dir_global(sim_list);
      sim_list.clear();
    }

  | EXTERN sym_list {
      a->dir_extern(sim_list);
      sim_list.clear();
    }

  | SECTION SYMBOL{
      a->dir_section($2);
    }

  | WORD list {
      a->dir_word(list);
      list.clear();
    }
  | SKIP literal {a->dir_skip($2);}
  | END {a->dir_end();YYACCEPT;}

two_reg_instruction:
  ADD {$$=0x50;}
  | SUB {$$=0x51;}
  | MUL {$$=0x52;}
  | DIV {$$=0x53;}

  | AND {$$=0x61;}
  | OR {$$=0x62;}
  | XOR {$$=0x63;}

  | SHL {$$=0x70;}
  | SHR {$$=0x71;}
  ;

branch_instruction:
  BEQ {$$=0x01;}
  | BNE {$$=0x02;}
  | BGT {$$=0x03;}
  ;

sym_list:
  SYMBOL {sim_list.push_back($1);}
  | sym_list COMMA SYMBOL {sim_list.push_back($3);}
  ;

list: 
  SYMBOL {list.push_back({false, 0, $1});}
  | literal {list.push_back({true, $1, ""});}
  | list COMMA SYMBOL {list.push_back({false, 0, $3});}
  | list COMMA literal {list.push_back({true, $3, ""});}
  ;

operand:
  DOLLAR literal {op=Operand(IMMED_LIT, 0, $2, "");}
  | DOLLAR SYMBOL {op=Operand(IMMED_SYM, 0, 0, $2);}
  | literal {op=Operand(DIR_LIT, 0, $1, "");}
  | SYMBOL {op=Operand(DIR_SYM, 0, 0, $1);}
  | REG {op=Operand(IMMED_REG, $1, 0, "");}
  | LEFT_BRACKET REG RIGHT_BRACKET {op=Operand(DIR_REG, $2, 0, "");}
  | LEFT_BRACKET REG PLUS literal RIGHT_BRACKET
    {op=Operand(DIR_REG_LIT, $2, $4, "");}
  ;

literal:
  LITERAL_DEC {$$=$1;}
  | LITERAL_HEX {$$=$1;}
  ;
%% 

int main(int argc, char** argv){
  if(argc<2){ cerr<<"Nedovoljno argumenata"<<endl; return -1;}
  if(strcmp(argv[0],"./asembler")!=0){ cerr<<"Pogresan kod "<<argv[0]<<endl; return -1 ;}
  FILE *file;
  if(strcmp(argv[1],"-o")==0){
    if(argc<4){cerr<<"Nedovoljno argumenata"<<endl; return -1;}
    a->fileOut=argv[2];
    file= fopen(argv[3], "r");
  }
  else
    file= fopen(argv[1], "r");
  if (!file) {
    cerr << "Can't find file " << endl; return -1; 
  }
  yyin = file;
  yyparse();
  fclose(file);
}