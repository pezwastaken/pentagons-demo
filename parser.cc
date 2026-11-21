#include <iostream>
#include <optional>

struct OpInfo {
    i32 prec;
    bool right_assoc;
};

static inline std::optional<UnaryOperatorKind> get_un_op_kind(TokenType t) {
    switch(t) {
    case MINUS: return OP_UNARY_NEG;
    case BANG: return OP_UNARY_LOG_NEG;
    default:
        UNREACHABLE;
        return std::nullopt;
    }
}

static inline std::optional<BinaryOperatorKind> get_bin_op_kind(TokenType t) {
    switch(t) {
    case PLUS:
        return OP_BINARY_ADD;
    case MINUS:
        return OP_BINARY_DIFF;
    case SLASH:
        return OP_BINARY_DIV;
    case STAR:
        return OP_BINARY_MUL;
    case BANG_EQUAL:
        return OP_BINARY_NE;
    case EQUAL_EQUAL:
        return OP_BINARY_EQ;
    case GREATER:
        return OP_BINARY_GT;
    case GREATER_EQUAL:
        return OP_BINARY_GE;
    case LESS:
        return OP_BINARY_LT;
    case LESS_EQUAL:
        return OP_BINARY_LE;
    case AND:
        return OP_BINARY_LOG_AND;
    case OR:
        return OP_BINARY_LOG_OR;
    default:
        UNREACHABLE;
        return std::nullopt;
    }
}

static inline OpInfo binary_op_info(TokenType t) {

    switch (t) {
    case OR: return OpInfo {2, false};
    case AND: return OpInfo {3, false};
    case BANG_EQUAL:
    case EQUAL_EQUAL: return OpInfo {4, false};
    case GREATER:
    case GREATER_EQUAL:
    case LESS:
    case LESS_EQUAL: return OpInfo {5, false};
    case PLUS:
    case MINUS: return OpInfo {6, false};
    case SLASH:
    case STAR: return OpInfo {7, false};
    default: return OpInfo {-1, false};
    }
}


static inline Token peek(const Parser& parser) {
    return parser.tokens[parser.pos];
}

static inline Token previous(const Parser& parser) {
    return parser.tokens[parser.pos - 1];
}

static inline bool more(const Parser& parser) {
    return peek(parser).type != END_OF_FILE;
}

static inline Token advance(Parser& parser) {
    if (more(parser)) ++parser.pos;
    return previous(parser);
}

static inline bool check_eq(const Parser& parser, TokenType t) {
    return peek(parser).type == t;
}

static inline bool match(Parser& parser, TokenType t) {
    if (!check_eq(parser, t)) return false;
    advance(parser);
    return true;
}

static inline std::optional<Token> expect(Parser& p, TokenType t) {
    if (check_eq(p, t)) return advance(p);
    return std::nullopt;
}

static inline ExprId parse_expr(Parser& parser, i32 min_prec = 1);

static inline ExprId make_literal (Parser& parser, Token t) {
    // Expr* e = arena_new<Expr>(parser.arena);
    Expr e {};
    e.kind = LITERAL_EXPR;
    e.token = t;
    e.type = t.literal.type;
    e.left = ExprId();
    e.right = ExprId();

    return parser.ast.add(e);
}

static inline ExprId make_variable (Parser& parser, Token t) {
    Expr e {};
    e.kind = VARIABLE_EXPR;
    e.token = t;
    e.type = INT;
    e.left = ExprId();
    e.right = ExprId();

    return parser.ast.add(e);
}

static inline ExprId make_binary (Parser& parser, Token op_token, BinaryOperatorKind op, ExprId left, ExprId right) {
    Expr e {};
    e.kind = BINARY_EXPR;
    e.token = op_token;
    e.bin_op = op;
    e.type = INT;
    e.left = ExprId();
    e.right = ExprId();

    return parser.ast.add(e);
}

static inline ExprId make_unary (Parser& parser, Token op_token, ExprId inner) {

    Expr e {};
    e.kind = UNARY_EXPR;
    e.token = op_token;
    e.un_op = get_un_op_kind(op_token.type).value();

    // TODO this should be populated by the typechecker OR
    // could even be done by the parser by simply looking at the operator
    // (- -> int, ! -> bool)
    e.type = INT;
    e.left = inner;
    e.right = ExprId();

    return parser.ast.add(e);
}

static inline ExprId parse_input(Parser& parser) {

    Token kw = advance(parser);
    if (!match(parser, TokenType::LEFT_PAREN)) {
        parser.errors.push_back({.inner = "expected `(` after `input`"});
        return ExprId {};
    }

    ExprId left = parse_expr(parser);
    assert(left.valid());
    if (!match(parser, TokenType::COMMA)) {
        parser.errors.push_back({.inner = "expected `input(lo, hi)`"});
        return ExprId {};
    }

    ExprId right = parse_expr(parser);
    assert(right.valid());

    if (!match(parser, TokenType::RIGHT_PAREN)) {
        parser.errors.push_back({.inner = "expected `)` after `input`"});
        return ExprId {};
    }

    Expr e {};
    e.kind = INPUT_EXPR;
    e.token = kw;
    e.type = INT;
    e.left = left;
    e.right = right;

    return parser.ast.add(e);
}

static inline ExprId parse_atom(Parser& parser) {

    switch (peek(parser).type) {
    case NUMBER:
    case FALSE:
    case TRUE: return make_literal(parser, advance(parser));
    case IDENTIFIER: return make_variable(parser, advance(parser));
    case INPUT: return parse_input(parser);
    default:
        return ExprId{};
    }
}

static inline ExprId parse_unary(Parser& parser) {

    switch (peek(parser).type) {

    case MINUS:
    case BANG:
        {
            Token op_token = advance(parser);
            ExprId inner = parse_expr(parser);
            return make_unary(parser, op_token, inner);
        }
    case LEFT_PAREN:
        {
            Token parens = advance(parser);
            ExprId inner = parse_expr(parser);
            ExprId res = make_unary(parser, parens, inner);
            if (!match(parser, TokenType::RIGHT_PAREN)) {
                // TODO:
                std::cout << "missing right paren" << std::endl;
            }
            return res;
        }
    case IDENTIFIER:
    case NUMBER:
    case FALSE:
    case INPUT:
    case TRUE: return parse_atom(parser);
    default:
        return ExprId {};
    }

}

// static inline Expr* parse_function_call(Parser& parser, Expr* fn_identifier) {
//
//     Expr* e = arena_new<Expr>(parser.arena);
//     e->kind = ExprKind::FN_CALL;
//     e->token = fn_identifier->token;
//     e->type = INT;
//
//     // Expr* args = arena_new_array<Expr>(parser.arena, size_t len)
//
//     assert(e->args.size() == 0);
//
//     std::cout << "parsing function call: peek: " << peek(parser).lexeme << std::endl;
//
//     // populate the args of the func call
//     while (!match(parser, TokenType::RIGHT_PAREN)) {
//
//         std::cout << "parsing function call: peek: " << peek(parser).lexeme << std::endl;
//
//         Expr* arg = parse_expr(parser);
//         e->args.push_back(arg);
//
//         if (match(parser, TokenType::RIGHT_PAREN)) {
//             break;
//         }
//         if (!match(parser, TokenType::COMMA)) {
//             parser.errors.push_back(
//                 ParseError {.inner = "each argument should be followed by a `,`"}
//             );
//             return nullptr;
//         }
//     }
//
//     std::cout << "parsed function call to identifier: " << e->token.lexeme
//         << " with " << e->args.size() << " arguments" << std::endl;
//     return e;
// }

static inline ExprId parse_expr(Parser& parser, i32 min_prec) {

    ExprId left = parse_unary(parser);

    while (true) {
        OpInfo info = binary_op_info(peek(parser).type);

        if (info.prec == -1 || info.prec < min_prec)
            break;

        Token op_token = advance(parser);

        int next_min = info.right_assoc ? info.prec : info.prec + 1;
        ExprId right = parse_expr(parser, next_min);

        BinaryOperatorKind op = get_bin_op_kind(op_token.type).value();
        left = make_binary(parser, op_token, op, left, right);
    }

    return left;

}

std::string dump_expr(const Ast& ast, ExprId expr_id) {

    const Expr& expr = ast[expr_id];

    switch (expr.kind) {
    case LITERAL_EXPR:
        return std::string(expr.token.lexeme);
    case VARIABLE_EXPR:
        return std::string(expr.token.lexeme);
    case UNARY_EXPR:
        return " ( " + std::string(expr.token.lexeme) + dump_expr(ast, expr.left) + " ) ";
    case BINARY_EXPR:
        return " ( " + std::string(expr.token.lexeme) + " " + dump_expr(ast, expr.left) + ", " +
            dump_expr(ast, expr.right) + " ) ";
    case INPUT_EXPR:
        return " input (" + dump_expr(ast, expr.left) + ", " + dump_expr(ast, expr.right) + " ) ";
    default:
        UNREACHABLE;
    }
}

static inline StmtId parse_statement(Parser& p);

static inline StmtId parse_var_decl(Parser& parser) {

    Token type = advance(parser);
    assert(type.type == INTID);

    bool is_identifier = match(parser, IDENTIFIER);
    assert(is_identifier);
    if (!is_identifier)
        return StmtId {};

    Token name = previous(parser);

    ExprId init;
    if (match(parser, EQUAL)) init = parse_expr(parser);

    assert (init.valid());

    if (!match(parser, SEMICOLON)) return StmtId {};

    Stmt s = Stmt(DECL_STMT, type);
    s.as.decl.expr = init;
    s.as.decl.name = name;

    return parser.ast.add(s);
}

static inline StmtId parse_assignment_or_expr(Parser& parser) {

    ExprId lhs = parse_expr(parser);
    StmtId res {};

    if (match(parser, EQUAL)) {
        if (parser.ast[lhs].kind != VARIABLE_EXPR) {
            parser.errors.push_back({"assigning a value to an expression that is not an identifier"});
            return StmtId {};
        }


        ExprId init = parse_expr(parser);

        Stmt s = Stmt(ASSIGN_STMT, parser.ast[lhs].token);
        s.as.assign.target = lhs;
        s.as.assign.expr = init;
        res = parser.ast.add(s);
    }
    else {
        Stmt s = Stmt(EXPR_STMT, parser.ast[lhs].token);
        s.as.expr.expr = lhs;
        res = parser.ast.add(s);
    }

    if (!match(parser, SEMICOLON)) return StmtId {};

    return res;
}

static inline StmtId parse_if(Parser& p) {

    Token kw = advance(p);
    assert(kw.type == IF);
    if (!match(p, LEFT_PAREN)) p.errors.push_back({"expected left paren after if"});

    ExprId guard = parse_expr(p);

    if (!match(p, RIGHT_PAREN)) p.errors.push_back({"expected right paren after guard"});

    StmtId then_body = parse_statement(p);
    StmtId else_body {};

    if (match(p, ELSE)) else_body = parse_statement(p);

    Stmt if_stmt (IF_STMT, kw);
    if_stmt.kind = IF_STMT;
    if_stmt.token = kw;
    if_stmt.as.if_stmt.guard = guard;
    if_stmt.as.if_stmt.then_branch = then_body;
    if_stmt.as.if_stmt.else_branch = else_body;

    return p.ast.add(if_stmt);
}

static inline StmtId parse_assert(Parser& p) {

    Token kw = advance(p);
    assert(kw.type == ASSERT);
    if (!match(p, LEFT_PAREN)) p.errors.push_back({"expected left paren after assert"});

    ExprId guard = parse_expr(p);

    if (!match(p, RIGHT_PAREN)) p.errors.push_back({"expected right paren after assert expr"});

    Stmt assert_stmt (ASSERT_STMT, kw);
    assert_stmt.as.expr.expr = guard;

    if (!match(p, SEMICOLON)) return StmtId {};

    return p.ast.add(assert_stmt);
}

static inline StmtId parse_while(Parser& p) {

    Token kw = advance(p);
    assert(kw.type == WHILE);
    if (!match(p, LEFT_PAREN)) p.errors.push_back({"expected left paren after while"});

    ExprId guard = parse_expr(p);

    if (!match(p, RIGHT_PAREN)) p.errors.push_back({"expected right paren after guard"});

    StmtId then_body = parse_statement(p);

    Stmt while_stmt (WHILE_STMT, kw);
    while_stmt.as.while_stmt.guard = guard;
    while_stmt.as.while_stmt.then_branch = then_body;

    return p.ast.add(while_stmt);
}

static inline StmtId parse_block(Parser& p) {

    Token kw = advance(p);
    assert(kw.type == LEFT_BRACE);

    Stmt block (BLOCK_STMT, kw);
    StmtId block_id = p.ast.add(block);

    const usize first = p.ast.stmts.size();
    assert(block_id.id + 1U == first);

    StmtId last {};
    while (!check_eq(p, RIGHT_BRACE) && more(p)) {
        last = parse_statement(p);
    }

    const usize count = last.id - first + 1;

    if (!match(p, RIGHT_BRACE)) p.errors.push_back({"expected right brace closing block"});

    p.ast.stmts[block_id.id].as.block.first = StmtId {.id = first};
    p.ast.stmts[block_id.id].as.block.count = count;

    std::cout << "parsing block: first is " << p.ast.stmts[block_id.id].as.block.first.id << " and count is "
        << p.ast.stmts[block_id.id].as.block.count << std::endl;

    return block_id;
}

static inline StmtId parse_return(Parser& p) {

    Token kw = advance(p);
    assert(kw.type == RETURN);

    ExprId expr = parse_expr(p);

    if (!match(p, SEMICOLON)) p.errors.push_back({"expected semicolon after return"});

    Stmt ret_stmt (RET_STMT, kw);
    ret_stmt.as.ret_stmt.expr = expr;

    return p.ast.add(ret_stmt);
}

static inline StmtId parse_statement(Parser& p) {

    switch (peek(p).type) {
        case INTID:
            return parse_var_decl(p);
        case IF:
            return parse_if(p);
        case WHILE:
            return parse_while(p);
        case ASSERT:
            return parse_assert(p);
        case LEFT_BRACE:
            return parse_block(p);
        case RETURN:
            return parse_return(p);
        default:
            return parse_assignment_or_expr(p);
    }
}


std::vector<StmtId> parse_program(Parser& p, std::vector<ParseError> errors) {

    std::vector<StmtId> res;

    while (more(p)) {
        StmtId s = parse_statement(p);
        res.push_back(s);
    }

    return res;
}

static std::string with_indent(u32 c) {
    std::string res = "";
    for (u32 i = 0; i < c; ++i) {
        res += "\t";
    }
    return res;
}

std::string dump_stmt(const Ast& ast, StmtId s_id) {

    static u32 indent_count = 0U;

    assert(s_id.valid());
    const Stmt& s = ast[s_id];

    switch (s.kind) {
    case DECL_STMT:
        return std::string(s.token.lexeme) + " " + std::string(s.as.decl.name.lexeme) + " = " + (dump_expr(ast, s.as.decl.expr));
    case ASSIGN_STMT:
        return std::string(ast[s.as.assign.target].token.lexeme) + " = " + (dump_expr(ast, s.as.assign.expr));
    case EXPR_STMT:
        return (dump_expr(ast, s.as.expr.expr));
    case ASSERT_STMT:
        return ("assert (" + dump_expr(ast, s.as.expr.expr) + ")");
    case RET_STMT:
        return ("return " + dump_expr(ast, s.as.ret_stmt.expr));
    case IF_STMT: {
        ++indent_count;
        auto res = "if " + dump_expr(ast, s.as.if_stmt.guard)
            + dump_stmt(ast, s.as.if_stmt.then_branch) + " else " +
            (s.as.if_stmt.else_branch.valid() ? dump_stmt(ast, s.as.if_stmt.else_branch) : "");
        --indent_count;
        return res;
    }
    case WHILE_STMT: {
        ++indent_count;
        auto res = "while " + dump_expr(ast, s.as.while_stmt.guard)
            + dump_stmt(ast, s.as.while_stmt.then_branch);
        --indent_count;
        return res;
    }
    case BLOCK_STMT:
        {
            std::string res = "{\n";
            StmtId first = s.as.block.first;

            ++indent_count;

            for (usize i = first.id; i < first.id + s.as.block.count; ++i) {
                std::cout << "accessing element " << i << std::endl;
                res += with_indent(indent_count) + dump_stmt(ast, StmtId {.id = i}) + "\n";
            }
            return res + "\n" + with_indent(--indent_count) + "}\n";
        }
    default:
        UNREACHABLE;
    }
}

