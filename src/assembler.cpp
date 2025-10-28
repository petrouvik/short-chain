#include "assembler.hpp"
#include <iostream>
#include <map>
#include <fstream>
#include <cstring>
#include "instructionEncoder.hpp"
#include "shelf.hpp"
#include "section.hpp"
#include "forwardRef.hpp"
#include "relocation.hpp"
#include "symbol.hpp"
#include "shelfWriter.hpp"
#include "shelfPrinter.hpp"
#include "symbolUndefinedExpressionException.hpp"
Assembler::Assembler(){
    Section* undSection = new Section(sectionList.size(), "");
    sectionList.push_back(undSection);
    undefinedSection = undSection;
    ///TODO: set absolute section to something
    absoluteSection = nullptr;

    Symbol* undSymbol = new Symbol(symbolList.size(), "", undefinedSection);
    symbolMap[""] = undSymbol;
    symbolList.push_back(undSymbol);
    
}
void Assembler::backPatch() {
    for (auto& sym : symbolList) {

        //Makes sure that all local symbols are defined
        if (!sym->defined) {
            if (!((sym->binding == Symbol::GLOBAL) || sym->external || sym->name == "")) {
                throw std::runtime_error("Undefined symbol: " + sym->name);
            }
        }

        //Make undefined extern symbols global. (If the symbol is defined and extern it should stay local, unless otherwise specified)
        if(!sym->defined && sym->external){
            sym->binding = Symbol::GLOBAL;
        }

        for (const auto& fr : sym->forwardRefs) {
            //Only allow absolute symbols to be used as displacements
            if (fr.type == Relocation::DISP && sym->section != absoluteSection) {
                    throw std::runtime_error("Displacement requires absolute value for symbol: " + sym->name);
            }

            //Resolve the forward reference by either patching or adding a relocation entry for the linker to patch
            if (sym->section == undefinedSection) {
                Relocation reloc(fr.offset, fr.type, 0, sym);
                fr.section->relocations.push_back(reloc);
            
            } else if (sym->section == absoluteSection) {
                patchForwardRef(sym, fr);
            } else {
                Relocation reloc(fr.offset, fr.type, 0, sym);
                fr.section->relocations.push_back(reloc);
            }
        }
    }
}
void Assembler::patchForwardRef(const Symbol* sym, const ForwardRef& fr) {
    if (fr.type == Relocation::DIRECT) {
        std::vector<uint8_t> bytes = InstructionEncoder::word(sym->value);
        if (static_cast<std::size_t>(fr.offset) + 4 > fr.section->contents.size()) {
            throw std::runtime_error("Internal error: Forward reference patch out of bounds");
        }
        for (size_t i = 0; i < 4; i++) {
            fr.section->contents[fr.offset + i] = bytes[i];
        }

    } else if (fr.type == Relocation::DISP) {
        int value = sym->value;
        if (value < InstructionEncoder::MIN_DISP || value > InstructionEncoder::MAX_DISP) {
            throw std::out_of_range("Displacement out of 12-bit signed range for symbol: " + sym->name);
        }
        if (static_cast<std::size_t>(fr.offset) + 2 > fr.section->contents.size()) {
            throw std::runtime_error("Internal error: Forward reference DISP patch out of bounds");
        }
        fr.section->contents[fr.offset] = (fr.section->contents[fr.offset] & 0xF0) | ((value >> 8) & 0x0F);
        fr.section->contents[fr.offset + 1] = value & 0xFF;
    }
}
void Assembler::correctRelocations() {
    for (auto& sec : sectionList) {

        for (auto& rel : sec->relocations) {

            if (rel.symbol->binding == Symbol::LOCAL) {
                makeSectionRelative(rel);
            }
        }
    }
}
void Assembler::makeSectionRelative(Relocation& rel) {
    Symbol* sym = rel.symbol;

    if (!sym || !sym->section) {
        throw std::runtime_error("Internal error: symbol has no section");
    }

    auto it = symbolMap.find(sym->section->name);
    if (it == symbolMap.end()) {
        throw std::runtime_error("No SCTN symbol found for section: " + sym->section->name);
    }
    Symbol* sectionSym = it->second;

    rel.addend = sym->value;
    rel.symbol = sectionSym;
}

void Assembler::cleanup(){
    //The following operations MUST be called in this order
    resolveAbsolutes();
    backPatch();
    correctRelocations();
}


void Assembler::symbolUsageHandler(const std::string& name, int offset, Relocation::RELTYPE reltype) {
    auto it = symbolMap.find(name);

    if (it != symbolMap.end()) {
        Symbol* sym = it->second;
        if (sym->defined) {
            //Only allow absolute symbols to be used as displacements
            if(reltype == Relocation::DISP && sym->section != absoluteSection){
                throw std::runtime_error("Non-absolute symbol used in an absolute only field");
            }

            if(sym->section == absoluteSection){
                //Don't make a relocation entry for absolute symbols, resolve them immediately instead
                ForwardRef ref(offset, reltype, 0, currentSection);
                patchForwardRef(sym, ref);
            }else{
                Relocation reloc(offset, reltype, 0, sym);
                currentSection->relocations.push_back(reloc);
            }
            
        } else {
            // Symbol exists but is not defined
            ForwardRef ref(offset, reltype, 0, currentSection);
            sym->forwardRefs.push_back(ref);
        }
    } else {
        // Symbol doesn’t exist yet
        Symbol* sym = new Symbol(symbolList.size(), name, undefinedSection);
        sym->binding = Symbol::LOCAL;
        sym->defined = false;
        sym->external = false;

        symbolMap[name] = sym;
        symbolList.push_back(sym);

        ForwardRef ref(offset, reltype, 0, currentSection);
        sym->forwardRefs.push_back(ref);
    }
}
void Assembler::processInstruction(const std::string &mnemonic,const std::vector<std::string> &operands){
    // std::cout << mnemonic;
    // for (const auto &op : operands) {
    //     std::cout << " " << op;
    // }
    // std::cout << std::endl;
    try{
        if(currentSection == nullptr){
            throw std::logic_error("No section was started before writing content");
        }
        if (mnemonic == "halt") {
            currentSection->emitBytes(InstructionEncoder::halt());
        }
        else if (mnemonic == "int") {
            currentSection->emitBytes(InstructionEncoder::intr());
        }
        else if (mnemonic == "iret") {
            currentSection->emitBytes(InstructionEncoder::iret());
        }
        else if (mnemonic == "ret") {
            currentSection->emitBytes(InstructionEncoder::ret());
        }
        else if (mnemonic == "calllit") {
            int literal = std::stoi(operands[0]);
            currentSection->emitBytes(InstructionEncoder::call(literal));
        }
        else if (mnemonic == "callsym") {
            std::string symName = operands[0];
            int operandOffset = currentSection->locationCounter + InstructionEncoder::CALL_OP_OFFSET;
            
            currentSection->emitBytes(InstructionEncoder::call(0));
            symbolUsageHandler(symName, operandOffset, Relocation::DIRECT);
        }
        else if (mnemonic == "jmplit") {
            int literal = std::stoi(operands[0]);
            currentSection->emitBytes(InstructionEncoder::jmp(literal));
        }
        else if (mnemonic == "jmpsym") {
            std::string symName = operands[0];
            int operandOffset = currentSection->locationCounter + InstructionEncoder::JMP_OP_OFFSET;
            
            currentSection->emitBytes(InstructionEncoder::jmp(0));
            symbolUsageHandler(symName, operandOffset, Relocation::DIRECT);
        }
        else if (mnemonic == "beqlit" || mnemonic == "bnelit" || mnemonic == "bgtlit") {
            int gpr1 = std::stoi(operands[0]);
            int gpr2 = std::stoi(operands[1]);
            int literal = std::stoi(operands[2]);
            if (mnemonic == "beqlit") currentSection->emitBytes(InstructionEncoder::beq(gpr1, gpr2, literal));
            else if(mnemonic == "bnelit") currentSection->emitBytes(InstructionEncoder::bne(gpr1, gpr2, literal));
            else if(mnemonic == "bgtlit") currentSection->emitBytes(InstructionEncoder::bgt(gpr1, gpr2, literal));
        }
        else if (mnemonic == "beqsym" || mnemonic == "bnesym" || mnemonic == "bgtsym") {
            int gpr1 = std::stoi(operands[0]);
            int gpr2 = std::stoi(operands[1]);
            std::string symName = operands[2];

            int operandOffset = currentSection->locationCounter + InstructionEncoder::CONDJMP_OP_OFFSET;
            
            if(mnemonic == "beqsym") currentSection->emitBytes(InstructionEncoder::beq(gpr1, gpr2, 0));
            else if(mnemonic == "bnesym") currentSection->emitBytes(InstructionEncoder::bne(gpr1, gpr2, 0));
            else if(mnemonic == "bgtsym") currentSection->emitBytes(InstructionEncoder::bgt(gpr1, gpr2, 0));
            symbolUsageHandler(symName, operandOffset, Relocation::DIRECT);
        }
        else if (mnemonic == "push" || mnemonic == "pop" ||
                 mnemonic == "not") 
        {
            int gpr = std::stoi(operands[0]);
            if (mnemonic == "push") currentSection->emitBytes(InstructionEncoder::push(gpr));
            else if (mnemonic == "pop") currentSection->emitBytes(InstructionEncoder::pop(gpr));
            else if (mnemonic == "not") currentSection->emitBytes(InstructionEncoder::bit_not(gpr));
        }
        else if (mnemonic == "xchg" || 
                mnemonic == "add" || mnemonic == "sub" || 
                mnemonic == "mul" || mnemonic == "div" ||
                mnemonic == "and" || mnemonic == "or" || mnemonic == "xor" ||
                mnemonic == "shl" || mnemonic == "shr") 
        {
            int gprS = std::stoi(operands[0]);
            int gprD = std::stoi(operands[1]);

            if (mnemonic == "xchg") currentSection->emitBytes(InstructionEncoder::xchg(gprS, gprD));
            else if (mnemonic == "add") currentSection->emitBytes(InstructionEncoder::add(gprS, gprD));
            else if (mnemonic == "sub") currentSection->emitBytes(InstructionEncoder::sub(gprS, gprD));
            else if (mnemonic == "mul") currentSection->emitBytes(InstructionEncoder::mul(gprS, gprD));
            else if (mnemonic == "div") currentSection->emitBytes(InstructionEncoder::div(gprS, gprD));
            else if (mnemonic == "and") currentSection->emitBytes(InstructionEncoder::bit_and(gprS, gprD));
            else if (mnemonic == "or") currentSection->emitBytes(InstructionEncoder::bit_or(gprS, gprD));
            else if (mnemonic == "xor") currentSection->emitBytes(InstructionEncoder::bit_xor(gprS, gprD));
            else if (mnemonic == "shl") currentSection->emitBytes(InstructionEncoder::shl(gprS, gprD));
            else if (mnemonic == "shr") currentSection->emitBytes(InstructionEncoder::shr(gprS, gprD));
        }
        else if (mnemonic == "ldimm") {
            int literal = std::stoi(operands[0]);
            int gpr = std::stoi(operands[1]);
            currentSection->emitBytes(InstructionEncoder::ld_immediate(gpr, literal));
        }
        else if (mnemonic == "ldsym") {
            std::string symName = operands[0];
            int gpr = std::stoi(operands[1]);

            int operandOffset = currentSection->locationCounter + InstructionEncoder::LD_IMM_OP_OFFSET;
            currentSection->emitBytes(InstructionEncoder::ld_immediate(gpr, 0));
            symbolUsageHandler(symName, operandOffset, Relocation::DIRECT);
        }
        else if (mnemonic == "ldlit") {
            int literal = std::stoi(operands[0]);
            int gpr = std::stoi(operands[1]);
            currentSection->emitBytes(InstructionEncoder::ld_memory(gpr, literal));
        }
        else if (mnemonic == "ldsymabs") {
            std::string symName = operands[0];
            int gpr = std::stoi(operands[1]);

            int operandOffset = currentSection->locationCounter + InstructionEncoder::LD_MEM_OP_OFFSET;
            currentSection->emitBytes(InstructionEncoder::ld_memory(gpr, 0));
            symbolUsageHandler(symName, operandOffset, Relocation::DIRECT);
        }
        else if (mnemonic == "ldreg") {
            int reg = std::stoi(operands[0]);
            int gpr = std::stoi(operands[1]);
            currentSection->emitBytes(InstructionEncoder::ld_register(gpr, reg));
        }
        else if (mnemonic == "ldind") {
            int reg = std::stoi(operands[0]);
            int gpr = std::stoi(operands[1]);
            currentSection->emitBytes(InstructionEncoder::ld_register_indirect(gpr, reg));
        }
        else if (mnemonic == "ldindlit") {
            int reg = std::stoi(operands[0]);
            int literal = std::stoi(operands[1]);
            int gpr = std::stoi(operands[2]);
            currentSection->emitBytes(InstructionEncoder::ld_register_indirect_disp(gpr, reg, literal));
        }
        else if (mnemonic == "ldindsym") {
            int reg = std::stoi(operands[0]);
            std::string symName = operands[1];
            int gpr = std::stoi(operands[2]);

            int operandOffset = currentSection->locationCounter + InstructionEncoder::LD_IND_DISP_OP_OFFSET;
            currentSection->emitBytes(InstructionEncoder::ld_register_indirect_disp(gpr, reg, 0));
            symbolUsageHandler(symName, operandOffset, Relocation::DISP);
        }
        else if (mnemonic == "stlit") {
            int gpr = std::stoi(operands[0]);
            int literal = std::stoi(operands[1]);
            currentSection->emitBytes(InstructionEncoder::st_direct(gpr, literal));
        }
        else if (mnemonic == "stsymabs") {
            int gpr = std::stoi(operands[0]);
            std::string symName = operands[1];
            
            int operandOffset = currentSection->locationCounter + InstructionEncoder::ST_DIR_OP_OFFSET;
            
            currentSection->emitBytes(InstructionEncoder::st_direct(gpr, 0));
            symbolUsageHandler(symName, operandOffset, Relocation::DIRECT);
        }
        else if (mnemonic == "stind") {
            int gpr = std::stoi(operands[0]);
            int reg = std::stoi(operands[1]);
            currentSection->emitBytes(InstructionEncoder::st_register_indirect(gpr, reg));
        }
        else if (mnemonic == "stindlit") {
            int gpr = std::stoi(operands[0]);
            int reg = std::stoi(operands[1]);
            int literal = std::stoi(operands[2]);
            currentSection->emitBytes(InstructionEncoder::st_register_indirect_disp(gpr, reg, literal));
        }
        else if (mnemonic == "stindsym") {
            int gpr = std::stoi(operands[0]);
            int reg = std::stoi(operands[1]);
            std::string symName = operands[2];

            int operandOffset = currentSection->locationCounter + InstructionEncoder::ST_IND_DISP_OP_OFFSET;
            

            currentSection->emitBytes(InstructionEncoder::st_register_indirect_disp(gpr, reg, 0));
            symbolUsageHandler(symName, operandOffset, Relocation::DISP);
        }
        else if (mnemonic == "csrrd") {
            int csr = std::stoi(operands[0]);
            int gpr = std::stoi(operands[1]);
            currentSection->emitBytes(InstructionEncoder::csrrd(csr, gpr));
        }
        else if (mnemonic == "csrwr") {
            int gpr = std::stoi(operands[0]);
            int csr = std::stoi(operands[1]);
            currentSection->emitBytes(InstructionEncoder::csrwr(gpr, csr));
        }
        else {
            throw std::runtime_error("Unknown instruction mnemonic: " + mnemonic);
        }

    }catch(const std::out_of_range& e){
        std::cerr << e.what() << std::endl;
        exit(1);
    }
}
void Assembler::processDirective(const std::string &directive,const std::vector<std::string> &args){
    // std::cout << directive;
    // for (const auto &arg : args) {
    //     std::cout << " " << arg;
    // }
    // std::cout << std::endl;

    if(directive == ".global"){
        for (const auto &symName : args) {
            auto it = symbolMap.find(symName);
            if (it != symbolMap.end()) {
                // Symbol already exists
                Symbol* sym = it->second;
                sym->binding = Symbol::GLOBAL;
            } else {
                // Symbol doesn't exist
                Symbol* sym = new Symbol(
                    symbolList.size(),
                    symName,
                    undefinedSection
                );
                sym->binding = Symbol::GLOBAL;
                symbolMap[symName] = sym;
                symbolList.push_back(sym);
            }
        }
    }else if(directive == ".extern"){
        for (const auto &symName : args) {
            auto it = symbolMap.find(symName);
            if (it != symbolMap.end()) {
                // Symbol already exists
                Symbol* sym = it->second;
                //Don't set extern symbols to global immediately.
                //They will be set to global only if they are undefined in the end.
                //sym->binding = Symbol::GLOBAL;
                sym->external = true;
            } else {
                // Symbol doesn't exist
                Symbol* sym = new Symbol(
                    symbolList.size(),
                    symName,
                    undefinedSection
                );
                //Don't set extern symbols to global immediately.
                //sym->binding = Symbol::GLOBAL;
                sym->external = true;
                symbolMap[symName] = sym;
                symbolList.push_back(sym);
            }
        }
    }else if(directive == ".section"){
        std::string sectionName = args[0];

        auto it = symbolMap.find(sectionName);
        if (it != symbolMap.end()) {
            // Section already exists (we use the section's symbol to check)
            Symbol* secSym = it->second;
            currentSection = secSym->section;
        } else {
            // Section doesn't exist
            Section* newSec = new Section(sectionList.size(), sectionName);
            sectionList.push_back(newSec);

            Symbol* secSym = new Symbol(symbolList.size(), sectionName, newSec);
            secSym->type = Symbol::SCTN;
            secSym->binding = Symbol::LOCAL;
            secSym->defined = true;
            symbolMap[sectionName] = secSym;
            symbolList.push_back(secSym);

            currentSection = newSec;
        }
    }else if(directive == ".word") {
        if(currentSection == nullptr){
            throw std::logic_error("No section was started before writing content");
        }
        for (size_t i = 0; i < args.size(); i += 2) {
            std::string kind = args[i]; //"lit" or "sym"
            std::string value = args[i + 1]; //either a value or identifier

            if (kind == "lit") {
                //literal
                int literal = std::stoi(value);
                std::vector<uint8_t> bytes = InstructionEncoder::word(literal);
                currentSection->emitBytes(bytes);
            }
            else if (kind == "sym") {
                //symbol
                std::string symName = value;
                int offset = currentSection->locationCounter;

                std::vector<uint8_t> bytes = InstructionEncoder::word(0);
                currentSection->emitBytes(bytes);

                symbolUsageHandler(symName, offset, Relocation::DIRECT);
                
            }
            else {
                throw std::runtime_error("Internal error: .word: argument must start with 'lit' or 'sym'");
            }
        }
    }else if(directive == ".skip"){
        if(currentSection == nullptr){
            throw std::logic_error("No section was started before writing content");
        }
        int numBytes = std::stoi(args[0]);
        for (int i = 0; i < numBytes; i++) {
            currentSection->emitByte(0);
        }
    }else if(directive == ".end"){
        //Nothing to do
    }else if(directive == ".ascii"){    
        if(currentSection == nullptr){
            throw std::logic_error("No section was started before writing content");
        }

        if(args.empty()){
            throw std::runtime_error("Internal error: .ascii directive requires a string argument");
        }

        std::string raw = args[0];
        std::vector<uint8_t> bytes;
        bytes.reserve(raw.size());

        for(size_t i = 0; i < raw.size(); ++i){
            unsigned char c = raw[i];

            if(c == '\\'){
                if(i + 1 >= raw.size())
                    throw std::runtime_error("Invalid escape sequence at end of string in .ascii");

                char esc = raw[++i];
                switch(esc){
                    case 'n':  bytes.push_back('\n'); break;
                    case 't':  bytes.push_back('\t'); break;
                    case 'r':  bytes.push_back('\r'); break;
                    case '0':  bytes.push_back('\0'); break;
                    case '\\': bytes.push_back('\\'); break;
                    case '\"': bytes.push_back('\"'); break;
                    case '\'': bytes.push_back('\''); break;
                    default:
                        //Just add the character itself if it isn't recognized
                        bytes.push_back(static_cast<uint8_t>(esc));
                        break;
                }
            } else {
                bytes.push_back(static_cast<uint8_t>(c));
            }
        }
        currentSection->emitBytes(bytes);
    }else{
        throw std::runtime_error("Error: Unknown directive " + directive);
    }




    
}
void Assembler::defineLabel(const std::string &label){
    if (currentSection == nullptr) {
        throw std::logic_error("Cannot define label outside of a section: " + label);
    }

    auto it = symbolMap.find(label);

    if (it != symbolMap.end()) {
        // Symbol exists
        Symbol* sym = it->second;

        if (sym->defined) {
            throw std::runtime_error("Multiple definitions of symbol: " + label);
        }else {
            sym->section = currentSection;
            sym->value   = currentSection->locationCounter;
            sym->defined = true;
        }
    }else {
        // Symbol doesn't exist
        Symbol* sym = new Symbol(
            symbolList.size(),
            label,
            currentSection
        );
        
        sym->value = currentSection->locationCounter;
        sym->defined = true;

        symbolMap[label] = sym;
        symbolList.push_back(sym);
    }
    
    // std::cout << label;
    // std::cout << std::endl;

}
void Assembler::writeOutput(const std::string &filename) {
    cleanup();
    ShelfWriter writer(sectionList, symbolList, absoluteSection, undefinedSection);
    writer.write(filename);
    //Convert the contents into a human-readable text and write it to a text file
    ShelfPrinter printer(filename);
    printer.print(filename + ".txt");
}

void Assembler::processEqu(const std::string& name, Expression* expression){
    auto it = symbolMap.find(name);

    if (it != symbolMap.end()) {
        // Symbol exists
        Symbol* sym = it->second;

        if (sym->defined) {
            throw std::runtime_error("Multiple definitions of symbol: " + name);
        }else {
            //Try to evaulate expression and resolve or add to unresolved if it can't be resolved
            sym->section = absoluteSection;
            tryResolveAbsolute(sym, expression, true);
        }
    }else {
        // Symbol doesn't exist
        Symbol* sym = new Symbol(
            symbolList.size(),
            name,
            absoluteSection
        );

        //Try to evaulate expression and resolve or add to unresolved if it can't be resolved
        tryResolveAbsolute(sym, expression, true);

        symbolMap[name] = sym;
        symbolList.push_back(sym);
    }
}
void Assembler::symbolUsageEquHandler(const std::string& name){
    auto it = symbolMap.find(name);

    if (it == symbolMap.end()) {
        // Symbol doesn’t exist yet so add it
        Symbol* sym = new Symbol(symbolList.size(), name, undefinedSection);
        sym->binding = Symbol::LOCAL;
        sym->defined = false;
        sym->external = false;

        symbolMap[name] = sym;
        symbolList.push_back(sym);
    }
}
bool Assembler::tryResolveAbsolute(Symbol* symbol, Expression* expression, bool addToUnresolved) {
    try {
        int value = expression->evaluate(symbolMap);

        //Check if the expression can be resolved at assembly time (if non-absolute symbols cancel out)
        auto sectionContributions = expression->getSectionContributions(symbolMap, absoluteSection);
        for (const auto& [sec, count] : sectionContributions) {
            if (sec != absoluteSection && count != 0) {
                throw std::runtime_error("EQU expression is not absolute for symbol: " + symbol->name);
            }
        }

        symbol->value = value;
        symbol->defined = true;
        symbol->section = absoluteSection;
        return true;
    }
    catch (const SymbolUndefinedExpressionException& ex) {
        if(addToUnresolved){
            unresolvedEqus.push_back({symbol, expression});
        }
        return false;
    }
    return false;
}
void Assembler::resolveAbsolutes() {
    bool progress = true;

    //We keep iterating until we've emptied the list
    while (!unresolvedEqus.empty() && progress) {
        progress = false;
        std::vector<UnresolvedEqu> remaining;

        for (auto& eq : unresolvedEqus) {
            Symbol* symbol = eq.symbol;
            Expression* expression = eq.expression;
            if (tryResolveAbsolute(symbol, expression, false)) {
                progress = true;
                delete expression;
            } else {
                remaining.push_back(eq);
            }
        }
        //Set unresolvedEqus to what's remaining
        unresolvedEqus.swap(remaining);
    }
    if (!unresolvedEqus.empty()) {
        std::string errMsg = "Failed to resolve absolute symbols: ";
        for (const auto& eq : unresolvedEqus) {
            errMsg += eq.symbol->name + " ";
        }
        throw std::runtime_error(errMsg);
    }
}

Assembler::~Assembler() {
    for (Symbol* sym : symbolList) {
        delete sym;
    }
    symbolList.clear();

    for (Section* sec : sectionList) {
        delete sec;
    }
    sectionList.clear();

    for (auto& ue : unresolvedEqus) {
        delete ue.expression;
        ue.expression = nullptr;
    }
    unresolvedEqus.clear();
}
