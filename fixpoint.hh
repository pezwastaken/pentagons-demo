#pragma once

#include "environment.hh"
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
typename D::State transfer_assume(typename D::State& entry_state, const Expr& e) {

    typename D::State out = D::assume(entry_state, e);
#ifndef NDEBUG
    if (!D::is_bottom(out)) {
        assert(!out.env.empty());
    }
#endif
    return out;
}


struct TransferInfo {
    Satisfiability is_assert_sat;
};

template <StateDomain D>
typename D::State transfer_assert(typename D::State& entry_state, const Expr& e, TransferInfo& assert_info) {

    auto ok = transfer_assume<D>(entry_state, e);
    auto bad = transfer_assume<D>(entry_state, *expr_negate(e));

    if (D::is_bottom(bad)) {
        assert_info.is_assert_sat = SAT_SAT;
    }
    else if (D::is_bottom(ok)){
        assert_info.is_assert_sat = SAT_UNSAT;
    }
    else {
        assert_info.is_assert_sat = SAT_UNKNOWN;
    }

    return ok;
}

template <StateDomain D>
typename D::State transfer(typename D::State entry_state, Node* n, FixpointResult<D>& result) {


    switch (n->stmt.kind) {
    case CFG_DECLARATION:
        return transfer_declaration<D>(entry_state, n);
    case CFG_ASSIGNMENT:
        return transfer_assign<D>(entry_state, n);
    case CFG_ASSERT:
        {
            TransferInfo t {.is_assert_sat = SAT_UNKNOWN };
            typename D::State res_state = transfer_assert<D>(entry_state, *n->stmt.expr, t);
            result.asserts_sat[n] = t.is_assert_sat;
            return res_state;
        }
    case CFG_EXPR_GUARD:
        UNREACHABLE;
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
struct SplitState {
    D::State true_state;
    D::State false_state;
};

template <StateDomain D>
SplitState<D> transfer_split(typename D::State entry_state, Node* n) {
    assert(n->stmt.kind == CFG_EXPR_GUARD);

    assert(n->true_edge);
    assert(n->false_edge);

    assert(n->stmt.lhs);
    assert(n->stmt.rhs);

    typename D::State true_state = transfer_assume<D>(entry_state, *n->stmt.lhs);
    typename D::State false_state = transfer_assume<D>(entry_state, *n->stmt.rhs);

    return SplitState<D> {.true_state = true_state, .false_state = false_state};
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

        if (x->stmt.kind != CFG_EXPR_GUARD) {

            S out_state = transfer<D>(entry_state, x, result);

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
        }
        else {

            SplitState<D> split_state = transfer_split<D>(entry_state, x);

            // state transfer along the true edge
            if (x->true_edge != nullptr) {

                Node* dest = x->true_edge;
                usize dest_id = dest->id;

                S prev = node_states[dest];

                S joined = D::join(split_state.true_state, prev);

                if (!D::leq(joined, prev)) {
                    node_states[dest] = joined;
                    que.push_back(dest);
                }
            }

            // state transfer along the false edge
            if (x->false_edge != nullptr) {
                Node* dest = x->false_edge;
                usize dest_id = dest->id;

                S prev = node_states[dest];

                S joined = D::join(split_state.false_state, prev);

                if (!D::leq(joined, prev)) {
                    node_states[dest] = joined;
                    que.push_back(dest);
                }
            }
        }
    }

    // for (const auto& [id, state] : node_states) {
    //     std::cout << "state for node id: " << id << D::dump_state(node_states[id])
    //         << std::endl;
    // }

    return result;
}



