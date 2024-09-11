%{
  #include "../inc/parser.hpp"
  #include <string>
  #include <stdio.h>
  #include <cstdint>
  using namespace std;
%}

%option noyywrap

%%
% {return PERCENT;}
".global" {return GLOBAL;}
".extern" {return EXTERN;}
".section" {return SECTION;}
".word" {return WORD;}
".skip" {return SKIP;}
".ascii" {return ASCII;}
".end" {return END;}
".equ" {return EQU;}

halt {return HALT;}
iret {return IRET;}
int {return INT;}
call {return CALL;}
ret {return RET;}
jmp {return JMP;}
bne {return BNE;}
bgt {return BGT;}
beq {return BEQ;}
push {return PUSH;}
pop {return POP;}
xchg {return XCHG;}
add {return ADD;}
sub {return SUB;}
mul {return MUL;}
div {return DIV;}
not {return NOT;}
and {return AND;}
or {return OR;}
xor {return XOR;}
shl {return SHL;}
shr {return SHR;}
ld {return LD;}
st {return ST;}
csrrd {return CSRRD;}
csrwr {return CSRWR;}
%r([0-9]|1[0-5]) {yylval.num=atoi(yytext+2);return REG;}
%sp {yylval.num=14;return REG;}
%pc {yylval.num=15;return REG;}
%status {yylval.num=0;return CSR;}
%handler {yylval.num=1;return CSR;}
%cause {yylval.num=2;return CSR;}

0[xX][0-9a-fA-F]+ {yylval.num=stol(yytext+2, nullptr, 16);return LITERAL_HEX;}
[0-9]|([1-9][0-9]*) {yylval.num=stol(yytext);return LITERAL_DEC;}
[a-zA-Z_][a-zA-Z_0-9]* { 
  yylval.txt = strdup(yytext);
  return SYMBOL;
}

"[" {return LEFT_BRACKET;}
"]" {return RIGHT_BRACKET;}
"$" {return DOLLAR;}
"," {return COMMA;}
\n {return EOL;}
"+" {return PLUS;}
"-" {return MINUS;}
":" {return COLON;}
"." {return DOT;}
#.*\n {return KOMENTAR;}
. {}
%%
