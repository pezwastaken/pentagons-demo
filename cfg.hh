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

    CFGStmtKind kind = CFG_NOP;
};

struct CfgNodeId {
    usize id = SIZE_MAX;

    bool valid() const {
        return id != SIZE_MAX;
    }

    bool operator<(const CfgNodeId& other) const {
        return id < other.id;
    }

};

struct Node {
    CFGStmt stmt;

    CfgNodeId true_edge;
    CfgNodeId false_edge;

    SyntheticPP pp;

    usize id;
};

struct CFG {

    std::vector<Node> nodes;

    CfgNodeId add(const Node& n) {
        nodes.push_back(n);
        return {nodes.size() - 1};
    }

    const Node& operator[](CfgNodeId id) const {
        assert(id.valid());
        assert(id.id < nodes.size());
        return nodes[id.id];
    }

    CfgNodeId root;
};


CFG get_cfg(Program& program);

ExprId expr_negate(Ast& ast, const Expr& e);

bool write_cfg_dot(const Ast& ast, const CFG& cfg, const char* path);

std::string dump_cfg_as_text(CFG cfg);

