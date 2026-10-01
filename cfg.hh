#pragma once

#include "parser.hh"

struct SyntheticPP {
    u32 line;
};

enum CFGStmtKind {
    DECLARATION,
    ASSIGNMENT,
    EXPR,
    NOP,
};

struct CFGStmt {

    Token name; // declaration
    Expr* lhs; // assignments
    Expr* rhs; // assignments/declarations

    Expr* expr; // expressions and guards

    CFGStmtKind kind;

    explicit CFGStmt(CFGStmtKind k = NOP) : kind(k) {

    }
};

struct Node {
    CFGStmt stmt;

    Node* true_edge = nullptr;
    Node* false_edge = nullptr;

    SyntheticPP pp;
};

Node* get_cfg(std::vector<Stmt*>& program);

void debug_cfg(Node* root);

bool write_cfg_dot(const Node* entry, const char* path);
