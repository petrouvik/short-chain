%{
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <iostream>
#include "assembler.hpp"
#include "expression.hpp"
#include "numberExpr.hpp"
#include "symbolExpr.hpp"
#include "unaryExpr.hpp"
#include "binaryExpr.hpp"

extern int yylex();
extern int yyparse();
extern FILE *yyin;
Assembler assembler;

int currentLine = 1;

void yyerror(const char *s) {
    fprintf(stderr, "Parse error: %s at line: %d\n", s, currentLine);
}
%}
%code requires {
    #include <vector>
    #include <string>
    #include "expression.hpp"
}
%union {
    int ival;
    char *sval;
    std::vector<std::string> *slist;
    Expression* expr; 
}

%token <ival> GPR CSR NUMBER CHAR
%token <sval> SYMBOL STRING
%type <slist> symbol_list symbol_literal_list
%type <slist> jmpOperand loadOperand storeOperand
%type <slist> symbol_or_literal
%type <ival> literal
%type <expr> expression term factor

/*Directives*/
%token GLOBAL EXTERN SECTION WORD SKIP END ASCII EQU

/*Commands*/
%token HALT INT IRET CALL RET JMP BEQ BNE BGT
%token PUSH POP XCHG ADD SUB MUL DIV NOT AND OR XOR SHL SHR
%token LD ST CSRRD CSRWR

/*Punctuation*/
%token DOLLAR COLON COMMA LBRACKET RBRACKET PLUS MINUS LPAREN RPAREN
%token NEWLINE
%left PLUS MINUS

%start file

/* ----- Grammar rules ----- */

%%

file:
    lines lastLine
    | lines
lastLine:
    label
    | label command
    | label directive
    | command
    | directive
    ;

lines:

    | lines line

line:
    label NEWLINE
    | label command NEWLINE
    | label directive NEWLINE
    | command NEWLINE
    | directive NEWLINE
    | NEWLINE
    ;

label: 
    SYMBOL COLON {
        assembler.defineLabel($1);
        free($1);
    }
    ;

/* ----- Directives ----- */

directive: 
    globalDir
    | externDir
    | sectionDir
    | wordDir
    | skipDir
    | endDir
    | asciiDir
    | equDir
    ;

globalDir:
    GLOBAL symbol_list { 
        assembler.processDirective(".global", *$2);
    }
    ;

externDir: 
    EXTERN symbol_list { 
        assembler.processDirective(".extern", *$2);
    }
    ;

sectionDir: 
    SECTION SYMBOL {
        std::vector<std::string> args;
        args.push_back($2);
        assembler.processDirective(".section", args);
        free($2);
    }
    ;

wordDir: 
    WORD symbol_literal_list { 
        assembler.processDirective(".word", *$2);
    }
    ;

skipDir: 
    SKIP literal {
        std::vector<std::string> args;
        args.push_back(std::to_string($2));
        assembler.processDirective(".skip", args);
    }
    ;

endDir: 
    END { 
        assembler.processDirective(".end", {});
    }
    ;
asciiDir:
    ASCII STRING {
        std::vector<std::string> args;
        args.push_back($2);
        assembler.processDirective(".ascii", args);
        free($2);
    };
equDir:
    EQU SYMBOL COMMA expression {
        assembler.processEqu($2, $4);
        free($2);
    }
    ;

/* ----- Expressions ----- */
expression:
    term { 
        $$ = $1; 
    }
    | expression PLUS term { 
        $$ = new BinaryExpr($1, '+', $3); 
    }
    | expression MINUS term { 
        $$ = new BinaryExpr($1, '-', $3); 
    }
    ;

term:
    factor { 
        $$ = $1; 
    }
    | MINUS factor{ 
        $$ = new UnaryExpr('-', $2); 
    }
    ;

factor:
    literal { 
        $$ = new NumberExpr($1); 
    }
    | SYMBOL { 
        $$ = new SymbolExpr($1); 
        assembler.symbolUsageEquHandler($1);
        free($1);
    }
    | LPAREN expression RPAREN {
         $$ = $2; 
    }
    ;

/* ----- Symbol lists ----- */

symbol_list:
    SYMBOL {
        $$ = new std::vector<std::string>();
        $$->push_back($1);
        free($1);
    }
    | symbol_list COMMA SYMBOL {
        $1->push_back($3);
        $$ = $1;
        free($3);
    }
    ;

symbol_literal_list:
    symbol_or_literal {
        $$ = new std::vector<std::string>();
        $$->push_back((*$1)[0]);
        $$->push_back((*$1)[1]);
        delete $1;
    }
    | symbol_literal_list COMMA symbol_or_literal {
        $$ = $1;
        $$->push_back((*$3)[0]);
        $$->push_back((*$3)[1]);
        delete $3;
    }
    ;

symbol_or_literal: 
    SYMBOL {
        $$ = new std::vector<std::string>();
        $$->push_back("sym");
        $$->push_back($1);
    }
    | literal {
        $$ = new std::vector<std::string>();
        $$->push_back("lit");
        $$->push_back(std::to_string($1));
    }
    ;

/* ----- Literals ----- */

literal:
    NUMBER {
        $$ = $1;
    }
    | CHAR {
        $$ = $1;
    }
    ;

/* ----- Commands ----- */

command: 
    HALT {
        assembler.processInstruction("halt", {});
    }
    | INT {
        assembler.processInstruction("int", {});
    }
    | IRET {
        assembler.processInstruction("iret", {});
    }
    | CALL jmpOperand {
        std::string mnemonic = "call" + (*$2)[0];
        $2->erase($2->begin());
        assembler.processInstruction(mnemonic, *$2);
        delete $2;
    }
    | RET {
        assembler.processInstruction("ret", {});
    }
    | JMP jmpOperand {
        std::string mnemonic = "jmp" + (*$2)[0];
        $2->erase($2->begin());
        assembler.processInstruction(mnemonic, *$2);
        delete $2;
    }
    | BEQ GPR COMMA GPR COMMA jmpOperand {
        std::string mnemonic = "beq" + (*$6)[0];
        std::vector<std::string> operands;
        operands.push_back(std::to_string($2));
        operands.push_back(std::to_string($4));
        operands.insert(operands.end(), $6->begin() + 1, $6->end());

        assembler.processInstruction(mnemonic, operands);
        delete $6;
    }
    | BNE GPR COMMA GPR COMMA jmpOperand {
        std::string mnemonic = "bne" + (*$6)[0];
        std::vector<std::string> operands;
        operands.push_back(std::to_string($2));
        operands.push_back(std::to_string($4));
        operands.insert(operands.end(), $6->begin() + 1, $6->end());

        assembler.processInstruction(mnemonic, operands);
        delete $6;
    }
    | BGT GPR COMMA GPR COMMA jmpOperand {
        std::string mnemonic = "bgt" + (*$6)[0];
        std::vector<std::string> operands;
        operands.push_back(std::to_string($2));
        operands.push_back(std::to_string($4));
        operands.insert(operands.end(), $6->begin() + 1, $6->end());

        assembler.processInstruction(mnemonic, operands);
        delete $6;
    }
    | PUSH GPR {
        assembler.processInstruction("push", {std::to_string($2)});
    }
    | POP GPR {
        assembler.processInstruction("pop", {std::to_string($2)});
    }
    | XCHG GPR COMMA GPR {
        assembler.processInstruction("xchg", {std::to_string($2), std::to_string($4)});
    }
    | ADD GPR COMMA GPR {
        assembler.processInstruction("add", {std::to_string($2), std::to_string($4)});
    }
    | SUB GPR COMMA GPR {
        assembler.processInstruction("sub", {std::to_string($2), std::to_string($4)});
    }
    | MUL GPR COMMA GPR {
        assembler.processInstruction("mul", {std::to_string($2), std::to_string($4)});
    }
    | DIV GPR COMMA GPR {
        assembler.processInstruction("div", {std::to_string($2), std::to_string($4)});
    }
    | NOT GPR {
        assembler.processInstruction("not", {std::to_string($2) });
    }
    | AND GPR COMMA GPR {
        assembler.processInstruction("and", {std::to_string($2), std::to_string($4) });
    }
    | OR GPR COMMA GPR {
        assembler.processInstruction("or", {std::to_string($2), std::to_string($4) });
    }
    | XOR GPR COMMA GPR {
        assembler.processInstruction("xor", {std::to_string($2), std::to_string($4) });
    }
    | SHL GPR COMMA GPR {
        assembler.processInstruction("shl", {std::to_string($2), std::to_string($4) });
    }
    | SHR GPR COMMA GPR {
        assembler.processInstruction("shr", {std::to_string($2), std::to_string($4) });
    }
    | LD loadOperand COMMA GPR {
        std::string mnemonic = (*$2)[0];
        $2->erase($2->begin());
        $2->push_back(std::to_string($4));

        assembler.processInstruction(mnemonic, *$2);
        delete $2;
    }
    | ST GPR COMMA storeOperand {
        std::string mnemonic = (*$4)[0];
        $4->erase($4->begin());
        $4->insert($4->begin(), std::to_string($2));

        assembler.processInstruction(mnemonic, *$4);
        delete $4;
    }
    | CSRRD CSR COMMA GPR {
        assembler.processInstruction("csrrd", {std::to_string($2), std::to_string($4)});
    }
    | CSRWR GPR COMMA CSR {
        assembler.processInstruction("csrwr", {std::to_string($2), std::to_string($4)});
    }
    ;

/* ----- Operands ----- */

jmpOperand:
    literal {
        $$ = new std::vector<std::string>();
        $$->push_back("lit");
        $$->push_back(std::to_string($1));
    }
    | SYMBOL {
        $$ = new std::vector<std::string>();
        $$->push_back("sym");
        $$->push_back($1);
        free($1);
    }
    ;

loadOperand:
    DOLLAR literal {
        $$ = new std::vector<std::string>();
        $$->push_back("ldimm");
        $$->push_back(std::to_string($2));
    }
    | DOLLAR SYMBOL { 
        $$ = new std::vector<std::string>();
        $$->push_back("ldsym");
        $$->push_back($2);
        free($2);
    }
    | literal { 
        $$ = new std::vector<std::string>();
        $$->push_back("ldlit");
        $$->push_back(std::to_string($1));
    }
    | SYMBOL { 
        $$ = new std::vector<std::string>();
        $$->push_back("ldsymabs");
        $$->push_back($1);
        free($1);
    }
    | GPR { 
        $$ = new std::vector<std::string>();
        $$->push_back("ldreg");
        $$->push_back(std::to_string($1));
    }
    | LBRACKET GPR RBRACKET { 
        $$ = new std::vector<std::string>();
        $$->push_back("ldind");
        $$->push_back(std::to_string($2));
    }
    | LBRACKET GPR PLUS literal RBRACKET { 
        $$ = new std::vector<std::string>();
        $$->push_back("ldindlit");
        $$->push_back(std::to_string($2));
        $$->push_back(std::to_string($4)); 
    }
    | LBRACKET GPR PLUS SYMBOL RBRACKET { 
        $$ = new std::vector<std::string>();
        $$->push_back("ldindsym");
        $$->push_back(std::to_string($2));
        $$->push_back($4);
        free($4);
    }
    ;

storeOperand: 
    literal { 
        $$ = new std::vector<std::string>();
        $$->push_back("stlit");
        $$->push_back(std::to_string($1));
    }
    | SYMBOL { 
        $$ = new std::vector<std::string>();
        $$->push_back("stsymabs");
        $$->push_back($1);
        free($1);
    }
    | LBRACKET GPR RBRACKET { 
        $$ = new std::vector<std::string>();
        $$->push_back("stind");
        $$->push_back(std::to_string($2));
    }
    | LBRACKET GPR PLUS literal RBRACKET { 
        $$ = new std::vector<std::string>();
        $$->push_back("stindlit");
        $$->push_back(std::to_string($2));
        $$->push_back(std::to_string($4)); 
    }
    | LBRACKET GPR PLUS SYMBOL RBRACKET { 
        $$ = new std::vector<std::string>();
        $$->push_back("stindsym");
        $$->push_back(std::to_string($2));
        $$->push_back($4);
        free($4);
    }
    ;

%%

void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " <input_file> -o <output_file>\n";
    std::cout << "\n";
    std::cout << "Options:\n";
    std::cout << "  -h             Show this help message and exit.\n";
    std::cout << "  -o <file>      Specify the output object file.\n";
    std::cout << "\n";
    std::cout << "Example:\n";
    std::cout << "  " << progName << " program.s -o program.o\n";
    std::cout << "\n";
}

int main(int argc, char **argv) {
    std::string inputFile;
    std::string outputFile;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h") {
            printUsage(argv[0]);
            return 0;

        } else if (arg == "-o") {
            if (i + 1 >= argc) {
                std::cerr << "Error: -o requires a filename\n";
                return 1;
            }
            outputFile = argv[++i];
        } else {
            inputFile = arg;
        }
    }

    if (inputFile.empty()) {
        std::cerr << "Error: No input file specified\n";
        printUsage(argv[0]);
        return 1;
    }
    if (outputFile.empty()) {
        std::cerr << "Error: -o option is required\n";
        printUsage(argv[0]);
        return 1;
    }

    
    yyin = fopen(inputFile.c_str(), "r");
    if (!yyin) {
        std::cerr << "Error: Cannot open input file " << inputFile << "\n";
        return 1;
    }

    
    if (yyparse()) {
        return 1;
    }

    
    assembler.writeOutput(outputFile);

    return 0;
}
