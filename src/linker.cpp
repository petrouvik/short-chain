#include "linker.hpp"
#include <stdexcept>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include "shelfReader.hpp"
#include "shelf.hpp"
#include "section.hpp"
#include "symbol.hpp"
#include "relocation.hpp"
#include "shelfWriter.hpp"
#include "shelfPrinter.hpp"
void Linker::readFile(const std::string& filename){
    ShelfReader reader(filename);
    //Add sections
    auto& sht = reader.getSectionHeaders();
    for(auto& sh: sht){
        sectionHeaders.push_back(sh);
    }
    //Add symbols
    auto& syms = reader.getSymbols();
    for(auto& sym: syms){
        //Update section indexes
        if(sym.sectionIndex != SHELF_SHN_ABS) sym.sectionIndex += sectionIndexOffset;
        symbols.push_back(sym);
    }
    //Add contents
    for(size_t i = 0; i < sht.size(); i++){
        if(sectionHeaders[sectionIndexOffset + i].type == SHELF_PROGBITS){
            sectionContents[sectionIndexOffset + i] = reader.getSectionContents(i);
        }
    }
    //Add relocation entries
    for(size_t i = 0; i < sht.size(); i++){
        auto& sh = sectionHeaders[sectionIndexOffset + i];
        if(sh.type == SHELF_RELOC){
            relocations[sh.info + sectionIndexOffset] = reader.getRelocations(sh.info);
            //Update section index
            sh.info += sectionIndexOffset;
            //Update symbol indexes
            for(auto& r: relocations[sh.info]){
                r.symIndex += symbolIndexOffset;
            }
        }
    }
    //Update offsets
    sectionIndexOffset += sht.size();
    symbolIndexOffset += syms.size();
}
void Linker::addSectionStartingAddress(const std::string& name, int address) {
    //Check if this section name was already assigned an address
    if (sectionStartingAddresses.find(name) != sectionStartingAddresses.end()) {
        throw std::runtime_error("Error: Starting address for section '" + name + "' already specified.");
    }
    sectionStartingAddresses[name] = address;
}

void Linker::link(const std::string& outputFilename){
    resolveUndefinedSymbols();
    computeMergedSectionSizes();
    computeSectionAddresses();
    assignFinalSectionAddresses();
    applyRelocations();

    std::ofstream out(outputFilename, std::ios::binary);
    if(!out) throw std::runtime_error("Cannot open output file");

    for (size_t i = 0; i < sectionHeaders.size(); ++i) {
        const auto& sh = sectionHeaders[i];
        if(sh.type != SHELF_PROGBITS) continue;

        const auto& content = sectionContents[i];
        uint32_t baseAddr = sh.address;

        for (size_t j = 0; j < content.size(); ++j) {
            uint32_t addr = baseAddr + j;
            uint8_t value = content[j];

            out.write(reinterpret_cast<const char*>(&addr), sizeof(addr));
            out.write(reinterpret_cast<const char*>(&value), sizeof(value));
        }
    }

    out.close();
    printLinkedFile(outputFilename);
}

void Linker::resolveUndefinedSymbols() {
    std::unordered_map<std::string, size_t> definedGlobals;
    //First pass
    for (size_t i = 0; i < symbols.size(); ++i) {
        auto& sym = symbols[i];

        bool isDefined = sym.sectionIndex == SHELF_SHN_ABS ? true : sectionHeaders[sym.sectionIndex].type != SHELF_NULL;
        bool isGlobal = sym.bind == STB_GLOBAL;
        bool isLocal  = sym.bind == STB_LOCAL;
        const std::string& name = sym.name;

        if (!isDefined) {
            if (isLocal && !name.empty()) {
                std::ostringstream oss;
                oss << "Error: Local symbol '" << name << "' is undefined.";
                throw std::runtime_error(oss.str());
            }
        }else{
            if (isGlobal) {
                //Check for multiple definitions
                auto [it, inserted] = definedGlobals.emplace(name, i);
                if (!inserted) {
                    std::ostringstream oss;
                    oss << "Error: Multiple definitions of global symbol '" << name << "'.";
                    throw std::runtime_error(oss.str());
                }
            }
        }
    }

    //Second pass
    for (size_t i = 0; i < symbols.size(); ++i) {
        auto& sym = symbols[i];

        bool isDefined = sym.sectionIndex == SHELF_SHN_ABS ? true : sectionHeaders[sym.sectionIndex].type != SHELF_NULL;
        bool isGlobal = sym.bind == STB_GLOBAL;
        const std::string& name = sym.name;

        if (!isDefined && isGlobal && !name.empty()) {
            auto it = definedGlobals.find(name);
            if (it == definedGlobals.end()) {
                std::ostringstream oss;
                oss << "Error: Undefined global symbol '" << name << "' cannot be resolved.";
                throw std::runtime_error(oss.str());
            }
            //Copy info from defined symbol into undefined
            const auto& defSym = symbols[it->second];
            sym = defSym;
        }
    }
}
void Linker::computeMergedSectionSizes() {
    for (auto& sh : sectionHeaders) {
        if (sh.type == SHELF_PROGBITS) {
            if (mergedSectionSizes.find(sh.name) == mergedSectionSizes.end()) {
                sh.address = 0;
                mergedSectionSizes[sh.name] = sh.size;
            } else {
                sh.address =  mergedSectionSizes[sh.name];
                mergedSectionSizes[sh.name] += sh.size;
            }
        }
    }
}
void Linker::computeSectionAddresses() {
    struct Range {
        int start;
        int end;
    };
    std::vector<Range> usedRanges;

    //Place fixed sections first
    for (const auto& [name, addr] : sectionStartingAddresses) {
        auto it = mergedSectionSizes.find(name);
        if (it == mergedSectionSizes.end()) {
            throw std::runtime_error("Error: User specified start address for unknown section:" + name);
        }

        int size = it->second;
        int start = addr;
        int end = addr + size;

        //Check for overlap
        for (const auto& r : usedRanges) {
            if (!(end <= r.start || start >= r.end)) {
                throw std::runtime_error("Error: Overlapping section address range for:" + name);
            }
        }

        usedRanges.push_back({start, end});
        mergedSectionAddresses[name] = start;
    }

    //Place remaining sections
    int currentAddr = 0;
    for (const auto& [name, size] : mergedSectionSizes) {
        
        //Skip sections that are already fixed
        if (mergedSectionAddresses.find(name) != mergedSectionAddresses.end()) continue;

        int start = currentAddr;
        int end = start + size;

        //Shift if overlapping with a fixed section
        bool adjusted;
        do {
            adjusted = false;
            for (const auto& r : usedRanges) {
                if (!(end <= r.start || start >= r.end)) {
                    //If we have an overlap, then skip the fixed section
                    start = r.end;
                    end = start + size;
                    adjusted = true;
                }
            }
        } while (adjusted);

        mergedSectionAddresses[name] = start;
        usedRanges.push_back({start, end});

        currentAddr = end;
    }
}
void Linker::assignFinalSectionAddresses() {
    for (auto& sh : sectionHeaders) {
        if (sh.type == SHELF_PROGBITS) {
            auto it = mergedSectionAddresses.find(sh.name);
            if (it != mergedSectionAddresses.end()) {
                sh.address += it->second;
            } else {
                throw std::runtime_error("Error: no starting address found for section: " + sh.name);
            }
        }
    }
}
void Linker::applyRelocations() {
    for (auto& [sectionIdx, relList] : relocations) {
        //Get the section contents this relocation applies to
        auto& secContent = sectionContents[sectionIdx];

        for (auto& rel : relList) {
            
            size_t offset = rel.offset;
            const auto& sym = symbols[rel.symIndex];
            uint32_t symbolValue = 0;
            if (sym.sectionIndex == SHELF_SHN_ABS) {
                symbolValue = sym.value;
            } else {
                symbolValue = sym.value + sectionHeaders[sym.sectionIndex].address;
            }
            int32_t addend = rel.addend;

            uint32_t finalValue = 0;

            //Compute final value based on relocation type
            switch (rel.type) {
                case R_SHELF_DIRECT:
                    finalValue = symbolValue + addend;
                    break;

                case R_SHELF_PC_REL:
                    finalValue = symbolValue - (sectionHeaders[sectionIdx].address + offset) + addend;
                    break;

                default:
                    throw std::runtime_error("Unknown relocation type");
            }

            //Patch the bytes in little endian
            if (offset + 4 > secContent.size()) {
                throw std::runtime_error("Relocation offset out of section bounds");
            }
            secContent[offset + 0] = (finalValue >> 0) & 0xFF;
            secContent[offset + 1] = (finalValue >> 8) & 0xFF;
            secContent[offset + 2] = (finalValue >> 16) & 0xFF;
            secContent[offset + 3] = (finalValue >> 24) & 0xFF;

            
        }
    }
}
void Linker::debugPrintState() const {
    std::cout << "=== Linker State ===\n";

    std::cout << "\nSection Headers:\n";
    for (size_t i = 0; i < sectionHeaders.size(); ++i) {
        const auto& sh = sectionHeaders[i];
        std::cout << "  [" << i << "] name: " << sh.name
                  << ", type: " << sh.type
                  << ", size: " << sh.size
                  << ", address: " << sh.address
                  << ", info: " << sh.info << "\n";
    }

    std::cout << "\nSymbols:\n";
    for (size_t i = 0; i < symbols.size(); ++i) {
        const auto& sym = symbols[i];
        std::cout << "  [" << i << "] name: " << sym.name
                  << ", value: " << sym.value
                  << ", size: " << sym.size
                  << ", sectionIndex: " << sym.sectionIndex
                  << ", bind: " << sym.bind
                  << ", type: " << sym.type << "\n";
    }

    std::cout << "\nRelocations:\n";
    for (const auto& [secIdx, relList] : relocations) {
        std::cout << "  Section " << secIdx << ":\n";
        for (const auto& r : relList) {
            std::cout << "    offset: " << r.offset
                      << ", symIndex: " << r.symIndex
                      << ", type: " << r.type
                      << ", addend: " << r.addend << "\n";
        }
    }

    std::cout << "\nUser-specified Section Starting Addresses:\n";
    for (const auto& [name, addr] : sectionStartingAddresses) {
        std::cout << "  " << name << " -> " << addr << "\n";
    }

    std::cout << "\nMerged Section Sizes:\n";
    for (const auto& [name, size] : mergedSectionSizes) {
        std::cout << "  " << name << " -> " << size << "\n";
    }

    std::cout << "\nMerged Section Addresses:\n";
    for (const auto& [name, addr] : mergedSectionAddresses) {
        std::cout << "  " << name << " -> " << addr << "\n";
    }

    std::cout << "===================\n";
}
void Linker::printLinkedFile(const std::string& filename) const {
    std::ifstream in(filename, std::ios::binary);
    if (!in) {
        std::cerr << "Cannot open file: " << filename << "\n";
        return;
    }
    std::string outFilename = filename + ".txt";
    std::ofstream out(outFilename);
    if (!out) {
        std::cerr << "Cannot open output file: " << outFilename << "\n";
        return;
    }

    std::map<uint32_t, uint8_t> memory;

    while (true) {
        uint32_t address;
        uint8_t value;

        //Read 4B address
        in.read(reinterpret_cast<char*>(&address), sizeof(address));
        if (in.eof()) break;

        //Read 1B value
        in.read(reinterpret_cast<char*>(&value), sizeof(value));
        if (in.eof()) break;

        memory[address] = value;
    }

    uint32_t countInLine = 0;
    uint32_t prevAddress = 0;
    bool first = true;

    for (auto it = memory.begin(); it != memory.end(); ++it) {
        uint32_t addr = it->first;

        if (!first) {
            if (addr != prevAddress + 1) {
                //There is a gap
                out << "\n...\n";
                countInLine = 0;
            } else if (countInLine >= 8) {
                out << "\n";
                countInLine = 0;
            }
        }

        //Start a new line if this is the first byte of a line
        if (countInLine == 0) {
            out << std::hex << std::setw(8) << std::setfill('0') << addr << ": ";
        }

        out << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(it->second) << " ";
        prevAddress = addr;
        ++countInLine;
        first = false;
    }

    if (!first) out << "\n";
}

void Linker::linkRelocatable(const std::string& filename){
    generateWriterSections();
    checkDuplicateGlobals();
    generateWriterSymbols();
    generateWriterRelocations();

    ShelfWriter writer(writerSections, writerSymbols, absoluteSection, undefinedSection);
    writer.write(filename);
    ShelfPrinter printer(filename);
    printer.print(filename + ".txt");


}

void Linker::generateWriterSections() {
    undefinedSection = new Section(0, "");
    writerSections.push_back(undefinedSection);

    absoluteSection = new Section(SHELF_SHN_ABS, "");
    
    for (size_t i = 0; i < sectionHeaders.size(); ++i) {
        const auto &sh = sectionHeaders[i];

        if (sh.type == SHELF_NULL) {
            sectionIndexOffsetMap[i] = 0;
            indexSectionMap[i] = undefinedSection;
            continue;
        }

        if (sh.type != SHELF_PROGBITS) {
            continue;
        }

        const std::vector<uint8_t> &contents = sectionContents[i];

        auto it = sectionNameOffsetMap.find(sh.name);
        if (it != sectionNameOffsetMap.end()) {
            SectionCurrentOffset &sco = it->second;
            Section *targetSection = sco.section;

            sectionIndexOffsetMap[i] = sco.currentOffset;

            if (!contents.empty()) {
                targetSection->emitBytes(contents);
            }
            sco.currentOffset += static_cast<int>(contents.size());
            
            indexSectionMap[i] = targetSection;
            
        } else {
            Section *newSec = new Section(static_cast<int>(writerSections.size()), sh.name);

            if (!contents.empty()) {
                newSec->emitBytes(contents);
            }

            writerSections.push_back(newSec);

            sectionIndexOffsetMap[i] = 0;

            SectionCurrentOffset sco;
            sco.section = newSec;
            sco.currentOffset = static_cast<int>(newSec->contents.size());
            sectionNameOffsetMap[sh.name] = sco;
            
            indexSectionMap[i] = newSec;
        }
    }
}
void Linker::checkDuplicateGlobals(){
    
    std::unordered_map<std::string, const ShelfReader::ResolvedSymbol*> seenGlobals;

    for (const auto& sym : symbols) {
        if (sym.bind != STB_GLOBAL) continue;
        if (sym.sectionIndex != SHELF_SHN_ABS && sectionHeaders[sym.sectionIndex].type == SHELF_NULL) continue;

        auto it = seenGlobals.find(sym.name);
        if (it != seenGlobals.end()) {
            throw std::runtime_error("Duplicate global symbol definition: " + sym.name);
        }

        seenGlobals[sym.name] = &sym;
    }

}
void Linker::generateWriterSymbols() {
    Symbol* emptySymbol = new Symbol(static_cast<int>(writerSymbols.size()), "", undefinedSection);
    writerSymbols.push_back(emptySymbol);
    for (size_t i = 0; i < symbols.size(); ++i) {
        const auto& inSym = symbols[i];
        if(inSym.name == ""){
            //Map empty symbols to the first symbol
            indexSymbolMap[i] = emptySymbol;
            continue;
        }
        //Handle section symbols specially
        if (inSym.type == ST_SECTION) {
            auto it = symbolSCTNName.find(inSym.name);

            if (it != symbolSCTNName.end()) {
                Symbol* existing = it->second;
                indexSymbolMap[i] = existing;
                symbolSCTNOffset[i] = sectionIndexOffsetMap[inSym.sectionIndex];

            }else{

                Section* sec = indexSectionMap.at(inSym.sectionIndex);

                Symbol* newSym = new Symbol(static_cast<int>(writerSymbols.size()), inSym.name, sec);
                newSym->value = inSym.value;
                newSym->size = inSym.size;
                newSym->type = Symbol::SCTN;
                newSym->binding = inSym.bind == STB_GLOBAL ? Symbol::GLOBAL : Symbol::LOCAL;

                writerSymbols.push_back(newSym);
                indexSymbolMap[i] = newSym;
                symbolSCTNName[inSym.name] = newSym;
                symbolSCTNOffset[i] = sectionIndexOffsetMap[inSym.sectionIndex];
            }

        }else{
            Section* sec;
            if(inSym.sectionIndex == SHELF_SHN_ABS){
                sec = absoluteSection;
            }
            else{
                sec = indexSectionMap.at(inSym.sectionIndex);
            }

            Symbol* newSym = new Symbol(static_cast<int>(writerSymbols.size()), inSym.name, sec);
            newSym->value = inSym.value + sectionIndexOffsetMap[inSym.sectionIndex];
            newSym->size = inSym.size;
            newSym->type = inSym.type == ST_SECTION ? Symbol::SCTN : Symbol::NOTYP;
            newSym->binding = inSym.bind == STB_GLOBAL ? Symbol::GLOBAL : Symbol::LOCAL;

            writerSymbols.push_back(newSym);
            indexSymbolMap[i] = newSym;
        }
    }
}
void Linker::generateWriterRelocations() {
    for (auto& [sectionIndex, relocVec] : relocations) {
        auto secIt = indexSectionMap.find(sectionIndex);
        if (secIt == indexSectionMap.end()) continue;

        Section* section = secIt->second;
        int sectionOffsetAdj = 0;

        auto offIt = sectionIndexOffsetMap.find(sectionIndex);
        if (offIt != sectionIndexOffsetMap.end())
            sectionOffsetAdj = offIt->second;
        
        for (auto& rr : relocVec) {
            int newOffset = rr.offset + sectionOffsetAdj;
            int newAddend = rr.addend;

            //If the symbol refers to a section, adjust addend as well
            if (symbols[rr.symIndex].type == ST_SECTION) {
                size_t targetSectionIndex = rr.symIndex;

                auto sctnOffIt = symbolSCTNOffset.find(targetSectionIndex);
                if (sctnOffIt != symbolSCTNOffset.end()) {
                    newAddend += sctnOffIt->second;
                }
            }

            //Construct the relocation for writer
            Relocation rel(
                newOffset,
                rr.type == R_SHELF_DIRECT ? Relocation::DIRECT : Relocation::PC_REL,
                newAddend,
                indexSymbolMap[rr.symIndex]
                
            );

            section->relocations.push_back(std::move(rel));
        }
    }
}
int main(int argc, char** argv){
    Linker linker;
    std::string outputFile;
    bool hexMode = false;
    bool relocatableMode = false;

    std::vector<std::string> objectFiles;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-hex") {
            if (relocatableMode) {
                std::cerr << "Cannot specify both -hex and -relocatable\n";
                return 1;
            }
            hexMode = true;
        } else if (arg == "-relocatable") {
            if (hexMode) {
                std::cerr << "Cannot specify both -hex and -relocatable\n";
                return 1;
            }
            relocatableMode = true;
        } else if (arg == "-o") {
            if (i + 1 >= argc) {
                std::cerr << "-o requires an argument\n";
                return 1;
            }
            outputFile = argv[++i];
        } else if (arg.rfind("-place=", 0) == 0) {
            std::string opt = arg.substr(7);
            auto atPos = opt.find('@');
            if (atPos == std::string::npos) {
                std::cerr << "Invalid -place format, expected section@address\n";
                return 1;
            }
            std::string sectionName = opt.substr(0, atPos);
            int address = static_cast<uint32_t>(std::stoul(opt.substr(atPos + 1), nullptr, 0));
            
            try {
                linker.addSectionStartingAddress(sectionName, address);
            } catch (const std::runtime_error& e) {
                std::cerr << "Error: " << e.what() << "\n";
                return 1;
            }
        } else {
            objectFiles.push_back(arg);
        }
    }

    if (outputFile.empty()) {
        std::cerr << "Output file not specified\n";
        return 1;
    }

    if (!hexMode && !relocatableMode) {
        std::cerr << "Must specify either -hex or -relocatable\n";
        return 1;
    }

    for (const auto& obj : objectFiles) {
        try {
            linker.readFile(obj);
        } catch (const std::runtime_error& e) {
            std::cerr << "Error reading file " << obj << ": " << e.what() << "\n";
            return 1;
        }
    }

    if (hexMode) {
        try {
            linker.link(outputFile);
        } catch (const std::runtime_error& e) {
            std::cerr << "Linking error: " << e.what() << "\n";
            return 1;
        }
    }
    if (relocatableMode){
        try {
            linker.linkRelocatable(outputFile);
        } catch (const std::runtime_error& e) {
            std::cerr << "Linking error: " << e.what() << "\n";
            return 1;
        }
    }

    return 0;
}