#pragma once

#include "cfg.hh"
#include "environment.hh"
#include "html_dump.hh"
#include <iostream>

template <StateDomain D>
typename D::State transfer_declaration(typename D::State& entry_state, Node* n) {

    std::cout << "transfer declaration called" << std::endl;

    assert(n->stmt.kind == CFG_DECLARATION);

    std::string declaring_name = std::string(n->stmt.name.lexeme);
    std::cout << "declaring name " << declaring_name << std::endl;
    assert(declaring_name != "");

    typename D::State out = D::assign(entry_state, declaring_name, *n->stmt.rhs);


    std::cout << "dumping state after assign of " << declaring_name <<
        D::dump_state(out) << std::endl;

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
typename D::State transfer(typename D::State entry_state, Node* n) {

    switch (n->stmt.kind) {
    case CFG_DECLARATION:
        return transfer_declaration<D>(entry_state, n);
    case CFG_ASSIGNMENT:
        return transfer_assign<D>(entry_state, n);
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
void fixpoint_analyze(const CFG& cfg) {

    using S = typename D::State;

    std::unordered_map<usize, S> node_states;

    auto entry_state = D::bot_state();
    node_states[0U] = entry_state;

    Node* root = cfg.root;

    std::deque<Node*> que;
    que.push_back(root);

    while (!que.empty()) {

        Node* x = que.front();
        que.pop_front();

        const S& entry_state = node_states[x->id];

        S out_state = transfer<D>(entry_state, x);

        if (x->true_edge != nullptr) {
            Node* dest = x->true_edge;
            usize dest_id = dest->id;

            S prev = node_states[dest_id];

            S joined = D::join(out_state, prev);

            if (!D::leq(joined, prev)) {

                std::cout << "dumping state after join (worklist) for node " <<
                    dest_id << D::dump_state(joined) << std::endl;

                node_states[dest_id] = joined;

                que.push_back(dest);
            }
        }
    }

    for (const auto& [id, state] : node_states) {
        std::cout << "state for node id: " << id << D::dump_state(node_states[id])
            << std::endl;
    }

    html_dump_fixpoint(cfg, FixpointResult<D> {node_states});
}



