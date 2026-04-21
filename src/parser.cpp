#include "parser.h"


bool Parser::is_at_end() const {
    return peek().type == TokenType::EndOfFile;
}


const Token& Parser::peek() const {
    return tokens_[current];
}


const Token& Parser::previous() const {
    return tokens_[current - 1];
}


const Token& Parser::advance() {
    if (!is_at_end()) current++;
    return previous();
}


bool Parser::check(TokenType type) const {
    return peek().type == type;
}


bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}


const Token& Parser::expect(TokenType type, const std::string_view message) {
    if (check(type)) return advance();

    SourceLocation error_location = peek().location;
    const bool missing_delimiter =
        type == TokenType::SEMIC || type == TokenType::RPAREN ||
        type == TokenType::RBRACE || type == TokenType::COMMA ||
        type == TokenType::COLON;

    if (missing_delimiter && current > 0) {
        const Token& prev = previous();
        error_location = prev.location;
        error_location.column += prev.lexeme.size();
    }

    throw ParseError(std::string(message), error_location);
}


bool Parser::match_any(std::initializer_list<TokenType> types) {
    for (TokenType type : types) {
        if (match(type)) return true;
    }
    return false;
}


bool Parser::is_expr_start(TokenType type) const {
    switch (type) {
        case TokenType::IDENT:
        case TokenType::INT_LITERAL:
        case TokenType::STR_LITERAL:
        case TokenType::KW_TRUE:
        case TokenType::KW_FALSE:
        case TokenType::LPAREN:
        case TokenType::MINUS:
        case TokenType::BANG:
            return true;
        default:
            return false;
    }
}


bool Parser::is_stmt_boundary(TokenType type) const {
    switch (type) {
        case TokenType::EndOfFile:
        // case TokenType::SEMIC:
        case TokenType::RBRACE:
        // case TokenType::COMMA:
        // case TokenType::COLON:
            return true;
        default:
            return false;
    }
}


std::unique_ptr<Expr> Parser::parse_required_expr(std::string_view message) {
    if (!is_expr_start(peek().type)) {
        throw ParseError(std::string(message), peek().location);
    }
    return parse_expression();
}


std::unique_ptr<Program> Parser::parse_program() {
    std::string name(source_name_);
    SourceLocation location = peek().location;
    std::vector<std::unique_ptr<FunctionDecl>> functions;
    while (!is_at_end()) {
        functions.push_back(parse_function());
    }
    return std::make_unique<Program>(
        std::move(name), std::move(functions), location);
}


std::unique_ptr<FunctionDecl> Parser::parse_function() {
    SourceLocation location = peek().location;
    expect(TokenType::KW_FN, "expected 'fn' at the beginning of a function");
    std::string name = expect(TokenType::IDENT, "expected function name").lexeme;
    expect(TokenType::LPAREN, "expected '(' after function name");
    std::vector<ParamDecl> params = parse_param_list();
    expect(TokenType::RPAREN, "expected ')' after parameter list");
    expect(TokenType::COLON, "expected ':' before return type");
    TypeKind returnType = parse_type();
    expect(TokenType::LBRACE, "expected '{' at the beginning of function body");
    std::unique_ptr<BlockStmt> body = parse_block();
    return std::make_unique<FunctionDecl>(
        std::move(name), std::move(params), returnType, std::move(body), location);
}


std::vector<ParamDecl> Parser::parse_param_list() {
    std::vector<ParamDecl> params;
    if (check(TokenType::RPAREN)) return params;
    
    do {
        SourceLocation location = peek().location;
        std::string name = expect(TokenType::IDENT, "expected parameter name").lexeme;
        expect(TokenType::COLON, "expected ':' after parameter name");
        TypeKind type = parse_type();
        params.push_back(ParamDecl(std::move(name), type, location));
    } while (match(TokenType::COMMA));
    
    return params;
}


TypeKind Parser::parse_type() {
    if (match(TokenType::KW_INT)) return TypeKind::Int;
    if (match(TokenType::KW_BOOL)) return TypeKind::Bool;
    if (match(TokenType::KW_STR)) return TypeKind::Str;
    if (match(TokenType::KW_VOID)) return TypeKind::Void;
    throw ParseError("expected type after ':'", peek().location);
}


std::unique_ptr<Stmt> Parser::parse_statement() {
    if (match(TokenType::LBRACE)) return parse_block();
    if (match(TokenType::KW_LET)) return parse_let_stmt();
    if (match(TokenType::KW_IF)) return parse_if_stmt();
    if (match(TokenType::KW_WHILE)) return parse_while_stmt();
    if (match(TokenType::KW_RETURN)) return parse_return_stmt();
    return parse_expr_stmt();
}


std::unique_ptr<BlockStmt> Parser::parse_block() {
    SourceLocation location = previous().location;
    std::vector<std::unique_ptr<Stmt>> statements;
    
    while (!check(TokenType::RBRACE) && !is_at_end()) {
        statements.push_back(parse_statement());
    }
    expect(TokenType::RBRACE, "expected '}' after block");
    
    return std::make_unique<BlockStmt>(std::move(statements), location);
}


std::unique_ptr<Stmt> Parser::parse_let_stmt() {
    SourceLocation location = previous().location;
    
    expect(TokenType::IDENT, "expected variable name");
    std::string name = previous().lexeme;
    expect(TokenType::COLON, "expected ':' after variable name");
    TypeKind declared_type = parse_type();
    expect(TokenType::ASSIGN, "expected '=' after variable type");
    std::unique_ptr<Expr> initializer = parse_required_expr("expected expression after '='");
    expect(TokenType::SEMIC, "expected ';' after variable declaration");
    
    return std::make_unique<LetStmt>(
        std::move(name), declared_type, std::move(initializer), location);
}


std::unique_ptr<Stmt> Parser::parse_if_stmt() {
    SourceLocation location = previous().location;
    
    expect(TokenType::LPAREN, "expected '(' after 'if'");
    std::unique_ptr<Expr> condition = parse_required_expr("expected expression in 'if' condition");
    expect(TokenType::RPAREN, "expected ')' after condition");
    std::unique_ptr<Stmt> then_branch = parse_statement();
    std::unique_ptr<Stmt> else_branch;
    if (match(TokenType::KW_ELSE)) {
        else_branch = parse_statement();
    }
    
    return std::make_unique<IfStmt>(
        std::move(condition), std::move(then_branch), std::move(else_branch), location);
}


std::unique_ptr<Stmt> Parser::parse_while_stmt() {
    SourceLocation location = previous().location;
    
    expect(TokenType::LPAREN, "expected '(' after 'while'");
    std::unique_ptr<Expr> condition = parse_required_expr("expected expression in 'while' condition");
    expect(TokenType::RPAREN, "expected ')' after condition");
    std::unique_ptr<Stmt> body = parse_statement();
    
    return std::make_unique<WhileStmt>(
        std::move(condition), std::move(body), location);
}


std::unique_ptr<Stmt> Parser::parse_return_stmt() {
    SourceLocation location = previous().location;
    std::unique_ptr<Expr> value;

    if (match(TokenType::SEMIC)) {
        return std::make_unique<ReturnStmt>(nullptr, location);
    }
    
    if (is_stmt_boundary(peek().type)) {
        SourceLocation error_location = previous().location;
        error_location.column += previous().lexeme.size();
        throw ParseError("expected ';' after return statement", error_location);
    }

    value = parse_required_expr("expected expression after 'return'");
    expect(TokenType::SEMIC, "expected ';' after return statement");
    return std::make_unique<ReturnStmt>(std::move(value), location);
}


std::unique_ptr<Stmt> Parser::parse_expr_stmt() {
    SourceLocation location = peek().location;
    
    std::unique_ptr<Expr> expr = parse_required_expr("expected expression at start of statement");
    expect(TokenType::SEMIC, "expected ';' after expression");
    
    return std::make_unique<ExprStmt>(std::move(expr), location);
}


std::unique_ptr<Expr> Parser::parse_expression() {
    return parse_assignment();
}


std::unique_ptr<Expr> Parser::parse_assignment() {
    std::unique_ptr<Expr> expr = parse_logical_or();
    if (match(TokenType::ASSIGN)) {
        if (dynamic_cast<IdentifierExpr*>(expr.get()) == nullptr) {
            throw ParseError("invalid assignment target", previous().location);
        }
        SourceLocation location = previous().location;
        std::unique_ptr<Expr> value = parse_assignment();
        return std::make_unique<AssignExpr>(
            std::move(expr), std::move(value), location);
    }
    return expr;
}


std::unique_ptr<Expr> Parser::parse_logical_or() {
    std::unique_ptr<Expr> expr = parse_logical_and();
    while (match(TokenType::OR_OR)) {
        SourceLocation location = previous().location;
        std::unique_ptr<Expr> right = parse_logical_and();
        expr = std::make_unique<BinaryExpr>(
            BinaryOp::LogicalOr, std::move(expr), std::move(right), location);
    }
    return expr;
}


std::unique_ptr<Expr> Parser::parse_logical_and() {
    std::unique_ptr<Expr> expr = parse_equality();
    while (match(TokenType::AND_AND)) {
        SourceLocation location = previous().location;
        std::unique_ptr<Expr> right = parse_equality();
        expr = std::make_unique<BinaryExpr>(
            BinaryOp::LogicalAnd, std::move(expr), std::move(right), location);
    }
    return expr;
}


std::unique_ptr<Expr> Parser::parse_equality() {
    std::unique_ptr<Expr> expr = parse_comparison();
    while (match_any({TokenType::EQ_EQ, TokenType::BANG_EQ})) {
        const Token& op = previous();
        BinaryOp binary_op = (op.type == TokenType::EQ_EQ) ? BinaryOp::EqEq : BinaryOp::BangEq;
        std::unique_ptr<Expr> right = parse_comparison();
        expr = std::make_unique<BinaryExpr>(
            binary_op, std::move(expr), std::move(right), op.location);
    }
    return expr;
}


std::unique_ptr<Expr> Parser::parse_comparison() {
    std::unique_ptr<Expr> expr = parse_additive();
    if (match_any({TokenType::LT, TokenType::LE, TokenType::GT, TokenType::GE})) {
        const Token& op = previous();
        BinaryOp binary_op;
        switch (op.type) {
            case TokenType::LT: binary_op = BinaryOp::Less; break;
            case TokenType::LE: binary_op = BinaryOp::LeEq; break;
            case TokenType::GT: binary_op = BinaryOp::Greater; break;
            case TokenType::GE: binary_op = BinaryOp::GrEq; break;
            default: throw ParseError("Invalid comparison operator", op.location);
        }
        std::unique_ptr<Expr> right = parse_additive();
        expr = std::make_unique<BinaryExpr>(
            binary_op, std::move(expr), std::move(right), op.location);
    }
    return expr;
}


std::unique_ptr<Expr> Parser::parse_additive() {
    std::unique_ptr<Expr> expr = parse_multiplicative();
    while (match_any({TokenType::PLUS, TokenType::MINUS})) {
        const Token& op = previous();
        BinaryOp binary_op = (op.type == TokenType::PLUS) ? BinaryOp::Add : BinaryOp::Sub;
        std::unique_ptr<Expr> right = parse_multiplicative();
        expr = std::make_unique<BinaryExpr>(
            binary_op, std::move(expr), std::move(right), op.location);
    }
    return expr;
}


std::unique_ptr<Expr> Parser::parse_multiplicative() {
    std::unique_ptr<Expr> expr = parse_unary();
    while (match_any({TokenType::STAR, TokenType::SLASH, TokenType::PERCENT})) {
        const Token& op = previous();
        BinaryOp binary_op;
        switch (op.type) {
            case TokenType::STAR:    binary_op = BinaryOp::Mul; break;
            case TokenType::SLASH:   binary_op = BinaryOp::Div; break;
            case TokenType::PERCENT: binary_op = BinaryOp::Mod; break;
            default: throw ParseError("Invalid multiplicative operator", op.location);
        }
        std::unique_ptr<Expr> right = parse_unary();
        expr = std::make_unique<BinaryExpr>(
            binary_op, std::move(expr), std::move(right), op.location);
    }
    return expr;
}


std::unique_ptr<Expr> Parser::parse_unary() {
    const Token& op = peek();
    if (match_any({TokenType::MINUS, TokenType::BANG})) {
        UnaryOp unary_op = (op.type == TokenType::MINUS) ? UnaryOp::Negate : UnaryOp::Not;
        std::unique_ptr<Expr> operand = parse_unary();
        return std::make_unique<UnaryExpr>(
            unary_op, std::move(operand), op.location);
    }
    return parse_primary();
}


std::unique_ptr<Expr> Parser::parse_primary() {
    const Token& token = peek();
    if (match(TokenType::INT_LITERAL)) {
        SourceLocation location = token.location;
        long long int value;
        try {
            value = std::stoll(token.lexeme);
        }
        catch(const std::exception& e) {
            ParseError("integer literal out of range", token.location);
        }
        
        return std::make_unique<IntLiteralExpr>(std::move(value), location);
    }
    
    if (match(TokenType::STR_LITERAL)) {
        SourceLocation location = token.location;
        std::string value = token.lexeme;
        return std::make_unique<StrLiteralExpr>(std::move(value), location);
    }

    if (match(TokenType::KW_TRUE)) {
        SourceLocation location = token.location;
        bool value = true;
        return std::make_unique<BoolLiteralExpr>(value, location);
    }

    if (match(TokenType::KW_FALSE)) {
        SourceLocation location = token.location;
        bool value = false;
        return std::make_unique<BoolLiteralExpr>(value, location);
    }

    if (check(TokenType::IDENT)) {
        const Token& ident_token = advance();

        if (match(TokenType::LPAREN)) {
            SourceLocation location = ident_token.location;
            std::string callee = ident_token.lexeme;
            std::vector<std::unique_ptr<Expr>> arguments;
            if (!check(TokenType::RPAREN)) {
                do {
                    arguments.push_back(parse_required_expr("expected expression in argument list"));
                } while (match(TokenType::COMMA));
            }
            expect(TokenType::RPAREN, "expected ')' after arguments");
            return std::make_unique<CallExpr>(
                std::move(callee), std::move(arguments), location);
        }

        SourceLocation location = ident_token.location;
        std::string name = ident_token.lexeme;
        return std::make_unique<IdentifierExpr>(std::move(name), location);
    }

    if (match(TokenType::LPAREN)) {
        std::unique_ptr<Expr> expr = parse_required_expr("expected expression after '('");
        expect(TokenType::RPAREN, "expected ')' after expression");
        return expr;
    }

    throw ParseError("expected expression", token.location);
}
