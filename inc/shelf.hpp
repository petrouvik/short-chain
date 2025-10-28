#pragma once
#include <cstdint>

struct ShelfHeader {
    uint8_t magic[5];
    uint32_t shoff;
    uint16_t shnum;
    uint16_t shstrndx;
};

struct ShelfSectionHeader {
    uint32_t nameOffset;
    uint32_t type;
    uint32_t offset;
    uint32_t size;
    uint32_t info;
    uint32_t address;
};
enum ShelfSectionType : uint32_t {
    SHELF_NULL      = 0,
    SHELF_PROGBITS  = 1, 
    SHELF_NOBITS    = 2,
    SHELF_SYMTAB    = 3,
    SHELF_STRTAB    = 4,
    SHELF_SYMSTRTAB = 5,
    SHELF_RELOC     = 6
};


struct ShelfSymbol {
    uint32_t nameOffset;
    uint32_t value;
    uint32_t size;
    uint8_t type;
    uint8_t bind;
    uint16_t shndx;
};
enum SymbolType : uint8_t { 
    ST_NOTYPE = 0, 
    ST_ABS = 1, 
    ST_SECTION = 2, 
    ST_FUNC = 3, 
    ST_OBJECT = 4
};
constexpr uint16_t SHELF_SHN_UNDEF = 0;
constexpr uint16_t SHELF_SHN_ABS   = -1;
enum SymbolBind : uint8_t { 
    STB_LOCAL = 0, 
    STB_GLOBAL = 1 
};


struct ShelfRelocation {
    uint32_t offset;
    uint32_t symIndex;
    uint8_t type;
    int32_t addend;
};
enum ShelfRelocType : uint8_t {
    R_SHELF_NONE = 0, 
    R_SHELF_DIRECT = 1, 
    R_SHELF_PC_REL = 2 
};