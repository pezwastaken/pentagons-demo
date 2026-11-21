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
    CFG_ASSERT,
    CFG_NOP,
};


struct CFGStmt {

    Token name; // declaration
    ExprId lhs; // assignments   / true edge of guard
    ExprId rhs; // assignments/declarations  / false edge of guard

    ExprId expr; // expressions and guards

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


void cfg_set_arena(Arena* arena);

CFG get_cfg(Program& program);

ExprId expr_negate(Ast& ast, const Expr& e);

bool write_cfg_dot(const Ast& ast, CFG cfg, const char* path);

std::string dump_cfg_as_text(CFG cfg);

