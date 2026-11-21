#include <iostream>
#include <set>

static u32 id_tracker = 0U;

struct ConstructionResult {
    CfgNodeId root;
    CfgNodeId out;
};

static ConstructionResult inner_get_cfg(CFG& cfg, const std::vector<StmtId>& stmt_list, Program& program);
ConstructionResult inner_get_cfg(CFG& cfg, Program& program);

inline ConstructionResult make_assign_node(CFG& cfg, const Stmt& s) {
    Node node {};

    node.stmt.kind = CFG_ASSIGNMENT;
    node.stmt.lhs = s.as.assign.target;
    node.stmt.rhs = s.as.assign.expr;
    node.id = id_tracker;
    node.pp = SyntheticPP {id_tracker++};

    CfgNodeId id = cfg.add(node);
    ConstructionResult r = ConstructionResult {id, id};
    return r;
}

inline ConstructionResult make_decl_node(CFG& cfg, const Stmt& s) {

    Node node {};
    node.stmt.kind = CFG_DECLARATION;
    node.stmt.name = s.as.decl.name;
    node.stmt.rhs = s.as.decl.expr;
    node.id = id_tracker;
    node.pp = SyntheticPP {id_tracker++};

    CfgNodeId id = cfg.add(node);

    assert(!node.stmt.lhs.valid());
    assert(node.stmt.rhs.valid());
    ConstructionResult r = ConstructionResult {id, id};
    return r;
}

inline ConstructionResult make_expr_node(CFG& cfg, const Stmt& s) {
    Node node {};
    node.stmt.kind = CFG_EXPR;
    assert(s.as.expr.expr.valid());

    node.stmt.expr = s.as.expr.expr;
    node.id = id_tracker;
    node.pp = SyntheticPP {id_tracker++};

    CfgNodeId id = cfg.add(node);
    ConstructionResult r = ConstructionResult {id, id};
    return r;
}

inline ConstructionResult make_assert_node(CFG& cfg, const Stmt& s) {
    Node node {};
    node.stmt.kind = CFG_ASSERT;

    assert(s.as.expr.expr.valid());

    node.stmt.expr = s.as.expr.expr;
    node.id = id_tracker;
    node.pp = SyntheticPP {id_tracker++};

    CfgNodeId id = cfg.add(node);
    ConstructionResult r = ConstructionResult {id, id};
    return r;
}

inline ConstructionResult make_ret_node(CFG& cfg, const Stmt& s) {
    Node node {};
    node.stmt.kind = CFG_RETURN;
    assert(s.as.ret_stmt.expr.valid());

    node.stmt.expr = s.as.ret_stmt.expr;
    node.id = id_tracker;
    node.pp = SyntheticPP {id_tracker++};

    CfgNodeId id = cfg.add(node);
    ConstructionResult r = ConstructionResult {id, id};
    return r;
}

inline ConstructionResult make_nop(CFG& cfg) {
    Node node {};
    node.stmt.kind = CFG_NOP;
    node.id = id_tracker;
    node.pp = SyntheticPP {id_tracker++};

    CfgNodeId id = cfg.add(node);
    assert(id.valid());
    ConstructionResult r = ConstructionResult {id, id};
    return r;
}

inline ConstructionResult make_block_node(CFG& cfg, Program& program, const Stmt& s) {

    assert(s.kind == BLOCK_STMT);

    // usize upto = s.as.block.first.id + s.as.block.count;
    //
    // CfgNodeId cursor = root;
    // while (cfg[cursor].true_edge.valid()) {
    //     cursor = cfg[cursor].true_edge;
    // }

    return inner_get_cfg(cfg, s.children, program);
}

ExprId expr_negate(Ast& ast, const Expr& e) {
    assert(e.kind == BINARY_EXPR); // TODO: could be unary too but whatever

    Expr negated {};
    negated.kind = BINARY_EXPR;
    negated.token = e.token;
    negated.type = INT;
    negated.left = e.left;
    negated.right = e.right;

    switch (e.bin_op) {
    case OP_BINARY_EQ:
        negated.bin_op = OP_BINARY_NE; break;
    case OP_BINARY_NE:
        negated.bin_op = OP_BINARY_EQ; break;
    case OP_BINARY_LT:
        negated.bin_op = OP_BINARY_GE; break;
    case OP_BINARY_LE:
        negated.bin_op = OP_BINARY_GT; break;
    case OP_BINARY_GT:
        negated.bin_op = OP_BINARY_LE; break;
    case OP_BINARY_GE:
        negated.bin_op = OP_BINARY_LT; break;
    default:
        TODO;
    }

    return ast.add(negated);
}

inline ConstructionResult make_if(CFG& cfg, Program& program, const Stmt& s) {

    assert(s.kind == IF_STMT);

    Ast& ast = *program.ast;

    Node node {};
    node.stmt.kind = CFG_EXPR_GUARD;
    node.stmt.expr = s.as.if_stmt.guard;
    node.stmt.lhs = s.as.if_stmt.guard;
    node.stmt.rhs = expr_negate(ast, ast[s.as.if_stmt.guard]);
    node.id = id_tracker;
    node.pp = SyntheticPP {id_tracker++};

    assert(s.as.if_stmt.then_branch.valid());
    assert(ast[s.as.if_stmt.then_branch].kind == BLOCK_STMT);

    ConstructionResult then_branch_res =
        make_block_node(cfg, program, ast[s.as.if_stmt.then_branch]);
    ConstructionResult else_branch_res;

    if (s.as.if_stmt.else_branch.valid()) {
        else_branch_res =
            make_block_node(cfg, program, ast[s.as.if_stmt.else_branch]);
        node.false_edge = else_branch_res.root;
    }

    CfgNodeId join = make_nop(cfg).root;

    if (s.as.if_stmt.else_branch.valid()) {
        cfg.nodes[else_branch_res.out.id].true_edge = join;
    }
    else {
        node.false_edge = join;
    }

    node.true_edge = then_branch_res.root;
    cfg.nodes[then_branch_res.out.id].true_edge = join;

    CfgNodeId id = cfg.add(node);
    ConstructionResult r = ConstructionResult {id, join};
    return r;
}

inline ConstructionResult make_while(CFG& cfg, Program& program, const Stmt& s) {

    assert(s.kind == WHILE_STMT);

    Ast& ast = *program.ast;

    Node node {};
    node.stmt.kind = CFG_EXPR_GUARD;
    node.stmt.expr = s.as.while_stmt.guard;
    node.id = id_tracker;
    node.pp = SyntheticPP {id_tracker++};

    assert(s.as.while_stmt.then_branch.valid());
    assert(ast[s.as.while_stmt.then_branch].kind == BLOCK_STMT);

    CfgNodeId exit = make_nop(cfg).root;

    ConstructionResult then_branch_res =
        make_block_node(cfg,program, ast[s.as.while_stmt.then_branch]);

    node.true_edge = then_branch_res.root;
    node.false_edge = exit;

    CfgNodeId id = cfg.add(node);

    cfg.nodes[then_branch_res.out.id].true_edge = id;

    ConstructionResult r = ConstructionResult {id, exit};
    return r;
}

static ConstructionResult inner_get_cfg(CFG& cfg, const std::vector<StmtId>& stmt_list, Program& program) {

    CfgNodeId root = make_nop(cfg).root;
    CfgNodeId prev = root;

    ConstructionResult r;

    for (StmtId s_id : stmt_list) {

        assert(s_id.valid());

        const auto& s = (*program.ast)[s_id];

        switch (s.kind) {

        case ASSIGN_STMT: r = make_assign_node(cfg,s); break;
        case EXPR_STMT: r = make_expr_node(cfg, s); break;
        case ASSERT_STMT: r = make_assert_node(cfg, s); break;
        case DECL_STMT: r = make_decl_node(cfg, s); break;
        case RET_STMT: r = make_ret_node(cfg, s); break;
        case IF_STMT: r = make_if(cfg, program, s); break;
        case WHILE_STMT: r = make_while(cfg, program, s); break;
        case BLOCK_STMT: r = make_block_node(cfg, program, s); break;
        default:
            UNREACHABLE;
        }

        cfg.nodes[prev.id].true_edge = r.root;
        prev = r.out;
    }

    assert (cfg[root].stmt.kind == CFG_NOP);
    assert (root.valid());
    assert (prev.valid());

    if (cfg[root].true_edge.valid()) {
        assert (cfg[root].true_edge.valid());
        return ConstructionResult {.root = cfg[root].true_edge, .out = prev};
    }

    return ConstructionResult {.root = root, .out = prev };

}

ConstructionResult inner_get_cfg(CFG& cfg, Program& program) {
    return inner_get_cfg(cfg, program.inner, program);
}

CFG get_cfg(Program& program) {

    CFG cfg {};
    CfgNodeId root = inner_get_cfg(cfg, program).root;
    cfg.root = root;
    return cfg;
}

static std::string label(const Ast& ast, const CFGStmt& s, usize id) {

    switch (s.kind) {
        case CFG_DECLARATION:
            return "[" + std::to_string(id) + "]" + "decl " + std::string(s.name.lexeme) + (s.rhs.valid() ? " = " + dump_expr(ast, s.rhs) : std::string());
        case CFG_ASSIGNMENT:
            return "[" + std::to_string(id) + "]" + dump_expr(ast, s.lhs) + " = " + dump_expr(ast, s.rhs);
        case CFG_EXPR_GUARD:
        case CFG_EXPR:
            return "[" + std::to_string(id) + "]" + dump_expr(ast, s.expr);
        case CFG_RETURN:
            return "[" + std::to_string(id) + "]" + " return " + dump_expr(ast, s.expr);
        case CFG_ASSERT:
            return "[" + std::to_string(id) + "]" + " assert " + dump_expr(ast, s.expr);
        case CFG_NOP:
            return "[" + std::to_string(id) + "]" + " nop";
    }
    return "?";
}


bool write_cfg_dot(const Ast& ast, const CFG& cfg, const char* path) {
    FILE* f = fopen(path, "w");
    if (!f) return false;

    CfgNodeId entry = cfg.root;

    fputs("digraph CFG {\n  node [shape=box];\n", f);

    std::set<CfgNodeId> seen;
    std::vector<CfgNodeId> todo{entry};

    while (!todo.empty()) {
        CfgNodeId n = todo.back();
        todo.pop_back();
        if (!n.valid() || !seen.insert(n).second) continue;

        bool branch = cfg[n].true_edge.valid() && cfg[n].false_edge.valid();
        fprintf(f, "  n%p [label=\"%s\"];\n", (const void*)(&cfg.nodes[n.id]), label(ast, cfg[n].stmt, cfg[n].id).c_str());
        if (cfg[n].true_edge.valid())
            fprintf(f, "  n%p -> n%p%s;\n", (const void*)(&cfg.nodes[n.id]), (const void*)(&cfg.nodes[cfg[n].true_edge.id]),
                    branch ? " [label=\"T\"]" : "");
        if (cfg[n].false_edge.valid())
            fprintf(f, "  n%p -> n%p%s;\n", (const void*)(&cfg.nodes[n.id]), (const void*)(&cfg.nodes[cfg[n].false_edge.id]),
                    branch ? " [label=\"F\"]" : "");

        todo.push_back(cfg[n].true_edge);
        todo.push_back(cfg[n].false_edge);
    }

    fputs("}\n", f);
    fclose(f);
    return true;
}


std::string cfg_dump_as_text(const Ast& ast, const CFG& cfg) {

    CfgNodeId entry = cfg.root;
    assert (entry.valid());

    std::set<CfgNodeId> seen;
    std::vector<CfgNodeId> todo {entry};

    std::string res = "";

    // TODO: if done recursively by entering in the true edges and stuff like that,
    // can even easily track the indentation and print it nicely

    while (!todo.empty()) {
        CfgNodeId n = todo.back();
        todo.pop_back();

        if (!n.valid() || !seen.insert(n).second) continue;

        res += "[ " + std::to_string(cfg[n].id) + " ]    ";

        switch (cfg[n].stmt.kind) {
        case CFG_DECLARATION:
            res += std::string(cfg[n].stmt.name.lexeme) + " = " + dump_expr(ast, cfg[n].stmt.rhs);
            break;
        case CFG_ASSIGNMENT:
            res += dump_expr(ast, cfg[n].stmt.lhs) + " = " + dump_expr(ast, cfg[n].stmt.rhs) ;
            break;
        case CFG_EXPR:
            res += dump_expr(ast, cfg[n].stmt.expr);
            break;
        case CFG_EXPR_GUARD:
            res += dump_expr(ast, cfg[n].stmt.expr);
            break;
        case CFG_RETURN:
            res += "return " + dump_expr(ast, cfg[n].stmt.expr);
            break;
        case CFG_ASSERT:
            res += "assert " + dump_expr(ast, cfg[n].stmt.expr);
            break;
        case CFG_NOP:
            res += "NOP";
              break;
        }

        res += "\n";

        todo.push_back(cfg[n].false_edge);
        todo.push_back(cfg[n].true_edge);
    }

    return res;
}


