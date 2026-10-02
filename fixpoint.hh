#pragma once

#include "cfg.hh"
#include "environment.hh"
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
    case CFG_NOP:
        return entry_state;
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

    int i = 0;

    while (!que.empty() && i < 2) {

        Node* x = que.front();
        que.pop_front();

        const S& entry_state = node_states[x->id];

        S out_state = transfer<D>(entry_state, x);


        if (x->true_edge != nullptr) {
            const Node* dest = x->true_edge;
            usize dest_id = dest->id;

            S prev = node_states[dest_id];

            // TODO: if out_state leq prev then stop

            S joined = D::join(out_state, prev);

            std::cout << "dumping state after join (worklist) for node " <<
                dest_id << D::dump_state(joined) << std::endl;

            node_states[dest_id] = joined;
        }

        Node* out_1 = x->true_edge;
        que.push_back(out_1);

        i++;
    }

    std::cout << D::dump_state(node_states[1U]) << std::endl;
    std::cout << D::dump_state(node_states[2U]) << std::endl;
    std::cout << D::dump_state(node_states[3U]) << std::endl;
}

