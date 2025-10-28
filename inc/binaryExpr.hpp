#pragma once
#include <map>
#include <string>
#include "expression.hpp"

class BinaryExpr : public Expression {
    char op;
    Expression* left;
    Expression* right;

public:
    BinaryExpr(Expression* l, char o, Expression* r)
        : op(o), left(std::move(l)), right(std::move(r)) {}

    int evaluate(const std::map<std::string, Symbol*>& symbols) const override;

    std::map<Section*, int>
    getSectionContributions(const std::map<std::string, Symbol*>& symbols, const Section* absoluteSection) const override;

    ~BinaryExpr() override {
        delete left;
        delete right;
    }
};
