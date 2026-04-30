#pragma once

#include "ast.h"

#include <ostream>
#include <iomanip>


class ASTDumper : public ASTVisitor {
private:
    std::ostream& os;
    int indent_ = 0;
public:
    explicit ASTDumper(std::ostream& os) : os(os) {}

    void visit(const Program& program) override {
        os << std::string(indent_, ' ') << "Program name=" << program.name << "\n";
        for (const auto& func : program.functions) {
            func->accept(*this);
        }
    }

    void visit(const FunctionDecl& func) override {
        os << std::string(indent_, ' ') << "FunctionDecl name=" << func.name << " return=" << type_to_string(func.return_type) << "\n";
        os << std::string(indent_ + 2, ' ') << "Params\n";
        for (const auto& param : func.params) {
            param.accept(*this);
        }
        os << std::string(indent_ + 2, ' ') << "Body\n";
        func.body->accept(*this);
    }

    void visit(const ParamDecl& param) override {
        os << std::string(indent_, ' ') << "ParamDecl name=" << param.name << " type=" << type_to_string(param.type) << "\n";
    }

    void visit(const BlockStmt& block) override {
        os << std::string(indent_, ' ') << "BlockStmt\n";
        for (const auto& stmt : block.statements) {
            stmt->accept(*this);
        }
    }

    void visit(const LetStmt& stmt) override {
        os << std::string(indent_, ' ') << "LetStmt name=" << stmt.name << " type=" << type_to_string(stmt.declared_type) << "\n";
        if (stmt.initializer) {
            stmt.initializer->accept(*this);
        }
    }

    void visit(const IfStmt& stmt) override {
        os << std::string(indent_, ' ') << "IfStmt\n";
        os << std::string(indent_ + 2, ' ') << "Condition\n";
        stmt.condition->accept(*this);
        os << std::string(indent_ + 2, ' ') << "Then\n";
        stmt.then_branch->accept(*this);
        if (stmt.else_branch) {
            os << std::string(indent_ + 2, ' ') << "Else\n";
            stmt.else_branch->accept(*this);
        }
    }

    void visit(const WhileStmt& stmt) override {
        os << std::string(indent_, ' ') << "WhileStmt\n";
        os << std::string(indent_ + 2, ' ') << "Condition\n";
        stmt.condition->accept(*this);
        os << std::string(indent_ + 2, ' ') << "Body\n";
        stmt.body->accept(*this);
    }

    void visit(const ReturnStmt& stmt) override {
        os << std::string(indent_, ' ') << "ReturnStmt\n";
        if (stmt.value) {
            stmt.value->accept(*this);
        }
    }

    void visit(const ExprStmt& stmt) override {
        os << std::string(indent_, ' ') << "ExprStmt\n";
        stmt.expr->accept(*this);
    }

    void visit(const AssignExpr& expr) override {
        os << std::string(indent_, ' ') << "AssignExpr\n";
        os << std::string(indent_ + 2, ' ') << "Target\n";
        expr.target->accept(*this);
        os << std::string(indent_ + 2, ' ') << "Value\n";
        expr.value->accept(*this);
    }

    void visit(const BinaryExpr& expr) override {
        os << std::string(indent_, ' ') << "BinaryExpr op=" << binary_op_to_string(expr.op) << "\n";
        expr.left->accept(*this);
        expr.right->accept(*this);
    }

    void visit(const UnaryExpr& expr) override {
        os << std::string(indent_, ' ') << "UnaryExpr op=" << unary_op_to_string(expr.op) << "\n";
        expr.operand->accept(*this);
    }

    void visit(const CallExpr& expr) override {
        os << std::string(indent_, ' ') << "CallExpr callee=" << expr.callee << "\n";
        for (const auto& arg : expr.arguments) {
            arg->accept(*this);
        }
    }

    void visit(const IdentifierExpr& expr) override {
        os << std::string(indent_, ' ') << "IdentifierExpr name=" << expr.name << "\n";
    }

    void visit(const IntLiteralExpr& expr) override {
        os << std::string(indent_, ' ') << "IntLiteralExpr value=" << expr.value << "\n";
    }

    void visit(const BoolLiteralExpr& expr) override {
        os << std::string(indent_, ' ') << "BoolLiteralExpr value=" << expr.value << "\n";
    }

    void visit(const StrLiteralExpr& expr) override {
        os << std::string(indent_, ' ') << "StringLiteralExpr lexeme=" << std::quoted(expr.lexeme) << "\n";
    }
};