#pragma once

#include "ast.h"
#include "token.h"
#include <vector>
#include <format>
#include <stdexcept>
#include <string_view>


class Parser {
public:
    Parser(const std::vector<Token>& tokens, std::string_view source_name) : 
        tokens_(tokens), source_name_(source_name) {}
    
    std::unique_ptr<Program> parse_program();


private:
    const std::vector<Token>& tokens_;
    std::string_view source_name_;
    size_t current = 0;

    bool is_at_end() const;
    const Token& peek() const;
    const Token& previous() const;
    const Token& advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    const Token& expect(TokenType type, const std::string_view message);
    bool match_any(std::initializer_list<TokenType> types);
    bool is_expr_start(TokenType type) const;
    bool is_stmt_boundary(TokenType type) const;
    std::unique_ptr<Expr> parse_required_expr(std::string_view message);

    std::unique_ptr<FunctionDecl> parse_function();
    std::vector<ParamDecl> parse_param_list();
    TypeKind parse_type();

    std::unique_ptr<Stmt> parse_statement();
    std::unique_ptr<BlockStmt> parse_block();
    std::unique_ptr<Stmt> parse_let_stmt();
    std::unique_ptr<Stmt> parse_if_stmt();
    std::unique_ptr<Stmt> parse_while_stmt();
    std::unique_ptr<Stmt> parse_return_stmt();
    std::unique_ptr<Stmt> parse_expr_stmt();

    std::unique_ptr<Expr> parse_expression();
    std::unique_ptr<Expr> parse_assignment();
    std::unique_ptr<Expr> parse_logical_or();
    std::unique_ptr<Expr> parse_logical_and();
    std::unique_ptr<Expr> parse_equality();
    std::unique_ptr<Expr> parse_comparison();
    std::unique_ptr<Expr> parse_additive();
    std::unique_ptr<Expr> parse_multiplicative();
    std::unique_ptr<Expr> parse_unary();
    std::unique_ptr<Expr> parse_primary();
};


class ParseError : public std::runtime_error {
public:
    ParseError(const std::string& message, const SourceLocation& location) : 
        std::runtime_error(std::format("ParseError at {}:{}: {}", 
            location.line, location.column, message)) {}
};