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
        indent_ += 2;
        for (const auto& struct_decl : program.structs) {
            struct_decl->accept(*this);
        }
        for (const auto& func : program.functions) {
            func->accept(*this);
        }
    }

    void visit(const FunctionDecl& func) override {
        os << std::string(indent_, ' ') << "FunctionDecl name=" << func.name << " return=" << type_to_string(func.return_type) << "\n";
        indent_ += 4;
        os << std::string(indent_ - 2, ' ') << "Params\n";
        for (const auto& param : func.params) {
            param.accept(*this);
        }
        os << std::string(indent_ - 2, ' ') << "Body\n";
        func.body->accept(*this);
        indent_ -= 4;
    }

    void visit(const ParamField& param) override {
        os << std::string(indent_, ' ') << "ParamField name=" << param.name << " type=" << type_to_string(param.type) << "\n";
    }

    void visit(const StructDecl& struct_decl) override {
        os << std::string(indent_, ' ') << "StructDecl name=" << struct_decl.name << "\n";
        indent_ += 2;
        for (const auto& field : struct_decl.fields) {
            os << std::string(indent_, ' ') << "StructField name=" << field.name << " type=" << type_to_string(field.type) << "\n";
        }
        indent_ -= 2;
    }

    void visit(const EnumDecl& enum_decl) override {
        os << std::string(indent_, ' ') << "EnumDecl name=" << enum_decl.name << "\n";
        indent_ += 2;
        for (const auto& member : enum_decl.members) {
            os << std::string(indent_, ' ') << "EnumMember name=" << member.name << "\n";
        }
        indent_ -= 2;
    }

    void visit(const BlockStmt& block) override {
        os << std::string(indent_, ' ') << "BlockStmt\n";
        indent_ += 2;
        for (const auto& stmt : block.statements) {
            stmt->accept(*this);
        }
        indent_ -= 2;
    }

    void visit(const LetStmt& stmt) override {
        os << std::string(indent_, ' ') << "LetStmt name=" << stmt.name << " type=" << type_to_string(stmt.declared_type) << "\n";
        indent_ += 2;
        if (stmt.initializer) {
            stmt.initializer->accept(*this);
        }
        indent_ -= 2;
    }

    void visit(const IfStmt& stmt) override {
        os << std::string(indent_, ' ') << "IfStmt\n";
        indent_ += 4;
        os << std::string(indent_ - 2, ' ') << "Condition\n";
        stmt.condition->accept(*this);
        os << std::string(indent_ - 2, ' ') << "Then\n";
        stmt.then_branch->accept(*this);
        if (stmt.else_branch) {
            os << std::string(indent_ - 2, ' ') << "Else\n";
            stmt.else_branch->accept(*this);
        }
        indent_ -= 4;
    }

    void visit(const WhileStmt& stmt) override {
        os << std::string(indent_, ' ') << "WhileStmt\n";
        indent_ += 4;
        os << std::string(indent_ - 2, ' ') << "Condition\n";
        stmt.condition->accept(*this);
        os << std::string(indent_ - 2, ' ') << "Body\n";
        stmt.body->accept(*this);
        indent_ -= 4;
    }

    void visit(const ReturnStmt& stmt) override {
        os << std::string(indent_, ' ') << "ReturnStmt\n";
        indent_ += 2;
        if (stmt.value) {
            stmt.value->accept(*this);
        }
        indent_ -= 2;
    }

    void visit(const ExprStmt& stmt) override {
        os << std::string(indent_, ' ') << "ExprStmt\n";
        indent_ += 2;
        stmt.expr->accept(*this);
        indent_ -= 2;
    }

    void visit(const AssignExpr& expr) override {
        os << std::string(indent_, ' ') << "AssignExpr\n";
        indent_ += 4;
        os << std::string(indent_ - 2, ' ') << "Target\n";
        expr.target->accept(*this);
        os << std::string(indent_ - 2, ' ') << "Value\n";
        expr.value->accept(*this);
        indent_ -= 4;
    }

    void visit(const BinaryExpr& expr) override {
        os << std::string(indent_, ' ') << "BinaryExpr op=" << binary_op_to_string(expr.op) << "\n";
        indent_ += 2;
        expr.left->accept(*this);
        expr.right->accept(*this);
        indent_ -= 2;
    }

    void visit(const UnaryExpr& expr) override {
        os << std::string(indent_, ' ') << "UnaryExpr op=" << unary_op_to_string(expr.op) << "\n";
        indent_ += 2;
        expr.operand->accept(*this);
        indent_ -= 2;
    }

    void visit(const CallExpr& expr) override {
        os << std::string(indent_, ' ') << "CallExpr callee=" << expr.callee << "\n";
        indent_ += 2;
        for (const auto& arg : expr.arguments) {
            arg->accept(*this);
        }
        indent_ -= 2;
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

    void visit(const StructLiteralExpr& expr) override {
        os << std::string(indent_, ' ') << "StructLiteralExpr name=" << expr.name << "\n";
        indent_ += 2;
        for (const auto& initializer : expr.field_initializers) {
            os << std::string(indent_, ' ') << "FieldInitializer name=" << initializer.name << "\n";
            indent_ += 2;
            initializer.value->accept(*this);
            indent_ -= 2;
        }
        indent_ -= 2;
    }

    void visit(const FieldAccessExpr& expr) override {
        os << std::string(indent_, ' ') << "FieldAccessExpr field_name=" << expr.field_name << "\n";
        indent_ += 2;
        expr.object->accept(*this);
        indent_ -= 2;
    }
};