#pragma once

#include "cfg.hh"
#include "environment.hh"
#include <deque>
#include <unordered_map>
#include <utility>

static constexpr u32 WIDENING_THRESHOLD = 20U;

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
std::pair<typename D::State, bool> fixpoint_propagate(
        const typename D::State& entry_state,
        Node* src,
        Node* dst,
        std::unordered_map<Node*, typename D::State>& node_states,
        std::unordered_map<Node*, u32>& iter_counter,
        FixpointResult<D>& fixpoint_result) {

    using S = typename D::State;
    S out_state = transfer(entry_state, src, fixpoint_result);

    S prev = node_states[dst];
    S joined = D::join(out_state, prev);

    bool should_reque_node = false;

    // it's a guard, may have to widen
    if (dst->stmt.kind == CFG_EXPR_GUARD) {

        if (!iter_counter.contains(dst)) {
            iter_counter[dst] = 0U;
        }
        else if (iter_counter[dst] >= WIDENING_THRESHOLD){
            std::cout << "triggering widening" << std::endl;
            joined = D::widening(prev, joined);
        }
        ++iter_counter[dst];
    }

    if (!D::leq(joined, prev)) {
        node_states[dst] = joined;
        should_reque_node = true;
    }

    return std::make_pair(joined, should_reque_node);
}

template <StateDomain D>
FixpointResult<D> fixpoint_analyze(const CFG& cfg) {

    using S = typename D::State;

    FixpointResult<D> result;
    std::unordered_map<Node*, S>& node_states = result.node_states;
    std::unordered_map<Node*, u32> iter_counter {};

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

                Node* dst = x->true_edge;
                S prev = node_states[dst];
                std::pair<S, bool> out =
                    fixpoint_propagate<D>(entry_state, x, dst, node_states, iter_counter, result);

                if (out.second) {
                    que.push_back(dst);
                }
            }
        }
        else {

            SplitState<D> split_state = transfer_split<D>(entry_state, x);

            // state transfer along the true edge
            if (x->true_edge != nullptr) {

                Node* dst = x->true_edge;
                S prev = node_states[dst];
                std::pair<S, bool> out =
                    fixpoint_propagate<D>(entry_state, x, dst, node_states, iter_counter, result);

                if (out.second) {
                    que.push_back(dst);
                }
            }

            // state transfer along the false edge
            if (x->false_edge != nullptr) {

                Node* dst = x->false_edge;
                S prev = node_states[dst];
                std::pair<S, bool> out =
                    fixpoint_propagate<D>(entry_state, x, dst, node_states, iter_counter, result);

                if (out.second) {
                    que.push_back(dst);
                }
            }
        }
    }

    for (const auto& [id, state] : node_states) {
        std::cout << "state for node id: " << id << D::dump_state(node_states[id])
            << std::endl;
    }

    return result;
}



