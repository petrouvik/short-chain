#pragma once
#include <string>
#include <vector>
#include <map>
#include "relocation.hpp"
#include "expression.hpp"
struct Symbol;
struct Section;
struct ForwardRef;
class Assembler {
public:
    Assembler();
    ~Assembler();
    //Encode and emit instruction to current section
    void processInstruction(const std::string &mnemonic, const std::vector<std::string> &operands);

    //Process directives (except .equ)
    void processDirective(const std::string &directive, const std::vector<std::string> &args);

    //Create a new symbol or define an existing undefined symbol with the value equal to the LC of current section
    void defineLabel(const std::string &label);

    //Write binary to specified file and write a text representation of that binary
    void writeOutput(const std::string &filename);

    //Process .equ directive
    void processEqu(const std::string& name, Expression* expression);

    //For each referenced symbol in the .equ's expression create a new symbol if it doesn't exist yet
    void symbolUsageEquHandler(const std::string& name);

private:
    std::map<std::string, Symbol*> symbolMap;
    std::vector<Symbol*> symbolList;

    std::vector<Section*> sectionList;
    Section* currentSection = nullptr;
    Section* undefinedSection;
    Section* absoluteSection;

    struct UnresolvedEqu {
        Symbol* symbol;
        Expression* expression;
    };
    std::vector<UnresolvedEqu> unresolvedEqus;

    //Handles symbol usage - makes relocations/patches for defined symbols or makes a forward reference entry for undefined symbols
    void symbolUsageHandler(const std::string& name, int offset, Relocation::RELTYPE reltype);
    
    //Patches contents based on forward reference entries
    void backPatch();
    
    //Patch a forward reference that uses an absolute symbol
    void patchForwardRef(const Symbol* sym, const ForwardRef& fr);

    //All relocation entries that use local symbols are corrected to use that symbol's section symbol instead
    void correctRelocations();

    //Update a relocation to be section relative
    void makeSectionRelative(Relocation& rel);

    //Called after the assembler reads the whole file, but before writing the output file. Tidies the internal data so it can be directly writting to the output.
    void cleanup();

    //Try to resolve the given absolute symbol. If the flag "addToUnresolved" is set, then the symbol is added to unresolved symbols if it can't be currently resolved
    bool tryResolveAbsolute(Symbol* symbol, Expression* expression, bool addToUnresolved);
    
    //Resolves all the unresolved absolute symbols
    void resolveAbsolutes();
};