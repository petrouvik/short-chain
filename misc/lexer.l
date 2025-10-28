%option noyywrap

%{
#include "parser.hpp"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
extern int currentLine;
%}

/*Definitions*/
WS          [ \t\r]+
NL     \n
COMMENT     \#.*

LETTER      [A-Za-z_]
DIGIT       [0-9]
HEXDIGIT    [0-9A-Fa-f]
IDENT       {LETTER}({LETTER}|{DIGIT})*
DECIMAL     (0|[1-9][0-9]*)
HEX         0x{HEXDIGIT}+
CHARCONST   \'([^\'\\]|\\.)\'
STRINGCONST \"([^\\\"\n]|\\.)*\"

GPR_REG      \%r(1[0-5]|[0-9])

%x AFTER_END

%%
{WS}                ;
{COMMENT}           ;

{NL}           {currentLine++;return NEWLINE;}

".global"           return GLOBAL;
".extern"           return EXTERN;
".section"          return SECTION;
".word"             return WORD;
".skip"             return SKIP;
".end"              {BEGIN(AFTER_END); return END;}
".ascii"            return ASCII;
".equ"              return EQU;


"halt"              return HALT;
"int"               return INT;
"iret"              return IRET;
"call"              return CALL;
"ret"               return RET;
"jmp"               return JMP;
"beq"               return BEQ;
"bne"               return BNE;
"bgt"               return BGT;
"push"              return PUSH;
"pop"               return POP;
"xchg"              return XCHG;
"add"               return ADD;
"sub"               return SUB;
"mul"               return MUL;
"div"               return DIV;
"not"               return NOT;
"and"               return AND;
"or"                return OR;
"xor"               return XOR;
"shl"               return SHL;
"shr"               return SHR;
"ld"                return LD;
"st"                return ST;
"csrrd"             return CSRRD;
"csrwr"             return CSRWR;


":"                 return COLON;
","                 return COMMA;
"["                 return LBRACKET;
"]"                 return RBRACKET;
"+"                 return PLUS;
"-"                 return MINUS;
"("                 return LPAREN;
")"                 return RPAREN;

"$"                 return DOLLAR;


"%pc"                { yylval.ival = 15; return GPR; }
"%sp"                { yylval.ival = 14; return GPR; }
{GPR_REG}           { 
                      yylval.ival = atoi(yytext + 2);
                      return GPR; 
                    }


"%status"            { yylval.ival = 0; return CSR; }
"%handler"           { yylval.ival = 1; return CSR; }
"%cause"             { yylval.ival = 2; return CSR; }


{HEX}               { yylval.ival = (int)strtol(yytext, NULL, 0); return NUMBER; }
{DECIMAL}           { yylval.ival = atoi(yytext); return NUMBER; }
{CHARCONST}         { 
                      int c;
                      if (yytext[1] == '\\') {
                        char esc = yytext[2];
                        if (esc == 'n') c = '\n';
                        else if (esc == 't') c = '\t';
                        else if (esc == '\\') c = '\\';
                        else if (esc == '\'') c = '\'';
                        else c = esc;
                      } else {
                        c = yytext[1];
                      }
                      yylval.ival = c;
                      return CHAR;
                    }
{STRINGCONST}       {
                      int len = yyleng;
                      char temp[len - 1];

                      strncpy(temp, yytext + 1, len - 2);
                      temp[len - 2] = '\0';

                      yylval.sval = strdup(temp);
                      return STRING;
                    }

{IDENT}             { yylval.sval = strdup(yytext); return SYMBOL; }
\'      { fprintf(stderr, "Error: unmatched ' or empty character constant\n"); exit(1); }
\"      { fprintf(stderr, "Error: unmatched \"\n"); exit(1); }

. {
    fprintf(stderr, "Error: unrecognized character '%s' at line %d\n", yytext, currentLine);
    exit(1);
}

<AFTER_END>.        {return 0;}
<AFTER_END>{NL} {return 0;}
%%
