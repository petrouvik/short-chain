#include "shelfWriter.hpp"
#include <fstream>
#include <cstring>
#include "section.hpp"
#include "symbol.hpp"
#include "shelf.hpp"
#include "relocation.hpp"

ShelfWriter::ShelfWriter(std::vector<Section*>& sections, std::vector<Symbol*>& symbols, Section* absoluteSection, Section* undefinedSection)
    : sectionList(sections),
    symbolList(symbols),
    undefinedSection(undefinedSection),
    absoluteSection(absoluteSection),
    fileOffset(0) {}

void ShelfWriter::buildSectionNameTable(){
    uint32_t offset = 0;

    for (auto sec : sectionList) {
        if(sec != absoluteSection){
            shstrOffsets[sec->name] = offset;
            shstrtab.insert(shstrtab.end(), sec->name.begin(), sec->name.end());
            shstrtab.push_back('\0');
            offset += sec->name.size() + 1;
        }
    }
    //Add .symtab string
    std::string symTabString = ".symtab";
    shstrOffsets[symTabString] = offset;
    shstrtab.insert(shstrtab.end(), symTabString.begin(), symTabString.end());
    shstrtab.push_back('\0');
    offset += symTabString.size() + 1;
    //Add .shstrtab string
    std::string shstrTabString = ".shstrtab";
    shstrOffsets[shstrTabString] = offset;
    shstrtab.insert(shstrtab.end(), shstrTabString.begin(), shstrTabString.end());
    shstrtab.push_back('\0');
    offset += shstrTabString.size() + 1;
    //Add .symstrtab string
    std::string symstrTabString = ".symstrtab";
    shstrOffsets[symstrTabString] = offset;
    shstrtab.insert(shstrtab.end(), symstrTabString.begin(), symstrTabString.end());
    shstrtab.push_back('\0');
    offset += symstrTabString.size() + 1;
}
void ShelfWriter::buildSymbolNameTable(){
    uint32_t offset = 0;
    for (auto& sym : symbolList) {
        symstrOffsets[sym->name] = offset;
        symstrtab.insert(symstrtab.end(), sym->name.begin(), sym->name.end());
        symstrtab.push_back('\0');
        offset += sym->name.size() + 1;
    }
}

void ShelfWriter::addProgramSections(){
    fileOffset = sizeof(ShelfHeader);
    for (size_t i = 0; i < sectionList.size(); ++i) {
        Section* sec = sectionList[i];
        if (sec == absoluteSection) continue;
        bool hasContent = sec->contents.size() > 0 ? true : false;
        if(hasContent) sectionContents.push_back(sec->contents);

        ShelfSectionHeader sh{};
        sh.nameOffset = shstrOffsets[sec->name];
        sh.type = sec->name == "" ? SHELF_NULL : SHELF_PROGBITS;
        sh.offset = hasContent ? fileOffset : 0;
        sh.size = sec->contents.size();
        sh.info = 0;
        sh.address = 0;
        sectionHeaders.push_back(sh);

        sec->index = sectionHeaders.size() - 1; //update section's index
        
        fileOffset += sh.size;

        if (!sec->relocations.empty()) {
            addRelocationSection(sec);
        }

    }

}
void ShelfWriter::addRelocationSection(Section* sec){
    std::vector<ShelfRelocation> relocEntries;
    for (auto& r : sec->relocations) {
        ShelfRelocation sr{};
        sr.offset = r.offset;
        sr.symIndex = r.symbol->index;
        sr.type = R_SHELF_DIRECT; //this is the only kind of relocation that can show up currently
        sr.addend = r.addend;
        relocEntries.push_back(sr);
    }

    std::vector<uint8_t> relocContent(
        (uint8_t*)relocEntries.data(),
        (uint8_t*)relocEntries.data() + relocEntries.size() * sizeof(ShelfRelocation)
    );
    sectionContents.push_back(std::move(relocContent));

    ShelfSectionHeader rsh{};
    std::string relocName = ".rela" + sec->name;
    if (shstrOffsets.find(relocName) == shstrOffsets.end()) {
        shstrOffsets[relocName] = shstrtab.size();
        shstrtab.insert(shstrtab.end(), relocName.begin(), relocName.end());
        shstrtab.push_back('\0');
    }

    rsh.nameOffset = shstrOffsets[relocName];
    rsh.type = SHELF_RELOC;
    rsh.offset = fileOffset;
    rsh.size = relocEntries.size() * sizeof(ShelfRelocation);
    rsh.info = sectionHeaders.size() - 1; //index of the section this relocation applies to
    rsh.address = 0;
    sectionHeaders.push_back(rsh);

    fileOffset += rsh.size;
}
void ShelfWriter::addSymbolTableSection(){
    ShelfSectionHeader symtabHeader{};
    symtabHeader.nameOffset = shstrOffsets[".symtab"];
    symtabHeader.type = SHELF_SYMTAB;
    symtabHeader.offset = fileOffset;
    symtabHeader.info = 0;
    symtabHeader.address = 0;
    std::vector<ShelfSymbol> symtab;
    for (auto& sym : symbolList) {
        ShelfSymbol s{};
        s.nameOffset = symstrOffsets[sym->name];
        s.value = sym->value;
        s.size = sym->size;
        s.type = sym->type == Symbol::SCTN ? ST_SECTION : ST_NOTYPE;
        s.bind = sym->binding == Symbol::GLOBAL ? STB_GLOBAL : STB_LOCAL;
        if(sym->section == absoluteSection){
            s.shndx = SHELF_SHN_ABS;
        }else if(sym->section == undefinedSection){
            s.shndx = SHELF_SHN_UNDEF;
        }else{
            s.shndx = sym->section->index;
        }
        symtab.push_back(s);
    }
    symtabHeader.size = symtab.size() * sizeof(ShelfSymbol);
    sectionHeaders.push_back(symtabHeader);
    fileOffset += symtabHeader.size;
    sectionContents.push_back(std::vector<uint8_t>((uint8_t*)symtab.data(), (uint8_t*)symtab.data() + symtabHeader.size));
}
void ShelfWriter::addStringTableSections(){
    ShelfSectionHeader shstrHeader{};
    shstrHeader.nameOffset = shstrOffsets[".shstrtab"];
    shstrHeader.type = SHELF_STRTAB;
    shstrHeader.offset = fileOffset;
    shstrHeader.size = shstrtab.size();
    shstrHeader.info = 0;
    shstrHeader.address = 0;
    sectionHeaders.push_back(shstrHeader);
    std::vector<uint8_t> shstrContent(shstrtab.begin(), shstrtab.end());
    sectionContents.push_back(std::move(shstrContent));
    fileOffset += shstrtab.size();

    ShelfSectionHeader symstrHeader{};
    symstrHeader.nameOffset = shstrOffsets[".symstrtab"];
    symstrHeader.type = SHELF_SYMSTRTAB;
    symstrHeader.offset = fileOffset;
    symstrHeader.size = symstrtab.size();
    symstrHeader.info = 0;
    symstrHeader.address = 0;
    sectionHeaders.push_back(symstrHeader);
    std::vector<uint8_t> symstrtabContent(symstrtab.begin(), symstrtab.end());
    sectionContents.push_back(std::move(symstrtabContent));
    fileOffset += symstrtab.size();
}
void ShelfWriter::write(const std::string& filename){
    buildSectionNameTable();
    buildSymbolNameTable();
    addProgramSections();
    addSymbolTableSection();
    addStringTableSections();

    std::ofstream out(filename, std::ios::binary);
    if (!out) throw std::runtime_error("Cannot open output file");

    ShelfHeader header{};
    memcpy(header.magic, "SHELF", 5);
    header.shoff = fileOffset;
    header.shnum = sectionHeaders.size();
    header.shstrndx = sectionHeaders.size() - 2;
    out.write((char*)&header, sizeof(header));

    //Section contents
    for (auto& content : sectionContents) {
        out.write((char*)content.data(), content.size());
    }
    //Section headers
    for (auto& sh : sectionHeaders) {
        out.write((char*)&sh, sizeof(sh));
    }

    out.close();
}