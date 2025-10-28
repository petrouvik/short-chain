#include "binaryExpr.hpp"
#include <map>
#include <memory>
#include <stdexcept>
#include "expression.hpp"
#include "section.hpp"

int BinaryExpr::evaluate(const std::map<std::string, Symbol*>& symbols) const {
    int lv = left->evaluate(symbols);
    int rv = right->evaluate(symbols);

    switch(op) {
        case '+': return lv + rv;
        case '-': return lv - rv;
        default:
            throw std::runtime_error("Internal error: Unsupported binary operator");
    }
}

std::map<Section*, int>
BinaryExpr::getSectionContributions(const std::map<std::string, Symbol*>& symbols,
                                    const Section* absoluteSection) const
{
    auto leftMap = left->getSectionContributions(symbols, absoluteSection);
    auto rightMap = right->getSectionContributions(symbols, absoluteSection);

    for (auto& [sec, count] : rightMap) {
        if (op == '+') {
            leftMap[sec] += count;
        } else if (op == '-') {
            leftMap[sec] -= count;
        } else {
            throw std::runtime_error("Internal error: Unsupported binary operator.");
        }
    }

    return leftMap;
}
