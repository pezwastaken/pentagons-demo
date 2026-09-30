#include "lexer.hh"
#include <cassert>
#include <optional>
#include <string>

#define UNREACHABLE (assert(false))

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

enum ExprKind {
    LITERAL_EXPR,
    VARIABLE_EXPR,
    UNARY_EXPR,
    BINARY_EXPR,
};


struct Expr {

    ExprKind kind;
    Token token;
    StaticType type;

    Expr* left;
    Expr* right;

};



struct ParseError {
    std::string inner;
};


struct Parser {
    const std::vector<Token>& tokens;
    std::vector<ParseError> errors;
    usize pos;
};

static inline Token peek(const Parser& parser) {
    // std::cout << "peeking at pos" << parser.pos << "/" << parser.tokens.size() << std::endl;
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

static inline Expr* make_literal (Token t) {
    Expr* e = new Expr;
    e->kind = LITERAL_EXPR,
    e-> token = t;
    e->type = t.literal.type;
    e->left = nullptr;
    e->right = nullptr;

    std::cout << "making literal" << std::endl;
    return e;
}

static inline Expr* make_variable (Token t) {
    Expr* e = new Expr;
    e->kind = VARIABLE_EXPR,
    e-> token = t;
    e->type = INT;
    e->left = nullptr;
    e->right = nullptr;

    std::cout << "making variable" << std::endl;
    return e;
}

static inline Expr* make_binary (Token op, Expr* left, Expr* right) {
    Expr* e = new Expr;
    e->kind = BINARY_EXPR,
    e-> token = op;
    e->type = INT;
    e->left = left;
    e->right = right;

    std::cout << "making binary" << std::endl;
    if (left == nullptr) std::cout << "\tleft is null" << std::endl;
    if (right == nullptr) std::cout << "\tright is null" << std::endl;
    return e;
}

static inline Expr* parse_atom(Parser& parser) {

    switch (peek(parser).type) {

    case NUMBER:
    case FALSE:
    case TRUE: return make_literal(advance(parser));
    case LEFT_PAREN: UNREACHABLE; // TODO
    case IDENTIFIER: return make_variable(advance(parser));
    default:
        return nullptr;
    }
}

static inline Expr* parse_unary(Parser& parser) {

    // TODO: parse unary expressions like ! and `-`

    return parse_atom(parser);

}

inline Expr* parse_expr(Parser& parser, i32 min_prec = 1) {

    Expr* left = parse_unary(parser);

    std::cout << "parsed unary (parse_expr)" << std::endl;
    if (left == nullptr) std::cout << "\tit's null" << std::endl;

    while (true) {
        OpInfo info = binary_op_info(peek(parser).type);

        if (info.prec == -1 || info.prec < min_prec)
            break;

        Token op = advance(parser);

        int next_min = info.right_assoc ? info.prec : info.prec + 1;
        Expr* right = parse_expr(parser, next_min);

        left = make_binary(op, left, right);
    }

    return left;

}

inline std::string dump_expr(const Expr& expr) {
    switch (expr.kind) {
    case LITERAL_EXPR:
        return expr.token.lexeme;
    case VARIABLE_EXPR:
        return expr.token.lexeme;
    case UNARY_EXPR:
        return " ( " + expr.token.lexeme + dump_expr(*expr.left) + " ) ";
    case BINARY_EXPR:
        return " ( " + expr.token.lexeme + " " + dump_expr(*expr.left) + ", " +
            dump_expr(*expr.right) + " ) ";
    default:
        UNREACHABLE;
    }
}

enum StmtKind {
    EXPR_STMT,
    ASSIGN_STMT,
    DECL_STMT,
    IF_STMT,
    WHILE_STMT,
    BLOCK_STMT,
};

struct Stmt {

    StmtKind kind;
    Token token;

    union StmtData {

        struct { Expr* expr; } expr;
        struct { Token name; Expr* expr; } decl;
        struct { Expr* target; Expr* expr; } assign;
        struct { Expr* guard; Stmt* then_branch; Stmt* else_branch; } if_stmt;
        struct { Expr* guard; Stmt* then_branch; } while_stmt;
        struct { std::vector<Stmt*> stmts; } block;

        explicit StmtData() {
            expr.expr = nullptr;
        }

        ~StmtData() { }
    } as;

    explicit Stmt(StmtKind k, Token t) : kind(k), token(t) {
        // kind = k;
        // token = t;
    }

};

inline Stmt* parse_statement(Parser& p);

inline Stmt* parse_var_decl(Parser& parser) {

    std::cout << "parsing var decl" << std::endl;

    Token type = advance(parser);
    assert(type.type == INTID);

    bool is_identifier = match(parser, IDENTIFIER);
    assert(is_identifier);
    if (!is_identifier)
        return nullptr;

    Token name = previous(parser);
    std::cout << "var name is: " << name.lexeme << std::endl;

    Expr* init;
    if (match(parser, EQUAL)) init = parse_expr(parser);

    if (!match(parser, SEMICOLON)) return nullptr;

    Stmt* s = new Stmt(DECL_STMT, type);
    s->as.decl.expr = init;
    s->as.decl.name = name;

    return s;
}

inline Stmt* parse_assignment_or_expr(Parser& parser) {

    std::cout << "parsing assignment or expr" << std::endl;

    Expr* lhs = parse_expr(parser);
    Stmt* s = nullptr;

    if (match(parser, EQUAL)) {
        std::cout << "has an equal" << std::endl;
        if (lhs->kind != VARIABLE_EXPR) {
            parser.errors.push_back({"assigning a value to an expression that is not an identifier"});
            return nullptr;
        }

        Expr* init = parse_expr(parser);

        s = new Stmt(ASSIGN_STMT, lhs->token);
        s->as.assign.target = lhs;
        s->as.assign.expr = init;
    }
    else {
        s = new Stmt(EXPR_STMT, lhs->token);
        s->as.expr.expr = lhs;
    }

    if (!match(parser, SEMICOLON)) return nullptr;

    return s;
}

inline Stmt* parse_if(Parser& p) {

    std::cout << "parsing if" << std::endl;
    Token kw = advance(p);
    assert(kw.type == IF);
    if (!match(p, LEFT_PAREN)) p.errors.push_back({"expected left paren after if"});

    Expr* guard = parse_expr(p);

    if (!match(p, RIGHT_PAREN)) p.errors.push_back({"expected right paren after guard"});

    Stmt* then_body = parse_statement(p);
    Stmt* else_body = nullptr;

    if (match(p, ELSE)) else_body = parse_statement(p);

    Stmt* if_stmt = new Stmt(IF_STMT, kw);
    if_stmt->as.if_stmt.guard = guard;
    if_stmt->as.if_stmt.then_branch = then_body;
    if_stmt->as.if_stmt.else_branch = else_body;

    return if_stmt;
}

inline Stmt* parse_while(Parser& p) {

    Token kw = advance(p);
    assert(kw.type == WHILE);
    if (!match(p, LEFT_PAREN)) p.errors.push_back({"expected left paren after while"});

    Expr* guard = parse_expr(p);

    if (!match(p, RIGHT_PAREN)) p.errors.push_back({"expected right paren after guard"});

    Stmt* then_body = parse_statement(p);

    Stmt* while_stmt = new Stmt(WHILE_STMT, kw);
    while_stmt->as.while_stmt.guard = guard;
    while_stmt->as.if_stmt.then_branch = then_body;

    return while_stmt;
}

inline Stmt* parse_block(Parser& p) {

    std::cout << "parsing block" << std::endl;

    Token kw = advance(p);
    assert(kw.type == LEFT_BRACE);

    std::vector<Stmt*> res;

    while (!check_eq(p, RIGHT_BRACE) && more(p)) {
        res.push_back(parse_statement(p));
        std::cout << "\tparsed a statement inside the block" << std::endl;
    }

    if (!match(p, RIGHT_BRACE)) p.errors.push_back({"expected right brace closing block"});

    std::cout << "found a closing brace" << std::endl;

    Stmt* block = new Stmt(BLOCK_STMT, kw);
    block->as.block.stmts = res;

    return block;
}

inline Stmt* parse_statement(Parser& p) {

    std::cout << "stmt type: " << peek(p).type << std::endl;

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

inline std::vector<Stmt*> parse_program(Parser& p, std::vector<ParseError> errors) {

    std::vector<Stmt*> res;

    while (more(p)) {
        Stmt* s = parse_statement(p);
        res.push_back(s);

        if (s)
            std::cout << "pushed a non null statement" << std::endl;
        std::cout << std::endl;
    }

    return res;
}


inline std::string dump_stmt(const Stmt& s) {
    switch (s.kind) {
    case DECL_STMT:
        return s.token.lexeme + " " + s.as.decl.name.lexeme + " = " + (dump_expr(*s.as.decl.expr));
    case ASSIGN_STMT:
        return s.as.assign.target->token.lexeme + " = " + (dump_expr(*s.as.assign.expr));
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

