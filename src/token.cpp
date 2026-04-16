#include "token.h"

#include <iostream>
#include <iomanip>


std::string to_string(TokenType type) {
    switch (type) {
        using enum TokenType;
        case EndOfFile:     return "EndOfFile";
        case INVALID:       return "INVALID";
        case IDENT:         return "IDENT";
        case INT_LITERAL:   return "INT_LITERAL";
        case STR_LITERAL:   return "STR_LITERAL";
        case KW_FN:         return "KW_FN";
        case KW_LET:        return "KW_LET";
        case KW_IF:         return "KW_IF";
        case KW_ELSE:       return "KW_ELSE";
        case KW_WHILE:      return "KW_WHILE";
        case KW_RETURN:     return "KW_RETURN";
        case KW_TRUE:       return "KW_TRUE";
        case KW_FALSE:      return "KW_FALSE";
        case KW_INT:        return "KW_INT";
        case KW_BOOL:       return "KW_BOOL";
        case KW_STR:        return "KW_STR";
        case KW_VOID:       return "KW_VOID";
        case PLUS:          return "PLUS";
        case MINUS:         return "MINUS";
        case STAR:          return "STAR";
        case SLASH:         return "SLASH";
        case PERCENT:       return "PERCENT";
        case ASSIGN:        return "ASSIGN";
        case EQ_EQ:         return "EQ_EQ";
        case BANG_EQ:       return "BANG_EQ";
        case LT:            return "LT";
        case LE:            return "LE";
        case GT:            return "GT";
        case GE:            return "GE";
        case AND_AND:       return "AND_AND";
        case OR_OR:         return "OR_OR";
        case BANG:          return "BANG";
        case LPAREN:        return "LPAREN";
        case RPAREN:        return "RPAREN";
        case LBRACE:        return "LBRACE";
        case RBRACE:        return "RBRACE";
        case COMMA:         return "COMMA";
        case SEMIC:         return "SEMIC";
        case COLON:         return "COLON";
    }
}


std::ostream& operator<<(std::ostream& os, const Token& token) {
    os << to_string(token.type) << "(" << std::quoted(token.lexeme) << ") at " 
        << token.location.line << ":" << token.location.column;
    return os;
}