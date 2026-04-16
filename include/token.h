#pragma once

#include <string>
#include <iosfwd>


enum class TokenType {
    // Misc
    EndOfFile,
    INVALID,

    // identifiers and literals
    IDENT,
    INT_LITERAL,
    STR_LITERAL,

    // keywords
    KW_FN,
    KW_LET,
    KW_IF,
    KW_ELSE,
    KW_WHILE,
    KW_RETURN,
    KW_TRUE,
    KW_FALSE,
    KW_INT,
    KW_BOOL,
    KW_STR,
    KW_VOID,

    // operators
    PLUS,
    MINUS,
    STAR,
    SLASH,
    PERCENT,

    ASSIGN,
    EQ_EQ,
    BANG_EQ,

    LT,
    LE,
    GT,
    GE,

    AND_AND,
    OR_OR,
    BANG,

    LPAREN,
    RPAREN,

    LBRACE,
    RBRACE,

    COMMA,
    SEMIC,
    COLON,
};


struct SourceLocation {
    size_t line = 1;
    size_t column = 1;
};


struct Token {
    TokenType type;
    std::string lexeme;
    // std::string file_name;
    SourceLocation location;
};


std::string to_string(TokenType type);
std::ostream& operator<<(std::ostream& os, const Token& token);
