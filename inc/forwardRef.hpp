#pragma once
#include "relocation.hpp"
struct Section;
struct ForwardRef{
    int offset;
    Relocation::RELTYPE type;
    int addend;
    Section* section;

    ForwardRef(int off, Relocation::RELTYPE t, int ad, Section* sec)
        : offset(off), type(t), addend(ad), section(sec) {}
};