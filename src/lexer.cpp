#include "lexer.h"

#include <iostream>

#define CR_LF '\n'
using namespace std::string_literals;


bool Lexer::is_at_end() const {
    return pos_ >= source_.size();
}


char Lexer::peek() const {
    if (pos_ >= source_.size()) return '\0';
    return source_[pos_];
}


char Lexer::peek_next() const {
    if (pos_ + 1 >= source_.size()) return '\0';
    return source_[pos_ + 1];
}


char Lexer::advance() {
    if (source_[pos_] == CR_LF) {
        line_++;
        column_ = 1;
    } else {
        column_++;
    }
    return source_[pos_++];
}


SourceLocation Lexer::current_location() const {
    return SourceLocation{ line_, column_ };
}


void Lexer::skip_whitespace_and_comments() {
    while (true) {
        if (is_at_end()) return;
        if (std::isspace(peek())) {
            advance();
            continue;
        }
        if (peek() == '/' && peek_next() == '/') {
                advance(); advance();
                while (!is_at_end() && peek() != '\n') {
                    advance();
                }
                continue;
            } 
        if (peek() == '/' && peek_next() == '*') {
            SourceLocation comment_start = current_location();
            advance(); advance();
            while (true) {
                if (is_at_end())
                    throw LexError("Unterminated block comment", comment_start);
                if (peek() == '*' && peek_next() == '/') {
                    advance(); advance();
                    break;
                }
                advance();
            }
            continue;
        }
        break;
    }
}


Token Lexer::lex_number() {
    SourceLocation location = current_location();
    size_t start_pos = pos_;
    while (std::isdigit(peek())) {
        advance();
    }
    if (std::isalpha(peek()) || peek() == '_')
        throw LexError("Invalid number literal", current_location());
    std::string lexeme(source_.substr(start_pos, pos_ - start_pos));
    return Token{ TokenType::INT_LITERAL, lexeme, location };
}


Token Lexer::lex_string() {
    SourceLocation location = current_location();
    size_t start_pos = pos_;
    advance();
    while (true) {
        if (is_at_end())
            throw LexError("Unterminated string", location);
        if (peek() == '\n')
            throw LexError("Unterminated string", location);
        if (peek() == '\\') {
            advance();
            if (is_at_end() || peek() == '\n')
                throw LexError("Unterminated string", location);
            advance();
            continue;
        }
        if (peek() == '"') {
            advance();
            break;
        }
        advance();
    }
    std::string lexeme(source_.substr(start_pos, pos_ - start_pos));
    return Token{ TokenType::STR_LITERAL, lexeme, location };
}


Token Lexer::lex_ident_or_keyword() {
    SourceLocation location = current_location();
    size_t start_pos = pos_;
    while (std::isalnum(peek()) || peek() == '_') {
        advance();
    }
    std::string lexeme(source_.substr(start_pos, pos_ - start_pos));
    if (keywords.contains(lexeme))
        return Token{ keywords.at(lexeme), lexeme, location };
    return Token{ TokenType::IDENT, lexeme, location };
}


Token Lexer::lex_operator_or_punc() {
    SourceLocation location = current_location();
    char curr = peek();
    char next = peek_next();
    std::string lexeme;

    // Check two-character operators
    if (curr == '=' && next == '=') {
        advance(); advance();
        return Token{ TokenType::EQ_EQ, "=="s, location };
    }
    if (curr == '!' && next == '=') {
        advance(); advance();
        return Token{ TokenType::BANG_EQ, "!="s, location };
    }
    if (curr == '<' && next == '=') {
        advance(); advance();
        return Token{ TokenType::LE, "<="s, location };
    }
    if (curr == '>' && next == '=') {
        advance(); advance();
        return Token{ TokenType::GE, ">="s, location };
    }
    if (curr == '&' && next == '&') {
        advance(); advance();
        return Token{ TokenType::AND_AND, "&&"s, location };
    }
    if (curr == '|' && next == '|') {
        advance(); advance();
        return Token{ TokenType::OR_OR, "||"s, location };
    }

    // Check single-character operators and punctuation
    switch (curr) {
        case '+':
            advance();
            return Token{ TokenType::PLUS, "+"s, location };
        case '-':
            advance();
            return Token{ TokenType::MINUS, "-"s, location };
        case '*':
            advance();
            return Token{ TokenType::STAR, "*"s, location };
        case '/':
            advance();
            return Token{ TokenType::SLASH, "/"s, location };
        case '%':
            advance();
            return Token{ TokenType::PERCENT, "%"s, location };
        case '=':
            advance();
            return Token{ TokenType::ASSIGN, "="s, location };
        case '<':
            advance();
            return Token{ TokenType::LT, "<"s, location };
        case '>':
            advance();
            return Token{ TokenType::GT, ">"s, location };
        case '!':
            advance();
            return Token{ TokenType::BANG, "!"s, location };
        case '(':
            advance();
            return Token{ TokenType::LPAREN, "("s, location };
        case ')':
            advance();
            return Token{ TokenType::RPAREN, ")"s, location };
        case '{':
            advance();
            return Token{ TokenType::LBRACE, "{"s, location };
        case '}':
            advance();
            return Token{ TokenType::RBRACE, "}"s, location };
        case ',':
            advance();
            return Token{ TokenType::COMMA, ","s, location };
        case ';':
            advance();
            return Token{ TokenType::SEMIC, ";"s, location };
        case ':':
            advance();
            return Token{ TokenType::COLON, ":"s, location };
        default:
            throw LexError(std::format("Unexpected operator '{}'", curr), location);
    }
}


Token Lexer::next_token() {
    skip_whitespace_and_comments();
    
    if (is_at_end()) 
        return Token{ TokenType::EndOfFile, "", SourceLocation{ line_, column_ } };
    
    char curr = peek();

    if (std::isdigit(curr))
        return lex_number();
    if (curr == '"')
        return lex_string();
    if (std::isalpha(curr) || curr == '_')
        return lex_ident_or_keyword();
    
    return lex_operator_or_punc();
}


std::vector<Token> Lexer::tokenize() {
    std::vector<Token> token_list;
    
    while (true) {
        Token token = next_token();
        token_list.push_back(token);
        if (token.type == TokenType::EndOfFile) break;
    }
    return token_list;
}
