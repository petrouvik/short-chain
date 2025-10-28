#include "unaryExpr.hpp"
#include <map>
#include <memory>
#include <stdexcept>
#include "expression.hpp"
#include "section.hpp"

int UnaryExpr::evaluate(const std::map<std::string, Symbol*>& symbols) const {
    int val = operand->evaluate(symbols);
    switch(op) {
        case '-': return -val;
        default:
            throw std::runtime_error("Internal error: Unsupported unary operator.");
    }
}

std::map<Section*, int>
UnaryExpr::getSectionContributions(const std::map<std::string, Symbol*>& symbols,
                                   const Section* absoluteSection) const
{
    auto operandMap = operand->getSectionContributions(symbols, absoluteSection);
    
    for(auto& [sec, count] : operandMap) {
        count = -count;
    }
    return operandMap;
}