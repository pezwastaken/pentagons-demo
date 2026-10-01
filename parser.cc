#include <iostream>
#include "parser.hh"

struct OpInfo {
    i32 prec;
    bool right_assoc;
};

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

static inline Expr* make_literal (Parser& parser, Token t) {
    Expr* e = arena_new<Expr>(parser.arena);
    e->kind = LITERAL_EXPR,
    e-> token = t;
    e->type = t.literal.type;
    e->left = nullptr;
    e->right = nullptr;

    return e;
}

static inline Expr* make_variable (Parser& parser, Token t) {
    Expr* e = arena_new<Expr>(parser.arena);
    e->kind = VARIABLE_EXPR,
    e-> token = t;
    e->type = INT;
    e->left = nullptr;
    e->right = nullptr;

    return e;
}

static inline Expr* make_binary (Parser& parser, Token op, Expr* left, Expr* right) {
    Expr* e = arena_new<Expr>(parser.arena);
    e->kind = BINARY_EXPR,
    e-> token = op;
    e->type = INT;
    e->left = left;
    e->right = right;

    return e;
}

static inline Expr* parse_atom(Parser& parser) {

    switch (peek(parser).type) {

    case NUMBER:
    case FALSE:
    case TRUE: return make_literal(parser, advance(parser));
    case LEFT_PAREN: UNREACHABLE; // TODO
    case IDENTIFIER: return make_variable(parser, advance(parser));
    default:
        return nullptr;
    }
}

static inline Expr* parse_unary(Parser& parser) {

    // TODO: parse unary expressions like ! and `-`

    return parse_atom(parser);

}

static inline Expr* parse_expr(Parser& parser, i32 min_prec = 1) {

    Expr* left = parse_unary(parser);

    if (left == nullptr) std::cout << "\tit's null" << std::endl;

    while (true) {
        OpInfo info = binary_op_info(peek(parser).type);

        if (info.prec == -1 || info.prec < min_prec)
            break;

        Token op = advance(parser);

        int next_min = info.right_assoc ? info.prec : info.prec + 1;
        Expr* right = parse_expr(parser, next_min);

        left = make_binary(parser, op, left, right);
    }

    return left;

}

std::string dump_expr(const Expr& expr) {
    switch (expr.kind) {
    case LITERAL_EXPR:
        return std::string(expr.token.lexeme);
    case VARIABLE_EXPR:
        return std::string(expr.token.lexeme);
    case UNARY_EXPR:
        return " ( " + std::string(expr.token.lexeme) + dump_expr(*expr.left) + " ) ";
    case BINARY_EXPR:
        return " ( " + std::string(expr.token.lexeme) + " " + dump_expr(*expr.left) + ", " +
            dump_expr(*expr.right) + " ) ";
    default:
        UNREACHABLE;
    }
}

static inline Stmt* parse_statement(Parser& p);

static inline Stmt* parse_var_decl(Parser& parser) {

    Token type = advance(parser);
    assert(type.type == INTID);

    bool is_identifier = match(parser, IDENTIFIER);
    assert(is_identifier);
    if (!is_identifier)
        return nullptr;

    Token name = previous(parser);

    Expr* init;
    if (match(parser, EQUAL)) init = parse_expr(parser);

    if (!match(parser, SEMICOLON)) return nullptr;

    Stmt* s = arena_new<Stmt>(parser.arena);
    s->kind = DECL_STMT;
    s->token = type;
    s->as.decl.expr = init;
    s->as.decl.name = name;

    return s;
}

static inline Stmt* parse_assignment_or_expr(Parser& parser) {

    Expr* lhs = parse_expr(parser);
    Stmt* s = nullptr;

    if (match(parser, EQUAL)) {
        if (lhs->kind != VARIABLE_EXPR) {
            parser.errors.push_back({"assigning a value to an expression that is not an identifier"});
            return nullptr;
        }

        Expr* init = parse_expr(parser);

        s = arena_new<Stmt>(parser.arena);
        s->kind = ASSIGN_STMT;
        s->token = lhs->token;
        s->as.assign.target = lhs;
        s->as.assign.expr = init;
    }
    else {
        s = arena_new<Stmt>(parser.arena);
        s->kind = EXPR_STMT;
        s->token = lhs->token;
        s->as.expr.expr = lhs;
    }

    if (!match(parser, SEMICOLON)) return nullptr;

    return s;
}

static inline Stmt* parse_if(Parser& p) {

    Token kw = advance(p);
    assert(kw.type == IF);
    if (!match(p, LEFT_PAREN)) p.errors.push_back({"expected left paren after if"});

    Expr* guard = parse_expr(p);

    if (!match(p, RIGHT_PAREN)) p.errors.push_back({"expected right paren after guard"});

    Stmt* then_body = parse_statement(p);
    Stmt* else_body = nullptr;

    if (match(p, ELSE)) else_body = parse_statement(p);

    Stmt* if_stmt = arena_new<Stmt>(p.arena);
    if_stmt->kind = IF_STMT;
    if_stmt->token = kw;
    if_stmt->as.if_stmt.guard = guard;
    if_stmt->as.if_stmt.then_branch = then_body;
    if_stmt->as.if_stmt.else_branch = else_body;

    return if_stmt;
}

static inline Stmt* parse_while(Parser& p) {

    Token kw = advance(p);
    assert(kw.type == WHILE);
    if (!match(p, LEFT_PAREN)) p.errors.push_back({"expected left paren after while"});

    Expr* guard = parse_expr(p);

    if (!match(p, RIGHT_PAREN)) p.errors.push_back({"expected right paren after guard"});

    Stmt* then_body = parse_statement(p);

    Stmt* while_stmt = arena_new<Stmt>(p.arena);
    while_stmt->kind = WHILE_STMT;
    while_stmt->token = kw;
    while_stmt->as.while_stmt.guard = guard;
    while_stmt->as.if_stmt.then_branch = then_body;

    return while_stmt;
}

static inline Stmt* parse_block(Parser& p) {

    Token kw = advance(p);
    assert(kw.type == LEFT_BRACE);

    std::vector<Stmt*> res;

    while (!check_eq(p, RIGHT_BRACE) && more(p)) {
        res.push_back(parse_statement(p));
    }

    if (!match(p, RIGHT_BRACE)) p.errors.push_back({"expected right brace closing block"});

    Stmt* block = arena_new<Stmt>(p.arena);
    block->kind = BLOCK_STMT;
    block->token = kw;
    block->as.block.stmts = res;

    return block;
}

static inline Stmt* parse_statement(Parser& p) {

    switch (peek(p).type) {
        case INTID:
            return parse_var_decl(p);
        case IF:
            return parse_if(p);
        case WHILE:
            return parse_if(p);
        case LEFT_BRACE:
            return parse_block(p);
        default:
            return parse_assignment_or_expr(p);
    }
}


std::vector<Stmt*> parse_program(Parser& p, std::vector<ParseError> errors) {

    std::vector<Stmt*> res;

    while (more(p)) {
        Stmt* s = parse_statement(p);
        res.push_back(s);

        if (!s)
            std::cout << "pushed a null statement" << std::endl;
        std::cout << std::endl;
    }

    return res;
}

std::string dump_stmt(const Stmt& s) {
    switch (s.kind) {
    case DECL_STMT:
        return std::string(s.token.lexeme) + " " + std::string(s.as.decl.name.lexeme) + " = " + (dump_expr(*s.as.decl.expr));
    case ASSIGN_STMT:
        return std::string(s.as.assign.target->token.lexeme) + " = " + (dump_expr(*s.as.assign.expr));
    case EXPR_STMT:
        return (dump_expr(*s.as.expr.expr));
    case IF_STMT:
        return "if " + dump_expr(*s.as.if_stmt.guard)
            + dump_stmt(*s.as.if_stmt.then_branch) + " else " +
            (s.as.if_stmt.else_branch ? dump_stmt(*s.as.if_stmt.else_branch) : "");
    case BLOCK_STMT:
        {
            std::string res = "{\n";
            for (auto x : s.as.block.stmts) {
                res += dump_stmt(*x) + "\n";
            }
            return res + "\n}\n";
        }
    default:
        UNREACHABLE;
    }
}
