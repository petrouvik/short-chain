#pragma once
#include <string>
#include <vector>
#include <map>
#include "shelfReader.hpp"

struct Section;
struct Symbol;
class Linker{
public:
    Linker(){}
    //Read an object file and load its contents into internal structures.
    void readFile(const std::string& filename);

    //Add what should be the starting address of a section. (only in hex mode, otherwise ignored)
    void addSectionStartingAddress(const std::string& name, int address);
    
    //Generate the executable. (hex mode)
    void link(const std::string& outputFilename);

    //Generate the object file. (relocatable mode)
    void linkRelocatable(const std::string& outputFilename);

private:
    //Large section header table made by concatenating section header tables from object files.
    std::vector<ShelfReader::ResolvedSectionHeader> sectionHeaders;
    //Index in the large SHT from which the current object file's SHT is written.
    size_t sectionIndexOffset = 0;

    //Large symbol table made by concatenating symbol tables from object files.
    std::vector<ShelfReader::ResolvedSymbol> symbols;
    //Index in the large ST from which the current object file's ST is written.
    size_t symbolIndexOffset = 0;

    //Maps the index of the section in the large SHT to a vector of relocation entries.
    std::map<size_t, std::vector<ShelfReader::ResolvedRelocation>> relocations;

    //Maps the index of the section in the large SHT to content of that section.
    std::map<size_t, std::vector<uint8_t>> sectionContents;

    /* --- Hex mode --- */

    //Stores user-specified starting addresses of sections are stored. (hex mode)
    std::map<std::string, int> sectionStartingAddresses;

    //Resolve undefined global symbols. (hex mode)
    void resolveUndefinedSymbols();

    //Maps section names to their size post-merge. (hex mode)
    std::map<std::string, size_t> mergedSectionSizes;
    //Calculate post-merge section sizes. The result gets stored in mergedSectionSizes. (hex mode)
    void computeMergedSectionSizes();

    //Maps section names to their final address. (hex mode)
    std::map<std::string, int> mergedSectionAddresses;
    //Calculate the start addresses of merged sections based on start addresses specified by the user. (hex mode)
    void computeSectionAddresses();

    //Assign a starting address for each large SHT entry. (hex mode)
    void assignFinalSectionAddresses();
    
    //Patch contents based on relocation entries. (hex mode)
    void applyRelocations();

    //Write the text representation to a text file. (hex mode)
    void printLinkedFile(const std::string& filename) const;

    /* --- Relocatable mode --- */

    //Maps the index of the section in the large SHT to the Section struct in writerSections vector.
    //Sections with the same name will be mapped to the same struct. (relocatable mode)
    std::map<size_t, Section*> indexSectionMap;

    //Populates writerSections. (relocatable mode)
    void generateWriterSections();
    struct SectionCurrentOffset{
        Section* section;
        int currentOffset;
    };
    //Stores the current offset at which the current section's content will be in the merged section. (relocatable mode)
    std::map<std::string, SectionCurrentOffset> sectionNameOffsetMap;

    //Stores the offset at which the section's content will be in the merged section. (relocatable mode)
    std::map<size_t, int> sectionIndexOffsetMap;
    
    //This is passed as an argument to the writer. (relocatable mode)
    std::vector<Section*> writerSections;
    //This is passed as an argument to the writer. (relocatable mode)
    Section* undefinedSection;
    //This is passed as an argument to the writer. (relocatable mode)
    Section* absoluteSection;

    //Checks if there are multiple global symbols that share a name and are defined, but allows multiple defined locals that share a name. (relocatable mode)
    void checkDuplicateGlobals();

    //Maps the index of the symbol in the large ST to the Symbol struct in writerSymbols vector. (relocatable mode)
    std::map<size_t, Symbol*> indexSymbolMap;

    //Maps the index of the SCTN symbol in the large ST to the offset in the merged section. (relocatable mode)
    std::map<size_t, int> symbolSCTNOffset;
    //Maps the name of the section to the Symbol struct in writerSymbols. (relocatable mode)
    std::map<std::string, Symbol*> symbolSCTNName;
    //This is passed as an argument to the writer. (relocatable mode)
    std::vector<Symbol*> writerSymbols;
    //The first symbol which we will add to the writerSymbols. (relocatable mode)
    Symbol* emptySymbol;
    //Populates writerSymbols
    void generateWriterSymbols();
    //Populates the relocations fields in Section structs
    void generateWriterRelocations();

    //Optional: helper for debugging.
    void debugPrintState() const;
};