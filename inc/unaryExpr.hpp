#pragma once
#include <map>
#include <string>
#include "expression.hpp"

class UnaryExpr : public Expression {
    char op;
    Expression* operand;

public:
    UnaryExpr(char o, Expression* expr)
        : op(o), operand(expr) {}

    int evaluate(const std::map<std::string, Symbol*>& symbols) const override;

    std::map<Section*, int>
    getSectionContributions(const std::map<std::string, Symbol*>& symbols, const Section* absoluteSection) const override;

    ~UnaryExpr() override {
        delete operand;
    }
};