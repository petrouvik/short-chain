#include "shelfReader.hpp"
#include <fstream>
#include <cstring>
#include "shelf.hpp"
ShelfReader::ShelfReader(const std::string& filename){
    parseFile(filename);
}
void ShelfReader::parseFile(const std::string& filename) {
    std::ifstream in(filename, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    /*--- Read and validate header --- */
    ShelfHeader hdr{};
    in.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
    if (!in || std::memcmp(hdr.magic, "SHELF", 5) != 0) {
        throw std::runtime_error("Invalid SHELF file: bad magic");
    }

    /*--- Load section headers --- */
    sectionHeaders.resize(hdr.shnum);
    in.seekg(hdr.shoff, std::ios::beg);
    for (size_t i = 0; i < hdr.shnum; i++) {
        ShelfSectionHeader sh{};
        in.read(reinterpret_cast<char*>(&sh), sizeof(sh));
        if (!in) throw std::runtime_error("Failed reading section header");
        ResolvedSectionHeader rsh{};
        rsh.nameOffset = sh.nameOffset;
        rsh.type = sh.type;
        rsh.offset = sh.offset;
        rsh.size = sh.size;
        rsh.info = sh.info;
        sectionHeaders[i] = rsh;
    }

    /*--- Load section contents --- */
    sectionContents.resize(hdr.shnum);
    for (size_t i = 0; i < hdr.shnum; i++) {
        auto& sh = sectionHeaders[i];
        if (sh.size == 0) {
            sectionContents[i] = {};
            continue;
        }

        std::vector<uint8_t> buf(sh.size);
        in.seekg(sh.offset, std::ios::beg);
        in.read(reinterpret_cast<char*>(buf.data()), sh.size);
        if (!in) throw std::runtime_error("Failed reading section contents");
        sectionContents[i] = std::move(buf);
    }

    /*--- Resolve section names  --- */
    if (hdr.shstrndx >= sectionHeaders.size())
        throw std::runtime_error("Invalid shstrndx in file header");

    const auto& shstrtab = sectionContents[hdr.shstrndx];

    for (size_t i = 0; i < sectionHeaders.size(); i++) {
        auto& sh = sectionHeaders[i];
        if (sh.nameOffset >= shstrtab.size())
            throw std::runtime_error("Invalid string offset in shstrtab");
        const char* namePtr = reinterpret_cast<const char*>(&shstrtab[sh.nameOffset]);
        sectionHeaders[i].name = std::string(namePtr);
    }
    
    /*--- Load symbol table --- */
    size_t symtabIndex = -1UL, symstrtabIndex = -1UL;
    for (size_t i = 0; i < sectionHeaders.size(); i++) {
        if (sectionHeaders[i].type == SHELF_SYMTAB) symtabIndex = i;
        if (sectionHeaders[i].type == SHELF_SYMSTRTAB) symstrtabIndex = i;
    }

    if(symtabIndex == -1UL || symstrtabIndex == -1UL){
        throw std::runtime_error("Missing symbol table section and/or symbol string table section");
    }

    auto& sec = sectionContents[symtabIndex];
    size_t numSymbols = sec.size() / sizeof(ShelfSymbol);
    symbols.reserve(numSymbols);

    for (size_t i = 0; i < numSymbols; i++) {
        const ShelfSymbol* shelfSym = reinterpret_cast<const ShelfSymbol*>(sec.data()) + i;
        if (shelfSym->nameOffset >= sectionContents[symstrtabIndex].size())
            throw std::runtime_error("Invalid string offset in shstrtab");
        const char* namePtr = reinterpret_cast<const char*>(&(sectionContents[symstrtabIndex])[shelfSym->nameOffset]);
        ResolvedSymbol sym{
            std::string(namePtr),
            shelfSym->value,
            shelfSym->size,
            shelfSym->type,
            shelfSym->bind,
            shelfSym->shndx
        };
        symbols.push_back(std::move(sym));
    }
    

    /*--- Load relocations --- */
    for (size_t i = 0; i < sectionHeaders.size(); i++) {
        if (sectionHeaders[i].type != SHELF_RELOC) continue;

        auto& sec = sectionContents[i];
        size_t numRelocs = sec.size() / sizeof(ShelfRelocation);
        std::vector<ResolvedRelocation> relocs;
        relocs.reserve(numRelocs);

        for (size_t j = 0; j < numRelocs; j++) {
            const ShelfRelocation* raw = reinterpret_cast<const ShelfRelocation*>(sec.data()) + j;
            std::string symName = (raw->symIndex < symbols.size())
                                  ? symbols[raw->symIndex].name
                                  : "<invalid>";
            ResolvedRelocation rr{
                raw->offset,
                raw->type,
                raw->addend,
                raw->symIndex,
                symName
            };
            relocs.push_back(std::move(rr));
        }
        relocations[sectionHeaders[i].info] = std::move(relocs);
    }
}

std::vector<ShelfReader::ResolvedSectionHeader>& ShelfReader::getSectionHeaders(){
    return sectionHeaders;

}
std::vector<uint8_t>& ShelfReader::getSectionContents(size_t sectionIndex){
    return sectionContents[sectionIndex];
}
std::vector<ShelfReader::ResolvedSymbol>& ShelfReader::getSymbols(){
    return symbols;
}
std::vector<ShelfReader::ResolvedRelocation>& ShelfReader::getRelocations(size_t sectionIndex){
    return relocations[sectionIndex];
}