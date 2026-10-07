#pragma once

#include "environment.hh"
#include <iostream>
#include <deque>
#include <unordered_map>

template <StateDomain D>
typename D::State transfer_declaration(typename D::State& entry_state, Node* n) {

    assert(n->stmt.kind == CFG_DECLARATION);

    std::string declaring_name = std::string(n->stmt.name.lexeme);
    assert(declaring_name != "");

    typename D::State out = D::assign(entry_state, declaring_name, *n->stmt.rhs);


#ifndef NDEBUG

    if (!D::is_bottom(out) && !D::is_top(out)) {
        assert(!out.env.empty());
    }
#endif

    return out;
}

template <StateDomain D>
typename D::State transfer_assign(typename D::State& entry_state, Node* n) {

    assert(n->stmt.kind == CFG_ASSIGNMENT);

    std::string declaring_name = std::string(n->stmt.lhs->token.lexeme);
    assert(declaring_name != "");

    typename D::State out = D::assign(entry_state, declaring_name, *n->stmt.rhs);

    assert(!out.env.empty());
    return out;
}

template <StateDomain D>
typename D::State transfer_assume(typename D::State& entry_state, Node* n) {

    assert(n->stmt.kind == CFG_EXPR_GUARD);
    assert(n->true_edge);

    typename D::State out = D::assume(entry_state, *n->stmt.expr);
#ifndef NDEBUG
    if (!D::is_bottom(out)) {
        assert(!out.env.empty());
    }
#endif
    return out;
}


template <StateDomain D>
typename D::State transfer(typename D::State entry_state, Node* n) {

    switch (n->stmt.kind) {
    case CFG_DECLARATION:
        return transfer_declaration<D>(entry_state, n);
    case CFG_ASSIGNMENT:
        return transfer_assign<D>(entry_state, n);
    case CFG_EXPR_GUARD:
        return transfer_assume<D>(entry_state, n);
        // TODO
    case CFG_EXPR:
        // TODO
    case CFG_NOP:
        return entry_state;
    case CFG_RETURN:
        return entry_state; // TODO
    }

    return D::top_state();
}

template <StateDomain D>
FixpointResult<D> fixpoint_analyze(const CFG& cfg) {

    using S = typename D::State;

    FixpointResult<D> result;
    std::unordered_map<Node*, S>& node_states = result.node_states;

    Node* root = cfg.root;

    std::deque<Node*> que;
    que.push_back(root);

    while (!que.empty()) {

        Node* x = que.front();
        que.pop_front();

        const S& entry_state = node_states[x];

        S out_state = transfer<D>(entry_state, x);

        if (x->true_edge != nullptr) {
            Node* dest = x->true_edge;
            usize dest_id = dest->id;

            S prev = node_states[dest];

            S joined = D::join(out_state, prev);

            if (!D::leq(joined, prev)) {
                node_states[dest] = joined;
                que.push_back(dest);
            }
        }

        if (x->false_edge != nullptr) {
            Node* dest = x->false_edge;
            usize dest_id = dest->id;

            S prev = node_states[dest];

            S joined = D::join(out_state, prev);

            if (!D::leq(joined, prev)) {
                node_states[dest] = joined;
                que.push_back(dest);
            }
        }
    }

    // for (const auto& [id, state] : node_states) {
    //     std::cout << "state for node id: " << id << D::dump_state(node_states[id])
    //         << std::endl;
    // }

    return result;
}



