#include "numberExpr.hpp"

int NumberExpr::evaluate(const std::map<std::string, Symbol*>& symbols) const {
    return value;
}

std::map<Section*, int>
NumberExpr::getSectionContributions(const std::map<std::string, Symbol*>& symbols, const Section* absoluteSection) const{
    return {};
}