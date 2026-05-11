#pragma once

#include "parser.h"
#include <string>
#include <format>
#include <vector>
#include <unordered_map>
#include <unordered_set>


static const std::unordered_set<std::string_view> keywords = {
    "fn", "let", "if", "else", "while", "return", 
    "struct", "enum", "vec", "int", "bool", "str", "void", "true", "false"
};


struct FunctionSignature {
    std::string name;
    std::vector<Type> param_types;
    Type return_type;
    SourceLocation location;
};


struct FieldInfo {
    std::string name;
    Type type;
    SourceLocation location;
};


struct StructInfo {
    std::string name;
    SourceLocation location;
    std::vector<FieldInfo> fields;
    std::unordered_map<std::string, size_t> field_index;
};


struct EnumInfo {
    std::string name;
    SourceLocation location;
    std::vector<std::string> members;
    std::unordered_map<std::string, size_t> member_index;
};


struct VariableInfo {
    std::string name;
    Type type;
    SourceLocation location;
};


class SemanticAnalyzer : public ASTVisitor {
public:
    void analyze(const Program& program);

    void visit(const Program& program) override;
    void visit(const FunctionDecl& func) override;
    void visit(const ParamField& param) override;
    void visit(const StructDecl& struct_decl) override;
    void visit(const EnumDecl& enum_decl) override;

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
    void visit(const StructLiteralExpr& expr) override;
    void visit(const FieldAccessExpr& expr) override;

    void visit_vec_new(const CallExpr& expr);
    void visit_vec_len(const CallExpr& expr);
    void visit_vec_push(const CallExpr& expr);
    void visit_vec_get(const CallExpr& expr);
    void visit_vec_set(const CallExpr& expr);

private:
    Type last_type_{TypeKind::Void};
    bool last_stmt_returns_;

    std::string_view source_name_;
    const Program* current_program_ = nullptr;
    std::unordered_map<std::string, FunctionSignature> functions_;
    std::unordered_map<std::string, StructInfo> structs_;
    std::unordered_map<std::string, EnumInfo> enums_;
    std::vector<std::unordered_map<std::string, VariableInfo>> scopes_;
    Type current_return_type_{TypeKind::Void};
    std::string current_function_name_;
    const Type* current_expected_type_ = nullptr;

    void collect_struct_declarations(const Program& program);
    void collect_enum_declarations(const Program& program);
    void collect_function_signatures(const Program& program);
    void install_builtin_functions();
    
    void push_scope();
    void pop_scope();
    void declare_variable(std::string_view name, Type type, const SourceLocation& loc);
    const VariableInfo& resolve_variable(std::string_view name, const SourceLocation& loc) const;
    Type infer_expr_type(const Expr& expr, const Type* expected_type = nullptr, std::string_view context = "expression");

    Type resolve_type(const Type& type, const SourceLocation& loc);
    void expect_type(Type actual, Type expected, const SourceLocation& loc, std::string_view context) const;
    bool is_lvalue(const Expr& expr) const;
    bool is_keyword(std::string_view name) const;
};


class SemaError : public std::runtime_error {
public:
    SemaError(const std::string& message, const SourceLocation& location) : 
        std::runtime_error(std::format("SemanticError at {}:{}: {}", 
            location.line, location.column, message)) {}
};
