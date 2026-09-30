#include "lexer.hh"
#include <cassert>

#define UNREACHABLE (assert(false))

struct OpInfo {
    i32 prec;
    bool right_assoc;
};

static inline OpInfo binary_op_info(TokenType t) {

    switch (t) {
    case EQUAL: return OpInfo {1, false};
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

struct Parser {
    const std::vector<Token>& tokens;
    usize pos;
};

static inline Token peek(const Parser& parser) {
    std::cout << "peeking at pos" << parser.pos << "/" << parser.tokens.size() << std::endl;
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
        return " ( " + expr.token.lexeme + dump_expr(*expr.left) +
            dump_expr(*expr.right) + " ) ";
    default:
        UNREACHABLE;
    }
}


