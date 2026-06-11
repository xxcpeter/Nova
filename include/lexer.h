#pragma once

#include "token.h"
#include "source_loader.h"
#include <string>
#include <vector>
#include <cstddef>
#include <stdexcept>
#include <iosfwd>
#include <format>
#include <string_view>
#include <unordered_map>

class Lexer {
public:
    Lexer(const LoadedSource& source) : source_(source.content), 
        source_filename_(source.line_mapping.empty() ? "" : source.line_mapping.front().file), line_mapping_(source.line_mapping) {}
    
    Token next_token();
    std::vector<Token> tokenize();
    
private:
    std::string_view source_;
    std::string_view source_filename_;
    std::vector<SourceLineMapping> line_mapping_;
    size_t pos_ = 0;
    size_t line_ = 1;
    size_t column_ = 1;
    
    bool is_at_end() const;
    char peek() const;
    char peek_next() const;
    char advance();
    SourceLocation current_location() const;
    SourceLocation location_for_current_position() const;
    
    void skip_whitespace_and_comments();
    
    Token lex_ident_or_keyword();
    Token lex_number();
    Token lex_string();
    Token lex_operator_or_punc();

    using enum TokenType;
    static inline const std::unordered_map<std::string_view, TokenType> keywords = {
        {"fn",      KW_FN},
        {"let",     KW_LET},
        {"if",      KW_IF},
        {"else",    KW_ELSE},
        {"while",   KW_WHILE},
        {"return",  KW_RETURN},
        {"true",    KW_TRUE},
        {"false",   KW_FALSE},
        {"int",     KW_INT},
        {"bool",    KW_BOOL},
        {"str",     KW_STR},
        {"void",    KW_VOID},
        {"struct",  KW_STRUCT},
        {"vec",     KW_VEC},
        {"enum",    KW_ENUM},
    };

    static inline const std::unordered_map<std::string_view, TokenType> oper_puncs = {
        {"+",       PLUS},
        {"-",       MINUS},
        {"*",       STAR},
        {"/",       SLASH},
        {"%",       PERCENT},
        {"=",       ASSIGN},
        {"==",      EQ_EQ},
        {"!=",      BANG_EQ},
        {"<",       LT},
        {"<=",      LE},
        {">",       GT},
        {">=",      GE},
        {"&&",      AND_AND},
        {"||",      OR_OR},
        {"!",       BANG},
        {"(",       LPAREN},
        {")",       RPAREN},
        {"{",       LBRACE},
        {"}",       RBRACE},
        {",",       COMMA},
        {";",       SEMIC},
        {":",       COLON},
        {".",       DOT},
    };
};


class LexError : public std::runtime_error {
public:
    LexError(const std::string& message, const SourceLocation& location) : 
        std::runtime_error(std::format("{}:{}:{}: LexError: {}", 
            location.file, location.line, location.column, message)) {}
};