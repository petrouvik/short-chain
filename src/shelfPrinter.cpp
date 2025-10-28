#include "shelfPrinter.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include "shelf.hpp"
ShelfPrinter::ShelfPrinter(const std::string& filename)
    : reader(filename) {}

void ShelfPrinter::print(const std::string& outputFile) {
    std::ofstream out(outputFile);
    if (!out)
        throw std::runtime_error("Failed to open output file: " + outputFile);

    printSectionHeaderTable(out);
    out << "\n";
    printSectionContents(out);
    out << "\n";
    printSymbols(out);
    out << "\n";
    printRelocations(out);
}


std::string to_hex_msb(uint32_t value) {
    std::ostringstream oss;
    for (int i = 3; i >= 0; --i) {
        oss << std::hex << std::setw(2) << std::setfill('0')
            << ((value >> (i*8)) & 0xFF);
    }
    return oss.str();
}
std::string section_type_to_string(uint32_t value){
    switch(value){
        case SHELF_NULL: return "NULL";
        case SHELF_PROGBITS: return "PROGBITS";
        case SHELF_NOBITS: return "NOBITS";
        case SHELF_SYMTAB: return "SYMTAB";
        case SHELF_STRTAB: return "STRTAB";
        case SHELF_SYMSTRTAB: return "SYMSTRTAB";
        case SHELF_RELOC: return "RELOC";
    }
    return "";
}
void ShelfPrinter::printSectionHeaderTable(std::ostream& os) {
    os << "Section Headers:\n";
    os << std::left
       << std::setw(4) << "Nr" 
       << std::setw(16) << "Name"
       << std::setw(10) << "Type" << "  "
       << std::setw(8) << "Size" << "  "
       << std::setw(8) << "Info" << "  "
       << "\n";

    const auto& shs = reader.getSectionHeaders();
    for (size_t i = 0; i < shs.size(); i++) {
        os << std::left << std::setw(4) << i;
        os << std::left << std::setw(16) << shs[i].name;
        os << std::left << std::setw(10) << section_type_to_string(shs[i].type) << "  ";
        os << to_hex_msb(shs[i].size) << "  ";
        os << to_hex_msb(shs[i].info) << "\n"; 
    }
}

void ShelfPrinter::printSectionContents(std::ostream& os) {
    const auto& shs = reader.getSectionHeaders();
    for (size_t i = 0; i < shs.size(); i++) {
        if (shs[i].type == SHELF_PROGBITS) {
            os << "#" << shs[i].name << "\n";
            const auto& data = reader.getSectionContents(i);
            printBytes(os, data);
            os << "\n";
        }
    }
}

void ShelfPrinter::printBytes(std::ostream& os, const std::vector<uint8_t>& data) {
    for (size_t i = 0; i < data.size(); i++) {
        if (i > 0 && i % 8 == 0) os << "\n";
        os << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(data[i]) << " ";
    }
    os << std::dec << std::setfill(' ');
}


std::string symbol_type_to_string(uint32_t value){
    switch(value){
        case ST_NOTYPE: return "NOTYPE";
        case ST_ABS: return "ABS";
        case ST_SECTION: return "SECTION";
        case ST_FUNC: return "FUNC";
        case ST_OBJECT: return "OBJECT";
    }
    return "";
}
std::string symbol_bind_to_string(uint32_t value){
    switch(value){
        case STB_GLOBAL: return "GLOBAL";
        case STB_LOCAL: return "LOCAL";
    }
    return "";
}
std::string section_index_to_string(uint16_t value){
    switch(value){
        case SHELF_SHN_UNDEF: return "UND";
        case SHELF_SHN_ABS: return "ABS";
    }
    return std::to_string(value);
}
void ShelfPrinter::printSymbols(std::ostream& os) {
    os << "#.symtab\n";
    os << std::left
       << std::setw(4) << "Num" 
       << std::setw(8) << "Value" << "  "
       << std::setw(8) << "Size" << "  "
       << std::setw(8) << "Type" << "  "
       << std::setw(8) << "Bind" << "  "
       << std::setw(8) << "Ndx" << "  "
       << std::setw(16) << "Name" << "  "
       << "\n";

    const auto& syms = reader.getSymbols();
    for (size_t i = 0; i < syms.size(); i++) {
        const auto& s = syms[i];
        os << std::left << std::setw(4) << i;
        os << to_hex_msb(s.value) << "  ";
        os << to_hex_msb(s.size) << "  ";
        os << std::left << std::setw(8) << symbol_type_to_string(s.type) << "  ";
        os << std::left << std::setw(8) << symbol_bind_to_string(s.bind) << "  ";
        os << std::left << std::setw(8) <<  section_index_to_string(s.sectionIndex) << "  ";
        os << std::left << std::setw(16) << s.name << "\n";
    }
}


std::string reloc_type_to_string(uint32_t value){
    switch(value){
        case R_SHELF_NONE: return "R_SHELF_NONE";
        case R_SHELF_DIRECT: return "R_SHELF_DIRECT";
        case R_SHELF_PC_REL: return "R_SHELF_PC_REL";
    }
    return "";
}
void ShelfPrinter::printRelocations(std::ostream& os) {
    const auto& shs = reader.getSectionHeaders();
    for (size_t i = 0; i < shs.size(); i++) {
        auto relocs = reader.getRelocations(i);
        if (shs[i].type == SHELF_RELOC) {
            auto relocs = reader.getRelocations(shs[i].info);
            os << "#" << shs[i].name << "\n";
            os << std::left
                << std::setw(8) << "Offset" << "  "
                << std::setw(16) << "Type" << "  "
                << std::setw(8) << "SymNdx" << "  "
                << std::setw(16) << "(SymName)" << "  "
                << std::setw(8) << "Addend" << "  "
                << "\n";
            for (const auto& r : relocs) {
                os << to_hex_msb(r.offset) << "  ";
                os << std::left << std::setw(16) << reloc_type_to_string(r.type) << "  ";
                os << std::left << std::setw(8) << r.symIndex << "  ";
                os << std::left << std::setw(16) << "(" + r.symbolName + ")" << "  ";
                os << std::left << std::setw(8) << r.addend << "\n";
            }
        }
    }
}
