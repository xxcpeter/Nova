#pragma once


struct Program;
struct FunctionDecl;
struct ParamDecl;

struct BlockStmt;
struct LetStmt;
struct IfStmt;
struct WhileStmt;
struct ReturnStmt;
struct ExprStmt;

struct AssignExpr;
struct BinaryExpr;
struct UnaryExpr;
struct CallExpr;
struct IdentifierExpr;
struct IntLiteralExpr;
struct BoolLiteralExpr;
struct StrLiteralExpr;


struct ASTVisitor {
    virtual ~ASTVisitor() = default;

    virtual void visit(const Program&) = 0;
    virtual void visit(const FunctionDecl&) = 0;
    virtual void visit(const ParamDecl&) = 0;

    virtual void visit(const BlockStmt&) = 0;
    virtual void visit(const LetStmt&) = 0;
    virtual void visit(const IfStmt&)= 0;
    virtual void visit(const WhileStmt&) = 0;
    virtual void visit(const ReturnStmt&) = 0;
    virtual void visit(const ExprStmt&) = 0;

    virtual void visit(const AssignExpr&) = 0;
    virtual void visit(const BinaryExpr&) = 0;
    virtual void visit(const UnaryExpr&) = 0;
    virtual void visit(const CallExpr&) = 0;
    virtual void visit(const IdentifierExpr&) = 0;
    virtual void visit(const IntLiteralExpr&) = 0;
    virtual void visit(const BoolLiteralExpr&) = 0;
    virtual void visit(const StrLiteralExpr&) = 0;
};
