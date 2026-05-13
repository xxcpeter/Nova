#include "sema.h"

#include <string>


void SemanticAnalyzer::collect_struct_declarations(const Program& program) {
    for (const auto& struct_decl : program.structs) {
        if (structs_.contains(struct_decl->name) || enums_.contains(struct_decl->name)) {
            throw SemaError(std::format("duplicate struct '{}'", struct_decl->name), struct_decl->location);
        }
        if (is_keyword(struct_decl->name)) {
            throw SemaError(std::format("struct name '{}' cannot be a keyword", struct_decl->name), struct_decl->location);
        }
        structs_.emplace(struct_decl->name, StructInfo{ struct_decl->name, struct_decl->location, {}, {} });
    }

    for (const auto& struct_decl : program.structs) {
        auto& struct_info = structs_.at(struct_decl->name);
        for (const auto& field : struct_decl->fields) {
            Type field_type = resolve_type(field.type, field.type_location);
            if (field_type.kind == TypeKind::Void) {
                throw SemaError(std::format("field '{}' in struct '{}' cannot have void type", field.name, struct_decl->name), field.location);
            }
            if (field_type.kind == TypeKind::Struct && struct_decl->name == field_type.name) {
                throw SemaError(std::format("struct '{}' cannot contain a field of its own type", struct_decl->name), field.type_location);
            }
            if (struct_info.field_index.contains(field.name)) {
                throw SemaError(std::format("duplicate field '{}' in struct '{}'", field.name, struct_decl->name), field.location);
            }
            field.resolved_type = field_type;
            struct_info.field_index.emplace(field.name, struct_info.fields.size());
            struct_info.fields.push_back(FieldInfo{field.name, field_type, field.location});
        }
    }
}


void SemanticAnalyzer::collect_enum_declarations(const Program& program) {
    for (const auto& enum_decl : program.enums) {
        if (enums_.contains(enum_decl->name) || structs_.contains(enum_decl->name)) {
            throw SemaError(std::format("duplicate enum '{}'", enum_decl->name), enum_decl->location);
        }
        if (is_keyword(enum_decl->name)) {
            throw SemaError(std::format("enum name '{}' cannot be a keyword", enum_decl->name), enum_decl->location);
        }
        enums_.emplace(enum_decl->name, EnumInfo{ enum_decl->name, enum_decl->location, {}, {} });
    }

    for (const auto& enum_decl : program.enums) {
        auto& enum_info = enums_.at(enum_decl->name);
        for (const auto& member : enum_decl->members) {
            if (enum_info.member_index.contains(member.name)) {
                throw SemaError(std::format("duplicate member '{}' in enum '{}'", member.name, enum_decl->name), member.location);
            }
            enum_info.member_index.emplace(member.name, enum_info.members.size());
            enum_info.members.push_back(member.name);
        }
    }
}


void SemanticAnalyzer::collect_function_signatures(const Program& program) {
    for (const auto& func : program.functions) {
        if (functions_.contains(func->name)) {
            throw SemaError(std::format("duplicate function '{}'", func->name), func->location);
        }
        if (is_keyword(func->name)) {
            throw SemaError(std::format("function name '{}' cannot be a keyword", func->name), func->location);
        }
        functions_.emplace(func->name, FunctionSignature{ func->name, {}, Type{ TypeKind::Void }, func->location });
    }

    for (const auto& func : program.functions) {
        auto& signature = functions_.at(func->name);
        Type return_type = resolve_type(func->return_type, func->location);
        func->resolved_return_type = return_type;
        for (const auto& param : func->params) {
            Type param_type = resolve_type(param.type, param.location);
            if (param_type.kind == TypeKind::Void) {
                throw SemaError("parameter cannot have void type", param.location);
            }
            param.resolved_type = param_type;
            signature.param_types.push_back(param_type);
        }
        signature.return_type = return_type;
    }
}


void SemanticAnalyzer::install_builtin_functions() {
    functions_.emplace("print_int", FunctionSignature{"print_int", {Type{ TypeKind::Int }}, Type{ TypeKind::Void }, SourceLocation{0, 0}});
    
    functions_.emplace("print_str", FunctionSignature{"print_str", {Type{ TypeKind::Str }}, Type{ TypeKind::Void }, SourceLocation{0, 0}});
    functions_.emplace("str_eq", FunctionSignature{"str_eq", {Type{ TypeKind::Str }, Type{ TypeKind::Str }}, Type{ TypeKind::Bool }, SourceLocation{0, 0}});
    functions_.emplace("str_concat", FunctionSignature{"str_concat", {Type{ TypeKind::Str }, Type{ TypeKind::Str }}, Type{ TypeKind::Str }, SourceLocation{0, 0}});
    functions_.emplace("str_len", FunctionSignature{"str_len", {Type{ TypeKind::Str }}, Type{ TypeKind::Int }, SourceLocation{0, 0}});
    functions_.emplace("str_get", FunctionSignature{"str_get", {Type{ TypeKind::Str }, Type{ TypeKind::Int }}, Type{ TypeKind::Int }, SourceLocation{0, 0}});
    functions_.emplace("str_slice", FunctionSignature{"str_slice", {Type{ TypeKind::Str }, Type{ TypeKind::Int }, Type{ TypeKind::Int }}, Type{ TypeKind::Str }, SourceLocation{0, 0}});
    functions_.emplace("str_starts_with", FunctionSignature{"str_starts_with", {Type{ TypeKind::Str }, Type{ TypeKind::Str }}, Type{ TypeKind::Bool }, SourceLocation{0, 0}});
    functions_.emplace("str_contains", FunctionSignature{"str_contains", {Type{ TypeKind::Str }, Type{ TypeKind::Str }}, Type{ TypeKind::Bool }, SourceLocation{0, 0}});
    functions_.emplace("int_to_str", FunctionSignature{"int_to_str", {Type{ TypeKind::Int }}, Type{ TypeKind::Str }, SourceLocation{0, 0}});

    functions_.emplace("read_file", FunctionSignature{"read_file", {Type{ TypeKind::Str }}, Type{ TypeKind::Str }, SourceLocation{0, 0}});
    functions_.emplace("write_file", FunctionSignature{"write_file", {Type{ TypeKind::Str }, Type{ TypeKind::Str }}, Type{ TypeKind::Void }, SourceLocation{0, 0}});

    functions_.emplace("buf_new", FunctionSignature{"buf_new", {}, Type{ TypeKind::Int }, SourceLocation{0, 0}});
    functions_.emplace("buf_push_str", FunctionSignature{"buf_push_str", {Type{ TypeKind::Int }, Type{ TypeKind::Str }}, Type{ TypeKind::Void }, SourceLocation{0, 0}});
    functions_.emplace("buf_push_int", FunctionSignature{"buf_push_int", {Type{ TypeKind::Int }, Type{ TypeKind::Int }}, Type{ TypeKind::Void }, SourceLocation{0, 0}});
    functions_.emplace("buf_to_str", FunctionSignature{"buf_to_str", {Type{ TypeKind::Int }}, Type{ TypeKind::Str }, SourceLocation{0, 0}});

    functions_.emplace("str_vec_new", FunctionSignature{"str_vec_new", {}, Type{ TypeKind::Int }, SourceLocation{0, 0}});
    functions_.emplace("str_vec_push", FunctionSignature{"str_vec_push", {Type{ TypeKind::Int }, Type{ TypeKind::Str }}, Type{ TypeKind::Void }, SourceLocation{0, 0}});
    functions_.emplace("str_vec_get", FunctionSignature{"str_vec_get", {Type{ TypeKind::Int }, Type{ TypeKind::Int }}, Type{ TypeKind::Str }, SourceLocation{0, 0}});
    functions_.emplace("str_vec_len", FunctionSignature{"str_vec_len", {Type{ TypeKind::Int }}, Type{ TypeKind::Int }, SourceLocation{0, 0}});

    functions_.emplace("arg_count", FunctionSignature{"arg_count", {}, Type{ TypeKind::Int }, SourceLocation{0, 0}});
    functions_.emplace("arg_get", FunctionSignature{"arg_get", {Type{ TypeKind::Int }}, Type{ TypeKind::Str }, SourceLocation{0, 0}});

    functions_.emplace("nova_runtime_error", FunctionSignature{"nova_runtime_error", {Type{ TypeKind::Str }}, Type{ TypeKind::Void }, SourceLocation{0, 0}});
}


void SemanticAnalyzer::push_scope() {
    scopes_.emplace_back();
}


void SemanticAnalyzer::pop_scope() {
    scopes_.pop_back();
}


void SemanticAnalyzer::declare_variable(std::string_view name, Type type, const SourceLocation& loc) {
    auto& current_scope = scopes_.back();
    if (current_scope.contains(std::string(name))) {
        throw SemaError(std::format("duplicate variable '{}'", name), loc);
    }
    if (type.kind == TypeKind::Void) {
        throw SemaError("variable cannot have void type", loc);
    }
    if (structs_.contains(std::string(name)) || enums_.contains(std::string(name))) {
        throw SemaError(std::format("variable name '{}' conflicts with a type name", name), loc);
    }
    Type declared_type = resolve_type(type, loc);
    current_scope.emplace(std::string(name), VariableInfo{ std::string(name), declared_type, loc });
}


const VariableInfo& SemanticAnalyzer::resolve_variable(std::string_view name, const SourceLocation& loc) const {
    for (auto scope_it = scopes_.rbegin(); scope_it != scopes_.rend(); ++scope_it) {
        const auto& scope = *scope_it;
        if (scope.contains(std::string(name))) {
            return scope.at(std::string(name));
        }
    }
    throw SemaError(std::format("undefined variable or enum '{}'", name), loc);
}


Type SemanticAnalyzer::infer_expr_type(const Expr& expr, const Type* expected_type, std::string_view context) {
    const Type* prev_expected_type = current_expected_type_;
    current_expected_type_ = expected_type;

    expr.accept(*this);
    current_expected_type_ = prev_expected_type;

    if (expected_type) {
        expect_type(last_type_, *expected_type, expr.location, context);
    }
    expr.resolved_type = last_type_;
    if (last_type_.kind == TypeKind::Vec) {
        current_program_->vec_types.insert(*last_type_.element_type);
    }

    return last_type_;
}


Type SemanticAnalyzer::resolve_type(const Type& type, const SourceLocation& loc) {
    switch (type.kind) {
        case TypeKind::Int:
        case TypeKind::Bool:
        case TypeKind::Str:
        case TypeKind::Void:
            return type;
        case TypeKind::Struct:
        case TypeKind::Enum:
            return type;
        case TypeKind::Named: {
            if (structs_.contains(type.name)) {
                return Type{ TypeKind::Struct, type.location, type.name };
            } else if (enums_.contains(type.name)) {
                return Type{ TypeKind::Enum, type.location, type.name };
            }
            throw SemaError(std::format("unknown type '{}'", type.name), loc);
        }
        case TypeKind::Vec: {
            Type elem = resolve_type(*type.element_type, type.element_type->location);
            if (elem == TypeKind::Void) {
                throw SemaError("vector element type cannot be void", type.element_type->location);
            }
            if (elem.kind == TypeKind::Vec) {
                throw SemaError("vector element type cannot be a vector", type.element_type->location);
            }
            return Type{ TypeKind::Vec, type.location, "", std::make_shared<Type>(elem) };
        }
    }
    throw SemaError("unknown type", loc);
}


void SemanticAnalyzer::expect_type(Type actual, Type expected, const SourceLocation& loc, std::string_view context) const {
    if (actual != expected) {
        throw SemaError(std::format("type error in {}: expected {}, got {}", 
            context, type_to_string(expected), type_to_string(actual)), loc);
    }
}


bool SemanticAnalyzer::is_lvalue(const Expr& expr) const {
    return dynamic_cast<const IdentifierExpr*>(&expr) != nullptr || 
        dynamic_cast<const FieldAccessExpr*>(&expr) != nullptr;
}


bool SemanticAnalyzer::is_keyword(std::string_view name) const {
    return keywords.contains(name);
}


void SemanticAnalyzer::analyze(const Program& program) {
    source_name_ = program.name;
    current_program_ = &program;
    program.accept(*this);
}


void SemanticAnalyzer::visit(const Program& program) {
    install_builtin_functions();
    collect_enum_declarations(program);
    collect_struct_declarations(program);
    collect_function_signatures(program);
    
    for (const auto& func : program.functions) {
        func->accept(*this);
    }
}


void SemanticAnalyzer::visit(const FunctionDecl& func) {
    current_function_name_ = func.name;
    current_return_type_ = functions_.at(func.name).return_type;

    push_scope();

    for (size_t i = 0; i < func.params.size(); ++i) {
        declare_variable(
            func.params[i].name,
            functions_.at(func.name).param_types[i],
            func.params[i].location
        );
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

    if (functions_.at(func.name).return_type != Type{ TypeKind::Void } && !body_returns) {
        throw SemaError(std::format("missing return statement in function '{}'", func.name), func.location);
    }

    last_stmt_returns_ = false;
}


void SemanticAnalyzer::visit(const ParamField&) {}


void SemanticAnalyzer::visit(const StructDecl&) {}


void SemanticAnalyzer::visit(const EnumDecl&) {}


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
    Type declared_type = resolve_type(stmt.declared_type, stmt.type_location);
    stmt.resolved_type = declared_type;

    infer_expr_type(*stmt.initializer, &declared_type, "variable initializer");
    declare_variable(stmt.name, declared_type, stmt.name_location);
    last_stmt_returns_ = false;
}


void SemanticAnalyzer::visit(const IfStmt& stmt) {
    Type bool_type = Type{ TypeKind::Bool };
    infer_expr_type(*stmt.condition, &bool_type, "if condition");

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
    Type bool_type = Type{ TypeKind::Bool };
    infer_expr_type(*stmt.condition, &bool_type, "while condition");

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
        if (current_return_type_ == Type{ TypeKind::Void }) {
            throw SemaError(std::format("void function {} should not return a value", current_function_name_), stmt.location);
        }
        
        Type value_type = infer_expr_type(*stmt.value, &current_return_type_, "return statement");
        current_return_type_ = value_type;
    } else {
        if (current_return_type_ != Type{ TypeKind::Void }) {
            throw SemaError(std::format("non-void function {} must return a value", current_function_name_), stmt.location);
        }
    }
    last_stmt_returns_ = true;
}


void SemanticAnalyzer::visit(const ExprStmt& stmt) {
    infer_expr_type(*stmt.expr);
    last_stmt_returns_ = false;
}


void SemanticAnalyzer::visit(const AssignExpr& expr) {
    if (!is_lvalue(*expr.target)) {
        throw SemaError("invalid assignment target", expr.target->location);
    }
    Type target_type = infer_expr_type(*expr.target);
    Type value_type = infer_expr_type(*expr.value, &target_type, "assignment");
    
    last_type_ = target_type;
    expr.resolved_type = target_type;
}


void SemanticAnalyzer::visit(const BinaryExpr& expr) {
    Type left_type = infer_expr_type(*expr.left);
    Type right_type = infer_expr_type(*expr.right);

    switch (expr.op) {
        case BinaryOp::Add:
        case BinaryOp::Sub:
        case BinaryOp::Mul:
        case BinaryOp::Div:
        case BinaryOp::Mod:
            expect_type(left_type, Type{ TypeKind::Int }, expr.left->location, "arithmetic operator left operand");
            expect_type(right_type, Type{ TypeKind::Int }, expr.right->location, "arithmetic operator right operand");
            last_type_ = Type{ TypeKind::Int };
            return;

        case BinaryOp::LogicalAnd:
        case BinaryOp::LogicalOr:
            expect_type(left_type, Type{ TypeKind::Bool }, expr.left->location, "logical operator left operand");
            expect_type(right_type, Type{ TypeKind::Bool }, expr.right->location, "logical operator right operand");
            last_type_ = Type{ TypeKind::Bool };
            return;

        case BinaryOp::EqEq:
        case BinaryOp::BangEq:
            if (left_type != right_type) {
                throw SemaError("type mismatch in equality operator", expr.location);
            }
            last_type_ = Type{ TypeKind::Bool };
            return;

        case BinaryOp::Less:
        case BinaryOp::Greater:
        case BinaryOp::LeEq:
        case BinaryOp::GrEq:
            expect_type(left_type, Type{ TypeKind::Int }, expr.left->location, "comparison operator left operand");
            expect_type(right_type, Type{ TypeKind::Int }, expr.right->location, "comparison operator right operand");
            last_type_ = Type{ TypeKind::Bool };
            return;

        default:
            throw SemaError("unknown binary operator", expr.location);
    }
}


void SemanticAnalyzer::visit(const UnaryExpr& expr) {
    Type operand_type = infer_expr_type(*expr.operand);

    switch (expr.op) {
        case UnaryOp::Negate:
            expect_type(operand_type, Type{ TypeKind::Int }, expr.operand->location, "negation operator operand");
            last_type_ = Type{ TypeKind::Int };
            return;

        case UnaryOp::Not:
            expect_type(operand_type, Type{ TypeKind::Bool }, expr.operand->location, "logical not operator operand");
            last_type_ = Type{ TypeKind::Bool };
            return;

        default:
            throw SemaError("unknown unary operator", expr.location);
    }
}


void SemanticAnalyzer::visit(const CallExpr& expr) {
    if (expr.callee == "vec_new") {
        visit_vec_new(expr);
        return;
    }
    if (expr.callee == "vec_len") {
        visit_vec_len(expr);
        return;
    }
    if (expr.callee == "vec_push") {
        visit_vec_push(expr);
        return;
    }
    if (expr.callee == "vec_get") {
        visit_vec_get(expr);
        return;
    }
    if (expr.callee == "vec_set") {
        visit_vec_set(expr);
        return;
    }
    
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
        infer_expr_type(*arg, &func_sig.param_types[i], std::format("argument {} in call to '{}'", i + 1, expr.callee));
    }

    last_type_ = func_sig.return_type;
}


void SemanticAnalyzer::visit(const IdentifierExpr& expr) {
    const auto& var_info = resolve_variable(expr.name, expr.location);
    last_type_ = var_info.type;
}


void SemanticAnalyzer::visit(const IntLiteralExpr&) {
    last_type_ = Type{ TypeKind::Int };
}


void SemanticAnalyzer::visit(const BoolLiteralExpr&) {
    last_type_ = Type{ TypeKind::Bool };
}


void SemanticAnalyzer::visit(const StrLiteralExpr&) {
    last_type_ = Type{ TypeKind::Str };
}


void SemanticAnalyzer::visit(const StructLiteralExpr& expr) {
    if (!structs_.contains(expr.name)) {
        throw SemaError(std::format("undefined struct type '{}'", expr.name), expr.location);
    }
    const auto& struct_info = structs_.at(expr.name);
    std::unordered_set<std::string> seen;

    for (const auto& initializer : expr.field_initializers) {
        if (seen.contains(initializer.name)) {
            throw SemaError(std::format("duplicate field '{}' in struct literal '{}'", initializer.name, expr.name), initializer.location);
        }
        if (!struct_info.field_index.contains(initializer.name)) {
            throw SemaError(std::format("unknown field '{}' in struct '{}'", initializer.name, expr.name), initializer.location);
        }
        const auto& expected_type = struct_info.fields[struct_info.field_index.at(initializer.name)].type;
        Type value_type = infer_expr_type(*initializer.value, &expected_type, "struct field initializer");
        seen.insert(initializer.name);
    }

    for (const auto& field : struct_info.fields) {
        if (!seen.contains(field.name)) {
            throw SemaError(std::format("missing field '{}' in struct literal '{}'", field.name, expr.name), expr.location);
        }
    }

    last_type_ = Type(TypeKind::Struct, expr.location, expr.name);
}


void SemanticAnalyzer::visit(const FieldAccessExpr& expr) {
    if (auto* ident = dynamic_cast<const IdentifierExpr*>(expr.object.get())) {
        if (enums_.contains(ident->name)) {
            const auto& enum_info = enums_.at(ident->name);
            if (!enum_info.member_index.contains(expr.field_name)) {
                throw SemaError(std::format("unknown enum member '{}' in enum '{}'", expr.field_name, ident->name), expr.field_location);
            }
            expr.access_kind = FieldAccessExpr::FieldAccessKind::EnumMember;
            expr.enum_name = ident->name;
            last_type_ = Type(TypeKind::Enum, expr.location, ident->name);
            return;
        }
    }

    Type object_type = infer_expr_type(*expr.object);
    
    if (object_type.kind == TypeKind::Struct) {
        if (!structs_.contains(object_type.name)) {
            throw SemaError(std::format("undefined struct type '{}'", object_type.name), expr.object->location);
        }
        const auto& struct_info = structs_.at(object_type.name);

        if (!struct_info.field_index.contains(expr.field_name)) {
            throw SemaError(std::format("unknown field '{}' in struct '{}'", expr.field_name, object_type.name), expr.field_location);
        }
        expr.access_kind = FieldAccessExpr::FieldAccessKind::StructField;
        last_type_ = struct_info.fields[struct_info.field_index.at(expr.field_name)].type;
        return;
    }

    throw SemaError("field access on non-struct/enum type", expr.object->location);
}


void SemanticAnalyzer::visit_vec_new(const CallExpr& expr) {
    if (!expr.arguments.empty()) {
        throw SemaError("vec_new expects 0 arguments", expr.location);
    }

    if (!current_expected_type_ || current_expected_type_->kind != TypeKind::Vec) {
        throw SemaError("cannot infer element type for vec_new", expr.location);
    }

    Type resolved_type = resolve_type(*current_expected_type_, current_expected_type_->location);
    last_type_ = resolved_type;
}


void SemanticAnalyzer::visit_vec_len(const CallExpr& expr) {
    if (expr.arguments.size() != 1) {
        throw SemaError("vec_len expects 1 argument", expr.location);
    }

    Type arg_type = infer_expr_type(*expr.arguments[0]);
    if (arg_type.kind != TypeKind::Vec) {
        throw SemaError("vec_len expects vec<T>", expr.arguments[0]->location);
    }

    last_type_ = Type{ TypeKind::Int };
}


void SemanticAnalyzer::visit_vec_push(const CallExpr& expr) {
    if (expr.arguments.size() != 2) {
        throw SemaError("vec_push expects 2 arguments", expr.location);
    }

    Type vec_type = infer_expr_type(*expr.arguments[0]);
    if (vec_type.kind != TypeKind::Vec) {
        throw SemaError("vec_push expects vec<T> as first argument", expr.arguments[0]->location);
    }

    Type value_type = infer_expr_type(*expr.arguments[1], vec_type.element_type.get(), "vec_push value");

    last_type_ = Type{ TypeKind::Void };
}


void SemanticAnalyzer::visit_vec_get(const CallExpr& expr) {
    if (expr.arguments.size() != 2) {
        throw SemaError("vec_get expects 2 arguments", expr.location);
    }

    Type vec_type = infer_expr_type(*expr.arguments[0]);
    if (vec_type.kind != TypeKind::Vec) {
        throw SemaError("vec_get expects vec<T> as first argument", expr.arguments[0]->location);
    }

    Type int_type = Type{ TypeKind::Int };
    Type index_type = infer_expr_type(*expr.arguments[1], &int_type, "vec_get index");

    last_type_ = *vec_type.element_type;
}


void SemanticAnalyzer::visit_vec_set(const CallExpr& expr) {
    if (expr.arguments.size() != 3) {
        throw SemaError("vec_set expects 3 arguments", expr.location);
    }

    Type vec_type = infer_expr_type(*expr.arguments[0]);
    if (vec_type.kind != TypeKind::Vec) {
        throw SemaError("vec_set expects vec<T> as first argument", expr.arguments[0]->location);
    }

    Type int_type = Type{ TypeKind::Int };
    Type index_type = infer_expr_type(*expr.arguments[1], &int_type, "vec_set index");
    Type value_type = infer_expr_type(*expr.arguments[2], vec_type.element_type.get(), "vec_set value");

    last_type_ = Type{ TypeKind::Void };
}
