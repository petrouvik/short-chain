#pragma once
#include <map>
#include <string>

struct Symbol;
struct Section;

class Expression {
public:
    virtual ~Expression() = default;

    virtual int evaluate(const std::map<std::string, Symbol*>& symbols) const = 0;

    virtual std::map<Section*, int>
    getSectionContributions(const std::map<std::string, Symbol*>& symbols, const Section* absoluteSection) const = 0;
};

