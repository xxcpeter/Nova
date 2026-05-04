#include "codegen_c.h"


void CCodeGenerator::generate(const Program& program, std::ostream& out) {
    out_ = &out;
    gen_program(program);
}


void CCodeGenerator::emit(std::string_view code) {
    (*out_) << code;
}


void CCodeGenerator::emit_line(std::string_view line) {
    emit_indent();
    (*out_) << line << "\n";
}


void CCodeGenerator::emit_indent() {
    for (auto i = 0; i < indent_level_; ++i) {
        (*out_) << "    ";
    }
}


void CCodeGenerator::indent() {
    ++indent_level_;
}


void CCodeGenerator::dedent() {
    --indent_level_;
}


std::string CCodeGenerator::c_type(Type type) {
    switch (type.kind) {
        case TypeKind::Int: return "int";
        case TypeKind::Str: return "const char*";
        case TypeKind::Void: return "void";
        case TypeKind::Bool: return "bool";
        case TypeKind::Struct: return type.name;
    }
    throw std::runtime_error("Unknown type");
}


void CCodeGenerator::gen_program(const Program& program) {
    emit_line("#include <stdio.h>");
    emit_line("#include <stdlib.h>");
    emit_line("#include <string.h>");
    emit_line("#include <stdbool.h>");
    emit_line("#include \"nova_runtime.h\"");
    emit_line();
    for (const auto& struct_decl : program.structs) {
        gen_struct_decl(*struct_decl);
        emit_line();
    }
    for (const auto& function : program.functions) {
        gen_function(*function);
        emit_line();
    }
}


void CCodeGenerator::gen_struct_decl(const StructDecl& struct_decl) {
    emit_line("typedef struct " + struct_decl.name + " " + struct_decl.name + ";");
    emit_line("struct {");
    indent();
    for (const auto& field : struct_decl.fields) {
        emit_line(c_type(field.type) + " " + field.name + ";");
    }
    dedent();
    emit_line("};");
}


void CCodeGenerator::gen_function(const FunctionDecl& function) {
    current_function_name_ = function.name;
    if (function.name == "main") {
        emit_line("int main(int argc, char** argv) {");
        emit_line("    nova_runtime_init(argc, argv);");
    } else {
        std::string params;
        for (const auto& param : function.params) {
            params += c_type(param.type) + " " + param.name;
            if (&param != &function.params.back()) params += ", ";
        }
        emit_line(c_type(function.return_type) + " " + function.name + "(" + params + ") {");
    }
    indent();
    gen_block_contents(*function.body);
    dedent();
    emit_line("}");
}


void CCodeGenerator::gen_block_stmt(const BlockStmt& block) {
    emit_line("{");
    indent();
    gen_block_contents(block);
    dedent();
    emit_line("}");
}


void CCodeGenerator::gen_block_contents(const BlockStmt& block) {
    for (const auto& stmt : block.statements) {
        gen_stmt(*stmt);
    }
}


void CCodeGenerator::gen_stmt(const Stmt& stmt) {
    if (auto* block = dynamic_cast<const BlockStmt*>(&stmt)) {
        gen_block_stmt(*block);
    } else if (auto expr_stmt = dynamic_cast<const ExprStmt*>(&stmt)) {
        gen_expr_stmt(*expr_stmt);
    } else if (auto let_stmt = dynamic_cast<const LetStmt*>(&stmt)) {
        gen_let_stmt(*let_stmt);
    } else if (auto if_stmt = dynamic_cast<const IfStmt*>(&stmt)) {
        gen_if_stmt(*if_stmt);
    } else if (auto while_stmt = dynamic_cast<const WhileStmt*>(&stmt)) {
        gen_while_stmt(*while_stmt);
    } else if (auto return_stmt = dynamic_cast<const ReturnStmt*>(&stmt)) {
        gen_return_stmt(*return_stmt);
    } else {
        throw std::runtime_error("Unknown statement type");
    }
}


void CCodeGenerator::gen_expr_stmt(const ExprStmt& stmt) {
    emit_line(gen_expr(*stmt.expr) + ";");
}


void CCodeGenerator::gen_let_stmt(const LetStmt& stmt) {
    emit_line(c_type(stmt.declared_type) + " " + stmt.name + " = " + gen_expr(*stmt.initializer) + ";");
}


void CCodeGenerator::gen_if_stmt(const IfStmt& stmt) {
    emit_line("if (" + gen_expr(*stmt.condition) + ") ");
    gen_stmt(*stmt.then_branch);
    if (stmt.else_branch) {
        emit_line("else ");
        gen_stmt(*stmt.else_branch);
    }
}


void CCodeGenerator::gen_while_stmt(const WhileStmt& stmt) {
    emit_line("while (" + gen_expr(*stmt.condition) + ") ");
    gen_stmt(*stmt.body);
}


void CCodeGenerator::gen_return_stmt(const ReturnStmt& stmt) {
    if (stmt.value) {
        emit_line("return " + gen_expr(*stmt.value) + ";");
    } else if (current_function_name_ == "main") {
        emit_line("return 0;");
    } else {
        emit_line("return;");
    }
}


std::string CCodeGenerator::gen_expr(const Expr& expr) {
    if (auto assign_expr = dynamic_cast<const AssignExpr*>(&expr)) {
        return gen_assign_expr(*assign_expr);
    } else if (auto binary_expr = dynamic_cast<const BinaryExpr*>(&expr)) {
        return gen_binary_expr(*binary_expr);
    } else if (auto unary_expr = dynamic_cast<const UnaryExpr*>(&expr)) {
        return gen_unary_expr(*unary_expr);
    } else if (auto call_expr = dynamic_cast<const CallExpr*>(&expr)) {
        return gen_call_expr(*call_expr);
    } else if (auto identifier_expr = dynamic_cast<const IdentifierExpr*>(&expr)) {
        return gen_identifier_expr(*identifier_expr);
    } else if (auto int_literal_expr = dynamic_cast<const IntLiteralExpr*>(&expr)) {
        return gen_literal_expr(*int_literal_expr);
    } else if (auto str_literal_expr = dynamic_cast<const StrLiteralExpr*>(&expr)) {
        return gen_literal_expr(*str_literal_expr);
    } else if (auto bool_literal_expr = dynamic_cast<const BoolLiteralExpr*>(&expr)) {
        return gen_literal_expr(*bool_literal_expr);
    } else if (auto struct_literal_expr = dynamic_cast<const StructLiteralExpr*>(&expr)) {
        return gen_literal_expr(*struct_literal_expr);
    } else if (auto field_access_expr = dynamic_cast<const FieldAccessExpr*>(&expr)) {
        return gen_field_access_expr(*field_access_expr);
    } else {
        throw std::runtime_error("Unknown expression type");
    }
}


std::string CCodeGenerator::gen_assign_expr(const AssignExpr& expr) {
    return gen_expr(*expr.target) + " = " + gen_expr(*expr.value);
}


std::string CCodeGenerator::gen_binary_expr(const BinaryExpr& expr) {
    std::string op_code;
    switch (expr.op) {
        case BinaryOp::Add: op_code = "+"; break;
        case BinaryOp::Sub: op_code = "-"; break;
        case BinaryOp::Mul: op_code = "*"; break;
        case BinaryOp::Div: op_code = "/"; break;
        case BinaryOp::Mod: op_code = "%"; break;
        case BinaryOp::LogicalAnd: op_code = "&&"; break;
        case BinaryOp::LogicalOr: op_code = "||"; break;
        case BinaryOp::EqEq: op_code = "=="; break;
        case BinaryOp::BangEq: op_code = "!="; break;
        case BinaryOp::Less: op_code = "<"; break;
        case BinaryOp::Greater: op_code = ">"; break;
        case BinaryOp::LeEq: op_code = "<="; break;
        case BinaryOp::GrEq: op_code = ">="; break;
    }
    return "(" + gen_expr(*expr.left) + " " + op_code + " " + gen_expr(*expr.right) + ")";
}


std::string CCodeGenerator::gen_unary_expr(const UnaryExpr& expr) {
    std::string op_code;
    switch (expr.op) {
        case UnaryOp::Negate: op_code = "-"; break;
        case UnaryOp::Not: op_code = "!"; break;
    }
    return "(" + op_code + gen_expr(*expr.operand) + ")";
}


std::string CCodeGenerator::gen_call_expr(const CallExpr& expr) {
    std::string args_code;
    for (const auto& arg : expr.arguments) {
        args_code += gen_expr(*arg);
        if (arg != expr.arguments.back()) args_code += ", ";
    }
    return expr.callee + "(" + args_code + ")";
}


std::string CCodeGenerator::gen_identifier_expr(const IdentifierExpr& expr) {
    return expr.name;
}


std::string CCodeGenerator::gen_literal_expr(const IntLiteralExpr& expr) {
    return std::to_string(expr.value);
}


std::string CCodeGenerator::gen_literal_expr(const StrLiteralExpr& expr) {
    return expr.lexeme;
}


std::string CCodeGenerator::gen_literal_expr(const BoolLiteralExpr& expr) {
    return expr.value ? "true" : "false";
}


std::string CCodeGenerator::gen_literal_expr(const StructLiteralExpr& expr) {
    std::string code = "((" + expr.name + "){";
    for (const auto& initializer : expr.field_initializers) {
        code += " ." + initializer.name + " = " + gen_expr(*initializer.value) + ",";
    }
    if (!expr.field_initializers.empty()) {
        code.end()[-1] = ' '; // replace last comma with space
    }
    code += "})";
    return code;
}


std::string CCodeGenerator::gen_field_access_expr(const FieldAccessExpr& expr) {
    return "(" + gen_expr(*expr.object) + ")." + expr.field_name;
}