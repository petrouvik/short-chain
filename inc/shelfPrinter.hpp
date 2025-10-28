#pragma once
#include <string>
#include <ostream>
#include "shelfReader.hpp"

class ShelfPrinter {
public:
    explicit ShelfPrinter(const std::string& filename);
    void print(const std::string& outputFile);

private:
    ShelfReader reader;

    void printSectionHeaderTable(std::ostream& os);
    void printSectionContents(std::ostream& os);
    void printBytes(std::ostream& os, const std::vector<uint8_t>& data);
    void printSymbols(std::ostream& os);
    void printRelocations(std::ostream& os);
};