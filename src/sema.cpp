#include "sema.h"

#include <string>


void SemanticAnalyzer::collect_function_signatures(const Program& program) {
    for (const auto& func : program.functions) {
        if (functions_.contains(func->name)) {
            throw SemaError(std::format("duplicate function '{}'", func->name), func->location);
        }
        std::vector<TypeKind> param_types;
        for (const auto& param : func->params) {
            param_types.push_back(param.type);
        }
        functions_.emplace(func->name, FunctionSignature{func->name, param_types, func->return_type, func->location});
    }
}


void SemanticAnalyzer::install_builtin_functions() {
    functions_.emplace("print_int", FunctionSignature{"print_int", {TypeKind::Int}, TypeKind::Void, SourceLocation{0, 0}});
    functions_.emplace("print_str", FunctionSignature{"print_str", {TypeKind::Str}, TypeKind::Void, SourceLocation{0, 0}});
    functions_.emplace("str_eq", FunctionSignature{"str_eq", {TypeKind::Str, TypeKind::Str}, TypeKind::Bool, SourceLocation{0, 0}});
    functions_.emplace("str_concat", FunctionSignature{"str_concat", {TypeKind::Str, TypeKind::Str}, TypeKind::Str, SourceLocation{0, 0}});
    functions_.emplace("read_file", FunctionSignature{"read_file", {TypeKind::Str}, TypeKind::Str, SourceLocation{0, 0}});
    functions_.emplace("write_file", FunctionSignature{"write_file", {TypeKind::Str, TypeKind::Str}, TypeKind::Void, SourceLocation{0, 0}});
}


void SemanticAnalyzer::push_scope() {
    scopes_.emplace_back();
}


void SemanticAnalyzer::pop_scope() {
    scopes_.pop_back();
}


void SemanticAnalyzer::declare_variable(std::string_view name, TypeKind type, const SourceLocation& loc) {
    auto& current_scope = scopes_.back();
    if (current_scope.contains(std::string(name))) {
        throw SemaError(std::format("duplicate variable '{}'", name), loc);
    }
    current_scope.emplace(std::string(name), VariableInfo{ std::string(name), type, loc });
}


const VariableInfo& SemanticAnalyzer::resolve_variable(std::string_view name, const SourceLocation& loc) const {
    for (auto scope_it = scopes_.rbegin(); scope_it != scopes_.rend(); ++scope_it) {
        const auto& scope = *scope_it;
        if (scope.contains(std::string(name))) {
            return scope.at(std::string(name));
        }
    }
    throw SemaError(std::format("undefined variable '{}'", name), loc);
}


void SemanticAnalyzer::expect_type(TypeKind actual, TypeKind expected, const SourceLocation& loc, std::string_view context) const {
    if (actual != expected) {
        throw SemaError(std::format("type error in {}: expected {}, got {}", 
            context, type_to_string(expected), type_to_string(actual)), loc);
    }
}


bool SemanticAnalyzer::is_lvalue(const Expr& expr) const {
    return dynamic_cast<const IdentifierExpr*>(&expr) != nullptr;
}


void SemanticAnalyzer::analyze(const Program& program) {
    source_name_ = program.name;
    program.accept(*this);
}


void SemanticAnalyzer::visit(const Program& program) {
    install_builtin_functions();
    collect_function_signatures(program);
    for (const auto& func : program.functions) {
        func->accept(*this);
    }
}


void SemanticAnalyzer::visit(const FunctionDecl& func) {
    current_function_name_ = func.name;
    current_return_type_ = func.return_type;

    push_scope();

    for (const auto& param : func.params) {
        param.accept(*this);
    }
    
    bool body_returns = false;
    for (const auto& stmt : func.body->statements) {
        stmt->accept(*this);
        if (last_stmt_returns_) {
            body_returns = true;
            break;
        }
    }

    pop_scope();

    if (func.return_type != TypeKind::Void && !body_returns) {
        throw SemaError(std::format("missing return statement in function '{}'", func.name), func.location);
    }

    last_stmt_returns_ = false;
    // todo: check for missing return statement in non-void functions
}


void SemanticAnalyzer::visit(const ParamDecl& param) {
    declare_variable(param.name, param.type, param.location);
}


void SemanticAnalyzer::visit(const BlockStmt& block) {
    push_scope();

    bool block_returns = false;
    for (const auto& stmt : block.statements) {
        stmt->accept(*this);
        if (last_stmt_returns_) {
            block_returns = true;
            break;
        }
    }

    pop_scope();
    last_stmt_returns_ = block_returns;
}


void SemanticAnalyzer::visit(const LetStmt& stmt) {
    stmt.initializer->accept(*this);
    expect_type(last_type_, stmt.declared_type, stmt.name_location, "variable initializer");
    
    declare_variable(stmt.name, stmt.declared_type, stmt.name_location);
    last_stmt_returns_ = false;
}


void SemanticAnalyzer::visit(const IfStmt& stmt) {
    stmt.condition->accept(*this);
    expect_type(last_type_, TypeKind::Bool, stmt.condition->location, "if condition");

    stmt.then_branch->accept(*this);
    bool then_returns = last_stmt_returns_;

    bool else_returns = false;
    if (stmt.else_branch) {
        stmt.else_branch->accept(*this);
        else_returns = last_stmt_returns_;
    }

    last_stmt_returns_ = then_returns && else_returns;
}


void SemanticAnalyzer::visit(const WhileStmt& stmt) {
    stmt.condition->accept(*this);
    expect_type(last_type_, TypeKind::Bool, stmt.condition->location, 
        "while condition");

    stmt.body->accept(*this);
    bool body_returns = last_stmt_returns_;

    bool is_infinite_loop = false;
    if (const auto* bool_literal = 
        dynamic_cast<const BoolLiteralExpr*>(stmt.condition.get())) {
        is_infinite_loop = bool_literal->value;
    }
    last_stmt_returns_ = body_returns && is_infinite_loop;
}


void SemanticAnalyzer::visit(const ReturnStmt& stmt) {
    if (stmt.value) {
        stmt.value->accept(*this);
        if (current_return_type_ == TypeKind::Void) {
            throw SemaError(std::format("void function {} should not return a value", current_function_name_), stmt.location);
        }
        if (last_type_ != current_return_type_) {
            throw SemaError(std::format("return type mismatch: expected {}, got {}", 
                type_to_string(current_return_type_), type_to_string(last_type_)), stmt.location);
        }
    } else {
        if (current_return_type_ != TypeKind::Void) {
            throw SemaError(std::format("non-void function {} must return a value", current_function_name_), stmt.location);
        }
    }
    last_stmt_returns_ = true;
}


void SemanticAnalyzer::visit(const ExprStmt& stmt) {
    stmt.expr->accept(*this);
    last_stmt_returns_ = false;
}


void SemanticAnalyzer::visit(const AssignExpr& expr) {
    if (!is_lvalue(*expr.target)) {
        throw SemaError("invalid assignment target", expr.target->location);
    }
    expr.value->accept(*this);
    TypeKind value_type = last_type_;

    expr.target->accept(*this);
    TypeKind target_type = last_type_;
    
    expect_type(value_type, target_type, expr.location, "assignment");
    last_type_ = target_type;
}


void SemanticAnalyzer::visit(const BinaryExpr& expr) {
    expr.left->accept(*this);
    TypeKind left_type = last_type_;

    expr.right->accept(*this);
    TypeKind right_type = last_type_;

    switch (expr.op) {
        case BinaryOp::Add:
        case BinaryOp::Sub:
        case BinaryOp::Mul:
        case BinaryOp::Div:
        case BinaryOp::Mod:
            expect_type(left_type, TypeKind::Int, expr.left->location, "arithmetic operator left operand");
            expect_type(right_type, TypeKind::Int, expr.right->location, "arithmetic operator right operand");
            last_type_ = TypeKind::Int;
            return;

        case BinaryOp::LogicalAnd:
        case BinaryOp::LogicalOr:
            expect_type(left_type, TypeKind::Bool, expr.left->location, "logical operator left operand");
            expect_type(right_type, TypeKind::Bool, expr.right->location, "logical operator right operand");
            last_type_ = TypeKind::Bool;
            return;

        case BinaryOp::EqEq:
        case BinaryOp::BangEq:
            if (left_type != right_type) {
                throw SemaError("type mismatch in equality operator", expr.location);
            }
            last_type_ = TypeKind::Bool;
            return;

        case BinaryOp::Less:
        case BinaryOp::Greater:
        case BinaryOp::LeEq:
        case BinaryOp::GrEq:
            expect_type(left_type, TypeKind::Int, expr.left->location, "comparison operator left operand");
            expect_type(right_type, TypeKind::Int, expr.right->location, "comparison operator right operand");
            last_type_ = TypeKind::Bool;
            return;

        default:
            throw SemaError("unknown binary operator", expr.location);
    }
}


void SemanticAnalyzer::visit(const UnaryExpr& expr) {
    expr.operand->accept(*this);
    TypeKind operand_type = last_type_;

    switch (expr.op) {
        case UnaryOp::Negate:
            expect_type(operand_type, TypeKind::Int, expr.operand->location, "negation operator operand");
            last_type_ = TypeKind::Int;
            return;

        case UnaryOp::Not:
            expect_type(operand_type, TypeKind::Bool, expr.operand->location, "logical not operator operand");
            last_type_ = TypeKind::Bool;
            return;

        default:
            throw SemaError("unknown unary operator", expr.location);
    }
}


void SemanticAnalyzer::visit(const CallExpr& expr) {
    if (!functions_.contains(expr.callee)) {
        throw SemaError(std::format("undefined function '{}'", expr.callee), expr.location);
    }
    const auto& func_sig = functions_.at(expr.callee);

    if (expr.arguments.size() != func_sig.param_types.size()) {
        throw SemaError(std::format("argument count mismatch in call to '{}': expected {}, got {}", 
            expr.callee, func_sig.param_types.size(), expr.arguments.size()), expr.location);
    }

    for (size_t i = 0; i < expr.arguments.size(); ++i) {
        const auto& arg = expr.arguments[i];
        arg->accept(*this);
        expect_type(last_type_, func_sig.param_types[i], arg->location, std::format("argument {} in call to '{}'", i + 1, expr.callee));
    }

    last_type_ = func_sig.return_type;
}


void SemanticAnalyzer::visit(const IdentifierExpr& expr) {
    const auto& var_info = resolve_variable(expr.name, expr.location);
    last_type_ = var_info.type;
}


void SemanticAnalyzer::visit(const IntLiteralExpr&) {
    last_type_ = TypeKind::Int;
}


void SemanticAnalyzer::visit(const BoolLiteralExpr&) {
    last_type_ = TypeKind::Bool;
}


void SemanticAnalyzer::visit(const StrLiteralExpr&) {
    last_type_ = TypeKind::Str;
}