#pragma once
#include <map>
#include <string>
#include "expression.hpp"

struct Symbol;
struct Section;

class SymbolExpr : public Expression {
    std::string name;

public:
    explicit SymbolExpr(std::string n) : name(std::move(n)) {}

    int evaluate(const std::map<std::string, Symbol*>& symbols) const override;

    std::map<Section*, int>
    getSectionContributions(const std::map<std::string, Symbol*>& symbols, const Section* absoluteSection) const override;
};