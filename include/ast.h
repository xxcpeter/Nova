#pragma once

#include "token.h"

#include <string>
#include <vector>
#include <memory>
#include <typeinfo>
#include <ostream>
#include <iomanip>


enum class TypeKind {
    Int,
    Bool,
    Str,
    Void,
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
    virtual void dump(std::ostream& os, int indent) const = 0;
    // virtual const char* node_name() const = 0;
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
        statements(std::move(stmts)), Stmt(loc) {}

    void dump(std::ostream& os, int indent) const override {
        os << std::string(indent, ' ') << "BlockStmt\n";
        for (const auto& stmt : statements) {
            stmt->dump(os, indent + 2);
        }
    }
};


struct ParamDecl {
    std::string name;
    TypeKind type;
    SourceLocation location;

    ParamDecl(std::string name, TypeKind type, SourceLocation loc) : 
        name(std::move(name)), type(type), location(loc) {}

    void dump(std::ostream& os, int indent) const {
        os << std::string(indent, ' ') << "ParamDecl name=" << name << " type=" << type_to_string(type) << "\n";
    }
};


struct FunctionDecl : Decl {
    std::string name;
    std::vector<ParamDecl> params;
    TypeKind returnType;
    std::unique_ptr<BlockStmt> body;

    FunctionDecl(std::string name, std::vector<ParamDecl> params, TypeKind returnType, std::unique_ptr<BlockStmt> body, SourceLocation loc) :
        name(std::move(name)), params(std::move(params)), returnType(returnType), body(std::move(body)), Decl(loc) {}

    void dump(std::ostream& os, int indent) const override {
        os << std::string(indent, ' ') << "FunctionDecl name=" << name << " return=" << type_to_string(returnType) << "\n";
        os << std::string(indent + 2, ' ') << "Params\n";
        for (const auto& param : params) {
            param.dump(os, indent + 4);
        }
        os << std::string(indent + 2, ' ') << "Body\n";
        body->dump(os, indent + 4);
    }
};


struct Program : ASTNode {
    std::string name;
    std::vector<std::unique_ptr<FunctionDecl>> functions;

    Program(std::string name, std::vector<std::unique_ptr<FunctionDecl>> functions, SourceLocation loc) :
        name(std::move(name)), functions(std::move(functions)), ASTNode(loc) {}

    void dump(std::ostream& os, int indent) const override {
        os << std::string(indent, ' ') << "Program name=" << name << "\n";
        for (const auto& func : functions) {
            func->dump(os, indent + 2);
        }
    }
};


struct LetStmt : Stmt {
    std::string name;
    TypeKind declared_type;
    std::unique_ptr<Expr> initializer;

    LetStmt(std::string name, TypeKind declared_type, std::unique_ptr<Expr> initializer, SourceLocation loc) :
        name(std::move(name)), declared_type(declared_type), initializer(std::move(initializer)), Stmt(loc) {}

    void dump(std::ostream& os, int indent) const override {
        os << std::string(indent, ' ') << "LetStmt name=" << name << " type=" << type_to_string(declared_type) << "\n";
        if (initializer) {
            initializer->dump(os, indent + 2);
        }
    }
};


struct IfStmt : Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> then_branch;
    std::unique_ptr<Stmt> else_branch; // optional

    IfStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> then_branch, std::unique_ptr<Stmt> else_branch, SourceLocation loc) :
        condition(std::move(condition)), then_branch(std::move(then_branch)), else_branch(std::move(else_branch)), Stmt(loc) {}

    void dump(std::ostream& os, int indent) const override {
        os << std::string(indent, ' ') << "IfStmt\n";
        os << std::string(indent + 2, ' ') << "Condition\n";
        condition->dump(os, indent + 4);
        os << std::string(indent + 2, ' ') << "Then\n";
        then_branch->dump(os, indent + 4);
        if (else_branch) {
            os << std::string(indent + 2, ' ') << "Else\n";
            else_branch->dump(os, indent + 4);
        }
    }
};


struct WhileStmt : Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;

    WhileStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body, SourceLocation loc) :
        condition(std::move(condition)), body(std::move(body)), Stmt(loc) {}
    
    void dump(std::ostream& os, int indent) const override {
        os << std::string(indent, ' ') << "WhileStmt\n";
        os << std::string(indent + 2, ' ') << "Condition\n";
        condition->dump(os, indent + 4);
        os << std::string(indent + 2, ' ') << "Body\n";
        body->dump(os, indent + 4);
    }
};


struct ReturnStmt : Stmt {
    std::unique_ptr<Expr> value; // optional

    ReturnStmt(std::unique_ptr<Expr> value, SourceLocation loc) :
        value(std::move(value)), Stmt(loc) {}

    void dump(std::ostream& os, int indent) const override {
        os << std::string(indent, ' ') << "ReturnStmt\n";
        if (value) {
            value->dump(os, indent + 2);
        }
    }
};


struct ExprStmt : Stmt {
    std::unique_ptr<Expr> expr;

    ExprStmt(std::unique_ptr<Expr> expr, SourceLocation loc) :
        expr(std::move(expr)), Stmt(loc) {}

    void dump(std::ostream& os, int indent) const override {
        os << std::string(indent, ' ') << "ExprStmt\n";
        expr->dump(os, indent + 2);
    }
};


struct AssignExpr : Expr {
    std::unique_ptr<Expr> target;
    std::unique_ptr<Expr> value;

    AssignExpr(std::unique_ptr<Expr> target, std::unique_ptr<Expr> value, SourceLocation loc) :
        target(std::move(target)), value(std::move(value)), Expr(loc) {}

    void dump(std::ostream& os, int indent) const override {
        os << std::string(indent, ' ') << "AssignExpr\n";
        os << std::string(indent + 2, ' ') << "Target\n";
        target->dump(os, indent + 4);
        os << std::string(indent + 2, ' ') << "Value\n";
        value->dump(os, indent + 4);
    }
};


struct BinaryExpr : Expr {
    BinaryOp op;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;

    BinaryExpr(BinaryOp op, std::unique_ptr<Expr> left, std::unique_ptr<Expr> right, SourceLocation loc) :
        op(op), left(std::move(left)), right(std::move(right)), Expr(loc) {}

    void dump(std::ostream& os, int indent) const override {
        os << std::string(indent, ' ') << "BinaryExpr op=" << binary_op_to_string(op) << "\n";
        left->dump(os, indent + 2);
        right->dump(os, indent + 2);
    }
};


struct UnaryExpr : Expr {
    UnaryOp op;
    std::unique_ptr<Expr> operand;

    UnaryExpr(UnaryOp op, std::unique_ptr<Expr> operand, SourceLocation loc) :
        op(op), operand(std::move(operand)), Expr(loc) {}

    void dump(std::ostream& os, int indent) const override {
        os << std::string(indent, ' ') << "UnaryExpr op=" << unary_op_to_string(op) << "\n";
        operand->dump(os, indent + 2);
    }
};


struct CallExpr : Expr {
    std::string callee;
    std::vector<std::unique_ptr<Expr>> arguments;
    
    CallExpr(std::string callee, std::vector<std::unique_ptr<Expr>> arguments, SourceLocation loc) :
        callee(std::move(callee)), arguments(std::move(arguments)), Expr(loc) {}

    void dump(std::ostream& os, int indent) const override {
        os << std::string(indent, ' ') << "CallExpr callee=" << callee << "\n";
        for (const auto& arg : arguments) {
            arg->dump(os, indent + 2);
        }
    }
};


struct IdentifierExpr : Expr {
    std::string name;

    IdentifierExpr(std::string name, SourceLocation loc) :
        name(std::move(name)), Expr(loc) {}

    void dump(std::ostream& os, int indent) const override {
        os << std::string(indent, ' ') << "IdentifierExpr name=" << name << "\n";
    }
};


struct IntLiteralExpr : Expr {
    long long int value;

    IntLiteralExpr(long long int value, SourceLocation loc) :
        value(value), Expr(loc) {}

    void dump(std::ostream& os, int indent) const override {
        os << std::string(indent, ' ') << "IntLiteralExpr value=" << value << "\n";
    }
};


struct BoolLiteralExpr : Expr {
    bool value;

    BoolLiteralExpr(bool value, SourceLocation loc) :
        value(value), Expr(loc) {}
    
    void dump(std::ostream& os, int indent) const override {
        os << std::string(indent, ' ') << "BoolLiteralExpr value=" << value << "\n";
    }
};


struct StrLiteralExpr : Expr {
    std::string lexeme;

    StrLiteralExpr(std::string value, SourceLocation loc) :
        lexeme(std::move(value)), Expr(loc) {}

    void dump(std::ostream& os, int indent) const override {
        os << std::string(indent, ' ') << "StringLiteralExpr lexeme=" << std::quoted(lexeme) << "\n";
    }
};
