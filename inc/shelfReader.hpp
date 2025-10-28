#pragma once
#include <vector>
#include <cstdint>
#include <string>
#include <map>
struct ShelfSectionHeader;

class ShelfReader {
public:
    //Read given file
    explicit ShelfReader(const std::string& filename);

    /*--- Section ---*/
    struct ResolvedSectionHeader{
        std::string name;
        uint32_t nameOffset;
        uint32_t type;
        uint32_t offset;
        uint32_t size;
        uint32_t info;  
        uint32_t address = 0;
    };
    std::vector<ResolvedSectionHeader>& getSectionHeaders();
    std::vector<uint8_t>& getSectionContents(size_t sectionIndex);
    
    /*--- Symbol ---*/
    struct ResolvedSymbol {
        std::string name;
        uint32_t value;
        uint32_t size;
        uint8_t type;
        uint8_t bind;
        uint16_t sectionIndex;
    };
    std::vector<ResolvedSymbol>& getSymbols();

    /*--- Relocation ---*/
    struct ResolvedRelocation {
        uint32_t offset;
        uint8_t type;
        int32_t addend;
        uint32_t symIndex;
        std::string symbolName;
    };
    std::vector<ResolvedRelocation>& getRelocations(size_t sectionIndex);

private:

    std::vector<ResolvedSectionHeader> sectionHeaders;
    std::vector<std::vector<uint8_t>> sectionContents;

    std::vector<ResolvedSymbol> symbols;
    std::map<size_t, std::vector<ResolvedRelocation>> relocations;

    void parseFile(const std::string& filename);
};