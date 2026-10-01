#include "cfg.hh"
#include "parser.hh"
#include <iostream>
#include <unordered_set>

struct ConstructionResult {
    Node* root;
    Node* out;
};

inline ConstructionResult make_assign_node(Stmt& s) {
    Node* node = new Node;
    node->stmt.kind = ASSIGNMENT;
    node->stmt.lhs = s.as.assign.target;
    node->stmt.rhs = s.as.assign.expr;
    node->true_edge = nullptr;
    node->false_edge = nullptr;

    ConstructionResult r = ConstructionResult {node, node};
    return r;
}

inline ConstructionResult make_decl_node(Stmt& s) {
    Node* node = new Node;
    node->stmt.kind = DECLARATION;
    node->stmt.name = s.as.decl.name;
    node->stmt.rhs = s.as.decl.expr;
    node->true_edge = nullptr;
    node->false_edge = nullptr;
    ConstructionResult r = ConstructionResult {node, node};
    return r;
}

inline ConstructionResult make_expr_node(Stmt& s) {
    Node* node = new Node;
    node->stmt.kind = EXPR;
    assert(s.as.expr.expr != nullptr);
    node->stmt.expr = s.as.expr.expr;
    node->true_edge = nullptr;
    node->false_edge = nullptr;
    ConstructionResult r = ConstructionResult {node, node};
    return r;
}

inline ConstructionResult make_nop() {
    Node* node = new Node;
    node->stmt.kind = NOP;
    node->true_edge = nullptr;
    node->false_edge = nullptr;
    ConstructionResult r = ConstructionResult {node, node};
    return r;
}


inline ConstructionResult make_block_node(Stmt& s) {

    std::cout << "making block" << std::endl;

    assert(s.kind == BLOCK_STMT);

    Node* root = get_cfg(s.as.block.stmts);

    Node* cursor = root;
    while (cursor->false_edge) {
        cursor = cursor->false_edge;
    }

    ConstructionResult r = ConstructionResult {root, cursor};
    return r;
}


inline ConstructionResult make_if(Stmt& s) {

    std::cout << "making if" << std::endl;
    assert(s.kind == IF_STMT);

    Node* node = new Node;
    node->stmt.kind = EXPR;
    node->stmt.expr = s.as.if_stmt.guard;
    node->true_edge = nullptr;
    node->false_edge = nullptr;

    assert(s.as.if_stmt.then_branch != nullptr);
    assert(s.as.if_stmt.then_branch->kind == BLOCK_STMT);

    Node* join = make_nop().root;

    ConstructionResult then_branch_res = make_block_node(*s.as.if_stmt.then_branch);

    if (s.as.if_stmt.else_branch != nullptr) {
        ConstructionResult else_branch_res = make_block_node(*s.as.if_stmt.else_branch);
        node->false_edge = else_branch_res.root;
        else_branch_res.out->true_edge = join;
    }
    else {
        node->false_edge = join;
    }

    node->true_edge = then_branch_res.root;
    then_branch_res.out->true_edge = join;

    ConstructionResult r = ConstructionResult {node, join};
    return r;
}

inline ConstructionResult make_while(Stmt& s) {

    std::cout << "making while cfg" << std::endl;

    assert(s.kind == WHILE_STMT);

    Node* node = new Node;
    node->stmt.kind = EXPR;
    node->stmt.expr = s.as.while_stmt.guard;
    node->true_edge = nullptr;
    node->false_edge = nullptr;

    assert(s.as.while_stmt.then_branch != nullptr);
    assert(s.as.while_stmt.then_branch->kind == BLOCK_STMT);

    Node* exit = make_nop().root;

    ConstructionResult then_branch_res = make_block_node(*s.as.while_stmt.then_branch);

    node->true_edge = then_branch_res.root;
    node->false_edge = exit;

    then_branch_res.out->true_edge = node;

    ConstructionResult r = ConstructionResult {node, exit};
    return r;
}


Node* get_cfg(std::vector<Stmt*>& program) {

    Node* root = make_nop().root;
    Node* prev = root;

    ConstructionResult r;

    for (auto s : program) {

        switch (s->kind) {

        case ASSIGN_STMT: r = make_assign_node(*s); break;
        case EXPR_STMT: r = make_expr_node(*s); break;
        case DECL_STMT: r = make_decl_node(*s); break;
        case IF_STMT: r = make_if(*s); break;
        case WHILE_STMT: r = make_while(*s); break;
        case BLOCK_STMT: r = make_block_node(*s); break;
        default:
            UNREACHABLE;
        }

        prev->true_edge = r.root;
        prev = r.out;
    }

    assert (root != nullptr);
#ifndef NDEBUG
    if (root->stmt.kind != NOP)
        assert(root->true_edge != nullptr);
#endif

    if (root->true_edge != nullptr)
        return root->true_edge;
    return root;
}

static std::string label(const CFGStmt& s) {

    switch (s.kind) {
        case DECLARATION:
            return "decl " + std::string(s.name.lexeme) + (s.rhs ? " = " + dump_expr(*s.rhs) : std::string());
        case ASSIGNMENT:
            return dump_expr(*s.lhs) + " = " + dump_expr(*s.rhs);
        case EXPR:
            return dump_expr(*s.expr);
        case NOP:
            return "nop";
    }
    return "?";
}


bool write_cfg_dot(const Node* entry, const char* path) {
    FILE* f = fopen(path, "w");
    if (!f) return false;

    fputs("digraph CFG {\n  node [shape=box];\n", f);

    std::unordered_set<const Node*> seen;
    std::vector<const Node*> todo{entry};

    while (!todo.empty()) {
        const Node* n = todo.back();
        todo.pop_back();
        if (!n || !seen.insert(n).second) continue;

        bool branch = n->true_edge && n->false_edge;
        fprintf(f, "  n%p [label=\"%s\"];\n", (const void*)n, label(n->stmt).c_str());
        if (n->true_edge)
            fprintf(f, "  n%p -> n%p%s;\n", (const void*)n, (const void*)n->true_edge,
                    branch ? " [label=\"T\"]" : "");
        if (n->false_edge)
            fprintf(f, "  n%p -> n%p%s;\n", (const void*)n, (const void*)n->false_edge,
                    branch ? " [label=\"F\"]" : "");

        todo.push_back(n->true_edge);
        todo.push_back(n->false_edge);
    }

    fputs("}\n", f);
    fclose(f);
    return true;
}



