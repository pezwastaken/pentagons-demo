#include "cfg.hh"
#include "parser.hh"
#include <iostream>
#include <unordered_set>

static u32 id_tracker = 0U;

struct ConstructionResult {
    Node* root;
    Node* out;
};

Node* inner_get_cfg(std::vector<Stmt*>& program);

inline ConstructionResult make_assign_node(Stmt& s) {
    Node* node = new Node;
    node->stmt.kind = CFG_ASSIGNMENT;
    node->stmt.lhs = s.as.assign.target;
    node->stmt.rhs = s.as.assign.expr;
    node->true_edge = nullptr;
    node->false_edge = nullptr;
    node->id = id_tracker;
    node->pp = SyntheticPP {id_tracker++};

    ConstructionResult r = ConstructionResult {node, node};
    return r;
}

inline ConstructionResult make_decl_node(Stmt& s) {
    Node* node = new Node;
    node->stmt.kind = CFG_DECLARATION;
    node->stmt.name = s.as.decl.name;
    node->stmt.rhs = s.as.decl.expr;
    node->true_edge = nullptr;
    node->false_edge = nullptr;
    node->id = id_tracker;
    node->pp = SyntheticPP {id_tracker++};
    ConstructionResult r = ConstructionResult {node, node};
    return r;
}

inline ConstructionResult make_expr_node(Stmt& s) {
    Node* node = new Node;
    node->stmt.kind = CFG_EXPR;
    assert(s.as.expr.expr != nullptr);
    node->stmt.expr = s.as.expr.expr;
    node->true_edge = nullptr;
    node->false_edge = nullptr;
    node->id = id_tracker;
    node->pp = SyntheticPP {id_tracker++};
    ConstructionResult r = ConstructionResult {node, node};
    return r;
}

inline ConstructionResult make_ret_node(Stmt& s) {
    Node* node = new Node;
    node->stmt.kind = CFG_RETURN;
    assert(s.as.ret_stmt.expr != nullptr);
    node->stmt.expr = s.as.ret_stmt.expr;
    node->true_edge = nullptr;
    node->false_edge = nullptr;
    node->id = id_tracker;
    node->pp = SyntheticPP {id_tracker++};
    ConstructionResult r = ConstructionResult {node, node};
    return r;
}

inline ConstructionResult make_nop() {
    Node* node = new Node;
    node->stmt.kind = CFG_NOP;
    node->true_edge = nullptr;
    node->false_edge = nullptr;
    node->id = id_tracker;
    node->pp = SyntheticPP {id_tracker++};
    ConstructionResult r = ConstructionResult {node, node};
    return r;
}

inline ConstructionResult make_block_node(Stmt& s) {

    std::cout << "making block" << std::endl;

    assert(s.kind == BLOCK_STMT);

    Node* root = inner_get_cfg(s.as.block.stmts);

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
    node->stmt.kind = CFG_EXPR;
    node->stmt.expr = s.as.if_stmt.guard;
    node->true_edge = nullptr;
    node->false_edge = nullptr;
    node->id = id_tracker;
    node->pp = SyntheticPP {id_tracker++};

    assert(s.as.if_stmt.then_branch != nullptr);
    assert(s.as.if_stmt.then_branch->kind == BLOCK_STMT);

    ConstructionResult then_branch_res = make_block_node(*s.as.if_stmt.then_branch);
    ConstructionResult else_branch_res;

    if (s.as.if_stmt.else_branch != nullptr) {
        else_branch_res = make_block_node(*s.as.if_stmt.else_branch);
        node->false_edge = else_branch_res.root;
    }

    Node* join = make_nop().root;

    if (s.as.if_stmt.else_branch != nullptr) {
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
    node->stmt.kind = CFG_EXPR;
    node->stmt.expr = s.as.while_stmt.guard;
    node->true_edge = nullptr;
    node->false_edge = nullptr;
    node->id = id_tracker;
    node->pp = SyntheticPP {id_tracker++};

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

Node* inner_get_cfg(std::vector<Stmt*>& program) {
    Node* root = make_nop().root;
    Node* prev = root;

    ConstructionResult r;

    for (auto s : program) {

        switch (s->kind) {

        case ASSIGN_STMT: r = make_assign_node(*s); break;
        case EXPR_STMT: r = make_expr_node(*s); break;
        case DECL_STMT: r = make_decl_node(*s); break;
        case RET_STMT: r = make_ret_node(*s); break;
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
    if (root->stmt.kind != CFG_NOP)
        assert(root->true_edge != nullptr);
#endif

    if (root->true_edge != nullptr)
        return root->true_edge;
    return root;
}

CFG get_cfg(std::vector<Stmt*>& program) {
    return CFG { inner_get_cfg(program) };
}

static std::string label(const CFGStmt& s, usize id) {

    switch (s.kind) {
        case CFG_DECLARATION:
            return "[" + std::to_string(id) + "]" + "decl " + std::string(s.name.lexeme) + (s.rhs ? " = " + dump_expr(*s.rhs) : std::string());
        case CFG_ASSIGNMENT:
            return "[" + std::to_string(id) + "]" + dump_expr(*s.lhs) + " = " + dump_expr(*s.rhs);
        case CFG_EXPR:
            return "[" + std::to_string(id) + "]" + dump_expr(*s.expr);
        case CFG_RETURN:
            return "[" + std::to_string(id) + "]" + " return " + dump_expr(*s.expr);
        case CFG_NOP:
            return "[" + std::to_string(id) + "]" + " nop";
    }
    return "?";
}


bool write_cfg_dot(CFG cfg, const char* path) {
    FILE* f = fopen(path, "w");
    if (!f) return false;

    const Node* entry = cfg.root;

    fputs("digraph CFG {\n  node [shape=box];\n", f);

    std::unordered_set<const Node*> seen;
    std::vector<const Node*> todo{entry};

    while (!todo.empty()) {
        const Node* n = todo.back();
        todo.pop_back();
        if (!n || !seen.insert(n).second) continue;

        bool branch = n->true_edge && n->false_edge;
        fprintf(f, "  n%p [label=\"%s\"];\n", (const void*)n, label(n->stmt, n->id).c_str());
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




