#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "relocation.hpp"

struct Section{
    int index;
    std::string name;
    int locationCounter;
    std::vector<uint8_t> contents;
    std::vector<Relocation> relocations;
    Section(int idx, std::string nm) : index(idx), name(nm), locationCounter(0){}
    void emitByte(uint8_t byte) {
        contents.push_back(byte);
        locationCounter++;
    }
    void emitBytes(const std::vector<uint8_t>& bytes) {
        contents.insert(contents.end(), bytes.begin(), bytes.end());
        locationCounter += bytes.size();
    }
};