#pragma once

#include <cassert>
#include <cstdint>
#include <string>


struct ExprId {
    usize id = SIZE_MAX;

    bool valid() const {
        return id != SIZE_MAX;
    }
};

struct StmtId {
    usize id = SIZE_MAX;

    bool valid() const {
        return id != SIZE_MAX;
    }
};

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
    ExprId left;
    ExprId right;
};

struct ParseError {
    std::string inner;
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

        struct { ExprId expr; } expr;
        struct { Token name; ExprId expr; } decl;
        struct { ExprId target; ExprId expr; } assign;
        struct { ExprId expr; } ret_stmt;
        struct { ExprId guard; StmtId then_branch; StmtId else_branch; } if_stmt;
        struct { ExprId guard; StmtId then_branch; } while_stmt;
        struct { StmtId first; usize count; } block;

        explicit StmtData() {
            expr.expr = ExprId();
        }

        ~StmtData() { }
    } as;

    explicit Stmt(StmtKind k, Token t) : kind(k), token(t) {

    }
};

struct Ast {
    std::vector<Expr> exprs;
    std::vector<Stmt> stmts;

    ExprId add(const Expr& e) {
        exprs.push_back(e);
        return {exprs.size() - 1};
    }

    StmtId add(const Stmt& s) {
        stmts.push_back(s);
        return {stmts.size() - 1};
    }

    const Expr& operator[](ExprId id) const {
        return exprs[id.id];
    }

    const Stmt& operator[](StmtId id) const {
        return stmts[id.id];
    }
};

struct Parser {
    Ast ast;
    const std::vector<Token>& tokens;
    std::vector<ParseError> errors;
    usize pos;
};

std::vector<StmtId> parse_program(Parser& p, std::vector<ParseError> errors);
void free_parser_pool();

std::string dump_expr(const Expr& expr);
std::string dump_stmt(const Stmt& s);

