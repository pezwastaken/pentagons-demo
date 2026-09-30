#pragma once

#include "parser.hh"
#include <iostream>


enum CFGStmtKind {
    DECLARATION,
    ASSIGNMENT,
    EXPR,
    NOP,
};

struct CFGStmt {

    union CFGStmtData {

        struct { std::string name; Expr* rhs;} decl;
        struct { Expr* lhs; Expr* rhs;} assign;
        struct { Expr* expr; } expr;

        explicit CFGStmtData() {
            expr.expr = nullptr;
        }

        ~CFGStmtData() { }

    } data;

    CFGStmtKind kind;

    explicit CFGStmt(CFGStmtKind k) : kind(k) {

    }

    explicit CFGStmt() {
        kind = NOP;
    }

};

struct Node {
    CFGStmt stmt;

    Node* true_edge;
    Node* false_edge;
};

inline Node* make_assign_node(Stmt& s) {
    Node* node = new Node;
    node->stmt.kind = ASSIGNMENT;
    node->stmt.data.assign.lhs = s.as.assign.target;
    node->stmt.data.assign.rhs = s.as.assign.expr;
    node->true_edge = nullptr;
    node->false_edge = nullptr;
    return node;
}

inline Node* make_decl_node(Stmt& s) {
    Node* node = new Node;
    node->stmt.kind = DECLARATION;
    node->stmt.data.decl.name = s.as.decl.name.lexeme;
    node->stmt.data.decl.rhs = s.as.decl.expr;
    node->true_edge = nullptr;
    node->false_edge = nullptr;
    return node;
}

inline Node* make_expr_node(Stmt& s) {
    Node* node = new Node;
    node->stmt.kind = EXPR;
    node->stmt.data.expr.expr = s.as.expr.expr;
    node->true_edge = nullptr;
    node->false_edge = nullptr;
    return node;
}

inline Node* make_nop() {
    Node* node = new Node;
    node->stmt.kind = NOP;
    node->true_edge = nullptr;
    node->false_edge = nullptr;
    return node;
}

inline Node* get_cfg(std::vector<Stmt*>& program) {

    std::array<Node*, 2> last_nodes = {nullptr, nullptr};

    Node* root = make_nop();
    Node* prev = root;

    for (auto s : program) {

        std::cout << "s kind: " << s->kind << std::endl;

        switch (s->kind) {

        case ASSIGN_STMT: last_nodes[0] = make_assign_node(*s); break;
        case EXPR_STMT: last_nodes[0] = make_expr_node(*s); break;
        case DECL_STMT: last_nodes[0] = make_decl_node(*s); break;
        case IF_STMT:
        case WHILE_STMT:
        case BLOCK_STMT:
        default:
            UNREACHABLE;
        }

        prev->true_edge = last_nodes[0];
        prev = prev->true_edge;
    }

    assert (root->true_edge != nullptr);
    return root->true_edge;
}


inline void debug_cfg(Node* root) {

    while (root != nullptr) {
        std::cout << root->stmt.kind << std::endl;
        root = root->true_edge;
    }
}

