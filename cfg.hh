#pragma once

struct SyntheticPP {
    u32 line;
};

enum CFGStmtKind {
    CFG_DECLARATION,
    CFG_ASSIGNMENT,
    CFG_EXPR,
    CFG_EXPR_GUARD,
    CFG_RETURN,
    CFG_NOP,
};

struct CFGStmt {

    Token name; // declaration
    Expr* lhs; // assignments
    Expr* rhs; // assignments/declarations

    Expr* expr; // expressions and guards

    CFGStmtKind kind;

    explicit CFGStmt(CFGStmtKind k = CFG_NOP) : kind(k) {

    }
};

struct Node {
    CFGStmt stmt;

    Node* true_edge = nullptr;
    Node* false_edge = nullptr;

    SyntheticPP pp;

    usize id;

};

struct CFG {
    Node* root;
};

CFG get_cfg(std::vector<Stmt*>& program);

void debug_cfg(CFG cfg);

bool write_cfg_dot(CFG cfg, const char* path);
