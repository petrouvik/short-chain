#pragma once
#include <map>
#include <string>
#include "expression.hpp"

class NumberExpr : public Expression {
    int value;

public:
    explicit NumberExpr(int v) : value(v) {}

    int evaluate(const std::map<std::string, Symbol*>& symbols) const override;

    std::map<Section*, int>
    getSectionContributions(const std::map<std::string, Symbol*>& symbols, const Section* absoluteSection) const override;
};

