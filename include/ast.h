#pragma once

#include "token.h"
#include "ast_visitor.h"

#include <string>
#include <vector>
#include <memory>
#include <unordered_set>
#include <optional>


enum class TypeKind {
    Int, Bool, Str, Void, Vec,
    Named,
    Struct, Enum
};


struct Type {
    mutable TypeKind kind;
    SourceLocation location;
    std::string name; // only used for Struct and Enum
    std::shared_ptr<Type> element_type; // only used for Vec

    Type(TypeKind kind, SourceLocation loc = {}, std::string name = "", std::shared_ptr<Type> element_type = nullptr) :
        kind(kind), location(loc), name(std::move(name)), element_type(std::move(element_type)) {}
};


struct TypeHash {
    std::size_t operator()(const Type& type) const {
        std::size_t hash = std::hash<int>()(static_cast<int>(type.kind));
        if (type.kind == TypeKind::Struct || type.kind == TypeKind::Enum) {
            hash ^= std::hash<std::string>{}(type.name);
        }
        if (type.kind == TypeKind::Vec) {
            hash ^= operator()(*type.element_type);
        }
        return hash;
    }
};


enum class UnaryOp {
    Negate,     // -
    Not,        // !
};


enum class BinaryOp {
    Add,        // +
    Sub,        // -
    Mul,        // *
    Div,        // /
    Mod,        // %

    LogicalAnd, // &&
    LogicalOr,  // ||

    EqEq,       // ==
    BangEq,     // !=
    Less,       // <
    Greater,    // >
    LeEq,       // <=
    GrEq,       // >=
};


inline bool operator==(const Type& a, const Type& b) {
    if (a.kind != b.kind) return false;
    if (a.kind == TypeKind::Struct || a.kind == TypeKind::Enum) {
        return a.name == b.name;
    }
    if (a.kind == TypeKind::Vec) {
        return a.element_type && b.element_type && *a.element_type == *b.element_type;
    }
    return true;
}


inline std::string type_to_string(Type type) {
    switch (type.kind) {
        case TypeKind::Int:     return "int";
        case TypeKind::Bool:    return "bool";
        case TypeKind::Str:     return "str";
        case TypeKind::Void:    return "void";
        case TypeKind::Named:
        case TypeKind::Struct:
        case TypeKind::Enum:    return type.name;
        case TypeKind::Vec:     return "vec<" + type_to_string(*type.element_type) + ">";
        default:                return "UnknownType";
    }
}


inline const char* unary_op_to_string(UnaryOp op) {
    switch (op) {
        case UnaryOp::Negate: return "Negate";
        case UnaryOp::Not:    return "Not";
        default:              return "UnknownUnaryOp";
    }
}


inline const char* binary_op_to_string(BinaryOp op) {
    switch (op) {
        case BinaryOp::Add:        return "Add";
        case BinaryOp::Sub:        return "Sub";
        case BinaryOp::Mul:        return "Mul";
        case BinaryOp::Div:        return "Div";
        case BinaryOp::Mod:        return "Mod";
        case BinaryOp::LogicalAnd: return "LogicalAnd";
        case BinaryOp::LogicalOr:  return "LogicalOr";
        case BinaryOp::EqEq:       return "EqEq";
        case BinaryOp::BangEq:     return "BangEq";
        case BinaryOp::Less:       return "Less";
        case BinaryOp::Greater:    return "Greater";
        case BinaryOp::LeEq:       return "LeEq";
        case BinaryOp::GrEq:       return "GrEq";
        default:                   return "UnknownBinaryOp";
    }
}


struct ASTNode {
    SourceLocation location;

    ASTNode(SourceLocation location) : location(location) {}
    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor& visitor) const = 0;
};


struct Decl : ASTNode {
    using ASTNode::ASTNode;
};


struct Stmt : ASTNode {
    using ASTNode::ASTNode;
};


struct Expr : ASTNode {
    using ASTNode::ASTNode;
    mutable std::optional<Type> resolved_type;
};


struct BlockStmt : Stmt {
    std::vector<std::unique_ptr<Stmt>> statements;
    
    BlockStmt(std::vector<std::unique_ptr<Stmt>> stmts, SourceLocation loc) : 
         Stmt(loc), statements(std::move(stmts)) {}
    
    void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
};


struct ParamField {
    SourceLocation location;
    std::string name;
    Type type;
    mutable std::optional<Type> resolved_type;

    ParamField(std::string name, Type type, SourceLocation loc) : 
        location(loc), name(std::move(name)), type(type) {}
    
    void accept(ASTVisitor& visitor) const { visitor.visit(*this); }
};


struct FunctionDecl : Decl {
    std::string name;
    std::vector<ParamField> params;
    Type return_type;
    mutable std::optional<Type> resolved_return_type;
    std::unique_ptr<BlockStmt> body;

    FunctionDecl(std::string name, std::vector<ParamField> params, Type return_type, std::unique_ptr<BlockStmt> body, SourceLocation loc) :
        Decl(loc), name(std::move(name)), params(std::move(params)), return_type(return_type), body(std::move(body)) {}
    
    void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
};


struct StructField {
    std::string name;
    Type type;
    SourceLocation location;
    SourceLocation type_location;
    mutable std::optional<Type> resolved_type;

    StructField(std::string name, Type type, SourceLocation loc, SourceLocation type_loc): 
        name(std::move(name)), type(type), location(loc), type_location(type_loc) {}
};


struct StructDecl : Decl {
    std::string name;
    std::vector<StructField> fields;

    StructDecl(std::string name, std::vector<StructField> fields, SourceLocation loc) :
        Decl(loc), name(std::move(name)), fields(std::move(fields)) {}
    
    void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
};


struct EnumMember {
    std::string name;
    SourceLocation location;

    EnumMember(std::string name, SourceLocation loc) : name(std::move(name)), location(loc) {}
};


struct EnumDecl : Decl {
    std::string name;
    std::vector<EnumMember> members;

    EnumDecl(std::string name, std::vector<EnumMember> members, SourceLocation loc) :
        Decl(loc), name(std::move(name)), members(std::move(members)) {}

    void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
};


struct Program : ASTNode {
    std::string name;
    std::vector<std::unique_ptr<FunctionDecl>> functions;
    std::vector<std::unique_ptr<StructDecl>> structs;
    std::vector<std::unique_ptr<EnumDecl>> enums;
    mutable std::unordered_set<Type, TypeHash> vec_types;

    Program(std::string name, std::vector<std::unique_ptr<FunctionDecl>> functions, 
        std::vector<std::unique_ptr<StructDecl>> structs, std::vector<std::unique_ptr<EnumDecl>> enums, SourceLocation loc, std::unordered_set<Type, TypeHash> vec_types) :
        ASTNode(loc), name(std::move(name)), functions(std::move(functions)), structs(std::move(structs)), enums(std::move(enums)), vec_types(vec_types) {}

    void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
};


struct LetStmt : Stmt {
    std::string name;
    Type declared_type;
    mutable std::optional<Type> resolved_type;
    SourceLocation type_location;
    std::unique_ptr<Expr> initializer;
    SourceLocation name_location;

    LetStmt(std::string name, Type declared_type, std::unique_ptr<Expr> initializer, SourceLocation loc, SourceLocation name_loc, SourceLocation type_loc) :
        Stmt(loc), name(std::move(name)), declared_type(declared_type), type_location(type_loc), initializer(std::move(initializer)), name_location(name_loc) {}

    void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
};


struct IfStmt : Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> then_branch;
    std::unique_ptr<Stmt> else_branch; // optional

    IfStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> then_branch, std::unique_ptr<Stmt> else_branch, SourceLocation loc) :
        Stmt(loc), condition(std::move(condition)), then_branch(std::move(then_branch)), else_branch(std::move(else_branch)) {}
    
    void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
};


struct WhileStmt : Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;

    WhileStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body, SourceLocation loc) :
        Stmt(loc), condition(std::move(condition)), body(std::move(body)) {}

    void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
};


struct ReturnStmt : Stmt {
    std::unique_ptr<Expr> value; // optional

    ReturnStmt(std::unique_ptr<Expr> value, SourceLocation loc) :
        Stmt(loc), value(std::move(value)) {}

    void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
};


struct ExprStmt : Stmt {
    std::unique_ptr<Expr> expr;

    ExprStmt(std::unique_ptr<Expr> expr, SourceLocation loc) :
        Stmt(loc), expr(std::move(expr)) {}

    void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
};


struct AssignExpr : Expr {
    std::unique_ptr<Expr> target;
    std::unique_ptr<Expr> value;

    AssignExpr(std::unique_ptr<Expr> target, std::unique_ptr<Expr> value, SourceLocation loc) :
        Expr(loc), target(std::move(target)), value(std::move(value)) {}

    void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
};


struct BinaryExpr : Expr {
    BinaryOp op;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;

    BinaryExpr(BinaryOp op, std::unique_ptr<Expr> left, std::unique_ptr<Expr> right, SourceLocation loc) :
        Expr(loc), op(op), left(std::move(left)), right(std::move(right)) {}

    void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
};


struct UnaryExpr : Expr {
    UnaryOp op;
    std::unique_ptr<Expr> operand;

    UnaryExpr(UnaryOp op, std::unique_ptr<Expr> operand, SourceLocation loc) :
        Expr(loc), op(op), operand(std::move(operand)) {}

    void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
};


struct CallExpr : Expr {
    std::string callee;
    std::vector<std::unique_ptr<Expr>> arguments;
    
    CallExpr(std::string callee, std::vector<std::unique_ptr<Expr>> arguments, SourceLocation loc) :
        Expr(loc), callee(std::move(callee)), arguments(std::move(arguments)) {}

    void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
};


struct IdentifierExpr : Expr {
    std::string name;

    IdentifierExpr(std::string name, SourceLocation loc) :
        Expr(loc), name(std::move(name)) {}

    void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
};


struct IntLiteralExpr : Expr {
    long long int value;

    IntLiteralExpr(long long int value, SourceLocation loc) :
        Expr(loc), value(value) {}
    
    void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
};


struct BoolLiteralExpr : Expr {
    bool value;

    BoolLiteralExpr(bool value, SourceLocation loc) :
        Expr(loc), value(value) {}
    
    void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
};


struct StrLiteralExpr : Expr {
    std::string lexeme;

    StrLiteralExpr(std::string value, SourceLocation loc) :
        Expr(loc), lexeme(std::move(value)) {}

    void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
};


struct StructLiteralField {
    std::string name;
    std::unique_ptr<Expr> value;
    SourceLocation location;

    StructLiteralField(std::string name, std::unique_ptr<Expr> value, SourceLocation loc) :
        name(std::move(name)), value(std::move(value)), location(loc) {}
};


struct StructLiteralExpr : Expr {
    std::string name;
    std::vector<StructLiteralField> field_initializers;

    StructLiteralExpr(std::string name, std::vector<StructLiteralField> field_initializers, SourceLocation loc) :
        Expr(loc), name(std::move(name)), field_initializers(std::move(field_initializers)) {}

    void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
};


struct FieldAccessExpr : Expr {
    enum FieldAccessKind {
        StructField,
        EnumMember,
        Unresolved
    };
    
    mutable FieldAccessKind access_kind = FieldAccessKind::Unresolved;
    mutable std::string enum_name;

    std::unique_ptr<Expr> object;
    std::string field_name;
    SourceLocation field_location;
    SourceLocation dot_location;

    FieldAccessExpr(std::unique_ptr<Expr> object, std::string field_name, SourceLocation loc, SourceLocation field_loc, SourceLocation dot_loc) :
        Expr(loc), object(std::move(object)), field_name(std::move(field_name)), field_location(field_loc), dot_location(dot_loc) {}

    void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
};
