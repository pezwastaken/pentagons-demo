#pragma once

#include "arena.hh"
#include "lexer.hh"
#include <cassert>
#include <string>


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
    Arena* arena;
};

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

    }

};

std::vector<Stmt*> parse_program(Parser& p, std::vector<ParseError> errors);

std::string dump_stmt(const Stmt& s);
