#pragma once

#include "token.h"
#include "ast_visitor.h"

#include <string>
#include <vector>
#include <memory>


enum class TypeKind {
    Int,
    Bool,
    Str,
    Void,
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


inline const char* type_to_string(TypeKind type) {
    switch (type) {
        case TypeKind::Int:  return "int";
        case TypeKind::Bool: return "bool";
        case TypeKind::Str:  return "str";
        case TypeKind::Void: return "void";
        default:             return "unknown";
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
};


struct BlockStmt : Stmt {
    std::vector<std::unique_ptr<Stmt>> statements;
    
    BlockStmt(std::vector<std::unique_ptr<Stmt>> stmts, SourceLocation loc) : 
         Stmt(loc), statements(std::move(stmts)) {}
    
    void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
};


struct ParamDecl {
    SourceLocation location;
    std::string name;
    TypeKind type;

    ParamDecl(std::string name, TypeKind type, SourceLocation loc) : 
        location(loc), name(std::move(name)), type(type) {}

    void accept(ASTVisitor& visitor) const { visitor.visit(*this); }
};


struct FunctionDecl : Decl {
    std::string name;
    std::vector<ParamDecl> params;
    TypeKind return_type;
    std::unique_ptr<BlockStmt> body;

    FunctionDecl(std::string name, std::vector<ParamDecl> params, TypeKind return_type, std::unique_ptr<BlockStmt> body, SourceLocation loc) :
        Decl(loc), name(std::move(name)), params(std::move(params)), return_type(return_type), body(std::move(body)) {}
    
    void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
};


struct Program : ASTNode {
    std::string name;
    std::vector<std::unique_ptr<FunctionDecl>> functions;

    Program(std::string name, std::vector<std::unique_ptr<FunctionDecl>> functions, SourceLocation loc) :
        ASTNode(loc), name(std::move(name)), functions(std::move(functions)) {}

    void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
};


struct LetStmt : Stmt {
    std::string name;
    TypeKind declared_type;
    std::unique_ptr<Expr> initializer;
    SourceLocation name_location;

    LetStmt(std::string name, TypeKind declared_type, std::unique_ptr<Expr> initializer, SourceLocation loc, SourceLocation name_loc) :
        Stmt(loc), name(std::move(name)), declared_type(declared_type), initializer(std::move(initializer)), name_location(name_loc) {}

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