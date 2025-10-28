#pragma once
#include <string>
#include <vector>
#include "forwardRef.hpp"
struct Section;
struct Symbol{
    enum SYMTYPE {NOTYP, SCTN};
    enum SYMBIND {LOCAL, GLOBAL};

    int index;
    std::string name;
    int value;
    int size;
    SYMTYPE type;
    SYMBIND binding;
    Section* section;

    bool external;
    bool defined;
    std::vector<ForwardRef> forwardRefs;

    Symbol(int idx, const std::string& nm, Section* sec)
        : index(idx), name(nm), value(0), size(0), type(NOTYP), binding(LOCAL), section(sec), external(false), defined(false) {}
};