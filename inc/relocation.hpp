#pragma once

struct Symbol;

struct Relocation{
    enum RELTYPE{DIRECT, DISP, PC_REL, NONE};
    int offset;
    RELTYPE type;
    int addend;
    Symbol* symbol;
    Relocation(int offset, RELTYPE type, int addend, Symbol* symbol){
        this->offset = offset;
        this->type = type;
        this->addend = addend;
        this->symbol = symbol;
    }

};