#include "symbolExpr.hpp"
#include <stdexcept>
#include "symbol.hpp"
#include "symbolUndefinedExpressionException.hpp"
int SymbolExpr::evaluate(const std::map<std::string, Symbol*>& symbols) const {
    auto it = symbols.find(name);
    if (it == symbols.end()) {
        throw std::runtime_error("Internal error: couldn't find symbol while evaluating." + name);
    }
    auto symbol = it->second;

    if(!symbol->defined){
        throw SymbolUndefinedExpressionException();
    }
    
    return symbol->value;
    
}

std::map<Section*, int>
SymbolExpr::getSectionContributions(const std::map<std::string, Symbol*>& symbols, const Section* absoluteSection) const{
    auto it = symbols.find(name);
    if (it == symbols.end()) {
        throw std::runtime_error("Internal error: couldn't find symbol while evaluating." + name);
    }
    auto symbol = it->second;
    std::map<Section*, int> result;

    if(!symbol->defined){
        throw SymbolUndefinedExpressionException();
    }
    if(symbol->section != absoluteSection) {
        result[symbol->section] = 1;
    }
    
    return result;
}