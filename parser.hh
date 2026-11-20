#pragma once

#include <cassert>
#include <string>

enum ExprKind {
    LITERAL_EXPR,
    VARIABLE_EXPR,
    UNARY_EXPR,
    BINARY_EXPR,
    FN_CALL,
    INPUT_EXPR,
};

enum UnaryOperatorKind {
    OP_UNARY_NEG,
    OP_UNARY_LOG_NEG,
    OP_UNARY_ASSERT,
    OP_UNARY_INPUT_INT,
    OP_UNARY_INPUT_STRING,
};

enum BinaryOperatorKind {
    OP_BINARY_ADD,
    OP_BINARY_DIFF,
    OP_BINARY_MUL,
    OP_BINARY_DIV,
    OP_BINARY_EQ,
    OP_BINARY_NE,
    OP_BINARY_LT,
    OP_BINARY_LE,
    OP_BINARY_GT,
    OP_BINARY_GE,
    OP_BINARY_LOG_AND,
    OP_BINARY_LOG_OR,
};

struct Expr {

    ExprKind kind;
    Token token;
    StaticType type;

    UnaryOperatorKind un_op;
    BinaryOperatorKind bin_op;
    Expr* left;
    Expr* right;

    // used by function calls, otherwise empty
    std::vector<Expr*> args;
};


struct ParseError {
    std::string inner;
};


struct Parser {
    const std::vector<Token>& tokens;
    std::vector<ParseError> errors;
    usize pos;
    Arena* arena;
};

enum StmtKind {
    EXPR_STMT,
    ASSERT_STMT,
    ASSIGN_STMT,
    DECL_STMT,
    RET_STMT,
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
        struct { Expr* expr; } ret_stmt;
        struct { Expr* guard; Stmt* then_branch; Stmt* else_branch; } if_stmt;
        struct { Expr* guard; Stmt* then_branch; } while_stmt;
        struct { std::vector<Stmt*> stmts; } block;

        explicit StmtData() {
            expr.expr = nullptr;
        }

        ~StmtData() { }
    } as;

    explicit Stmt(StmtKind k, Token t) : kind(k), token(t) {

    }

};

std::vector<Stmt*> parse_program(Parser& p, std::vector<ParseError> errors);

std::string dump_expr(const Expr& expr);
std::string dump_stmt(const Stmt& s);

