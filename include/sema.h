#pragma once

#include "parser.h"
#include <string>
#include <format>
#include <vector>
#include <unordered_map>


struct FunctionSignature {
    std::string name;
    std::vector<TypeKind> param_types;
    TypeKind return_type;
    SourceLocation location;
};


struct VariableInfo {
    std::string name;
    TypeKind type;
    SourceLocation location;
};


class SemanticAnalyzer : public ASTVisitor {
public:
    void analyze(const Program& program);

    void visit(const Program& program) override;
    void visit(const FunctionDecl& func) override;
    void visit(const ParamDecl& param) override;
    void visit(const BlockStmt& block) override;
    void visit(const LetStmt& stmt) override;
    void visit(const IfStmt& stmt) override;
    void visit(const WhileStmt& stmt) override;
    void visit(const ReturnStmt& stmt) override;
    void visit(const ExprStmt& stmt) override;

    void visit(const AssignExpr& expr) override;
    void visit(const BinaryExpr& expr) override;
    void visit(const UnaryExpr& expr) override;
    void visit(const CallExpr& expr) override;
    void visit(const IdentifierExpr& expr) override;
    void visit(const IntLiteralExpr& expr) override;
    void visit(const BoolLiteralExpr& expr) override;
    void visit(const StrLiteralExpr& expr) override;

private:
    TypeKind last_type_;
    bool last_stmt_returns_;

    std::string_view source_name_;
    std::unordered_map<std::string, FunctionSignature> functions_;
    std::vector<std::unordered_map<std::string, VariableInfo>> scopes_;
    TypeKind current_return_type_;
    std::string current_function_name_;

    void collect_function_signatures(const Program& program);
    void install_builtin_functions();
    
    void push_scope();
    void pop_scope();
    void declare_variable(std::string_view name, TypeKind type, const SourceLocation& loc);
    const VariableInfo& resolve_variable(std::string_view name, const SourceLocation& loc) const;

    void expect_type(TypeKind actual, TypeKind expected, const SourceLocation& loc, std::string_view context) const;
    bool is_lvalue(const Expr& expr) const;
};


class SemaError : public std::runtime_error {
public:
    SemaError(const std::string& message, const SourceLocation& location) : 
        std::runtime_error(std::format("SemanticError at {}:{}: {}", 
            location.line, location.column, message)) {}
};
