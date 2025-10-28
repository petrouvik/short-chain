#pragma once
#include <vector>
#include <map>
#include <string>
#include <cstdint>

struct Section;
struct Symbol;
struct ShelfSectionHeader;

class ShelfWriter {
public:
    ShelfWriter(std::vector<Section*>& sections, std::vector<Symbol*>& symbols, Section* absoluteSection, Section* undefinedSection);
    void write(const std::string& filename);

private:
    std::vector<Section*>& sectionList;
    std::vector<Symbol*>& symbolList;
    Section* undefinedSection;
    Section* absoluteSection;
    
    std::vector<char> shstrtab;
    std::vector<char> symstrtab;
    std::map<std::string, uint32_t> shstrOffsets;
    std::map<std::string, uint32_t> symstrOffsets;

    std::vector<ShelfSectionHeader> sectionHeaders;
    std::vector<std::vector<uint8_t>> sectionContents;

    uint32_t fileOffset = 0;

    void buildSectionNameTable();
    void buildSymbolNameTable();
    void addProgramSections();
    void addRelocationSection(Section* sec);
    void addSymbolTableSection();
    void addStringTableSections();
};