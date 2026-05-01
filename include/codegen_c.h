#pragma once

#include "ast.h"
#include <string>
#include <ostream>


class CCodeGenerator {
public:
    void generate(const Program& program, std::ostream& out);

private:
    std::ostream* out_;
    int indent_level_ = 0;
    std::string current_function_name_;

    void emit(std::string_view code);
    void emit_line(std::string_view line = "");
    void emit_indent();
    void indent();
    void dedent();

    std::string c_type(TypeKind type);

    void gen_program(const Program& program);
    void gen_function(const FunctionDecl& function);
    void gen_block_contents(const BlockStmt& block);
    void gen_block_stmt(const BlockStmt& block);
    void gen_stmt(const Stmt& stmt);
    void gen_expr_stmt(const ExprStmt& stmt);
    void gen_let_stmt(const LetStmt& stmt);
    void gen_if_stmt(const IfStmt& stmt);
    void gen_while_stmt(const WhileStmt& stmt);
    void gen_return_stmt(const ReturnStmt& stmt);

    std::string gen_expr(const Expr& expr);
    std::string gen_assign_expr(const AssignExpr& expr);
    std::string gen_binary_expr(const BinaryExpr& expr);
    std::string gen_unary_expr(const UnaryExpr& expr);
    std::string gen_call_expr(const CallExpr& expr);
    std::string gen_identifier_expr(const IdentifierExpr& expr);
    std::string gen_literal_expr(const IntLiteralExpr& expr);
    std::string gen_literal_expr(const StrLiteralExpr& expr);
    std::string gen_literal_expr(const BoolLiteralExpr& expr);
};