#pragma once

#include <deque>
#include <utility>
#include <map>

static constexpr u32 WIDENING_THRESHOLD = 20U;

template <StateDomain D>
typename D::State transfer_declaration(const CFG& cfg, typename D::State& entry_state, const Node& n) {

    assert(n.stmt.kind == CFG_DECLARATION);

    auto declaring_name = n.stmt.name.lexeme;
    assert(declaring_name != "");

    typename D::State out = D::assign(cfg, entry_state, declaring_name, cfg[n.stmt.rhs]);
    return out;
}

template <StateDomain D>
typename D::State transfer_assign(const CFG& cfg, typename D::State& entry_state, const Node& n) {

    assert(n.stmt.kind == CFG_ASSIGNMENT);

    auto declaring_name = cfg[n.stmt.lhs].token.lexeme;
    assert(declaring_name != "");

    typename D::State out = D::assign(cfg, entry_state, declaring_name, cfg[n.stmt.rhs]);

    return out;
}

template <StateDomain D>
typename D::State transfer_assume(const CFG& cfg, typename D::State& entry_state, const Expr& e) {

    typename D::State out = D::assume(cfg, entry_state, e);
    return out;
}


struct TransferInfo {
    Satisfiability is_assert_sat;
};

template <StateDomain D>
typename D::State transfer_assert(const CFG& cfg, typename D::State& entry_state, const Expr& e, TransferInfo& assert_info) {

    Satisfiability sat = D::satisfies(cfg,  entry_state, e);
    assert_info.is_assert_sat = sat;

    return transfer_assume<D>(cfg, entry_state, e);
}

template <StateDomain D>
typename D::State transfer(const CFG& cfg, typename D::State entry_state, CfgNodeId node_id, FixpointResult<D>& result) {

    const Node& n = cfg[node_id];

    switch (n.stmt.kind) {
    case CFG_DECLARATION:
        return transfer_declaration<D>(cfg, entry_state, n);
    case CFG_ASSIGNMENT:
        return transfer_assign<D>(cfg, entry_state, n);
    case CFG_ASSERT:
        {
            TransferInfo t {.is_assert_sat = SAT_UNKNOWN };
            typename D::State res_state = transfer_assert<D>(cfg, entry_state, cfg[n.stmt.expr], t);
            result.asserts_sat[node_id] = t.is_assert_sat;
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
SplitState<D> transfer_split(const CFG& cfg, typename D::State entry_state, CfgNodeId n) {
    assert(cfg[n].stmt.kind == CFG_EXPR_GUARD);

    assert(cfg[n].true_edge.valid());
    assert(cfg[n].false_edge.valid());

    assert(cfg[n].stmt.lhs.valid());
    assert(cfg[n].stmt.rhs.valid());

    typename D::State true_state = transfer_assume<D>(cfg, entry_state, cfg[cfg[n].stmt.lhs]);
    typename D::State false_state = transfer_assume<D>(cfg, entry_state, cfg[cfg[n].stmt.rhs]);

    return SplitState<D> {.true_state = true_state, .false_state = false_state};
}

template <StateDomain D>
std::pair<typename D::State, bool> fixpoint_propagate(
        const CFG& cfg,
        const typename D::State& entry_state,
        CfgNodeId dst_id,
        std::map<CfgNodeId, typename D::State>& node_states,
        std::map<CfgNodeId, u32>& iter_counter) {

    using S = typename D::State;

    const Node& dst = cfg[dst_id];

    S prev = node_states[dst_id];
    S joined = D::join(entry_state, prev);

    bool should_reque_node = false;

    // it's a guard, may have to widen
    if (dst.stmt.kind == CFG_EXPR_GUARD) {

        if (!iter_counter.contains(dst_id)) {
            iter_counter[dst_id] = 0U;
        }
        else if (iter_counter[dst_id] >= WIDENING_THRESHOLD){
            joined = D::widening(prev, joined);
        }
        ++iter_counter[dst_id];
    }

    if (!D::leq(joined, prev)) {
        node_states[dst_id] = joined;
        should_reque_node = true;
    }

    return std::make_pair(joined, should_reque_node);
}

template <StateDomain D>
FixpointResult<D> fixpoint_analyze(const CFG& cfg) {

    using S = typename D::State;

    FixpointResult<D> result {};
    std::map<CfgNodeId, S>& node_states = result.node_states;
    std::map<CfgNodeId, u32> iter_counter {};

    CfgNodeId root = cfg.root;

    std::deque<CfgNodeId> que;
    que.push_back(root);

    S entrypoint_state = D::initial_state();
    node_states[root] = entrypoint_state;

    while (!que.empty()) {

        CfgNodeId x = que.front();
        que.pop_front();

        const S& entry_state = node_states[x];

        const Node& node = cfg[x];

        if (node.stmt.kind != CFG_EXPR_GUARD) {

            S out_state = transfer<D>(cfg, entry_state, x, result);

            if (node.true_edge.valid()) {

                CfgNodeId dst_id = node.true_edge;

                std::pair<S, bool> out =
                    fixpoint_propagate<D>(cfg, out_state, dst_id, node_states, iter_counter);

                if (out.second) {
                    que.push_back(dst_id);
                }
            }
        }
        else {

            SplitState<D> split_state = transfer_split<D>(cfg, entry_state, x);

            // state transfer along the true edge
            if (node.true_edge.valid()) {

                CfgNodeId dst_id = node.true_edge;

                std::pair<S, bool> out =
                    fixpoint_propagate<D>(cfg, split_state.true_state, dst_id, node_states, iter_counter);

                if (out.second) {
                    que.push_back(dst_id);
                }
            }

            // state transfer along the false edge
            if (node.false_edge.valid()) {

                CfgNodeId dst_id = node.false_edge;

                std::pair<S, bool> out =
                    fixpoint_propagate<D>(cfg, split_state.false_state, dst_id, node_states, iter_counter);

                if (out.second) {
                    que.push_back(dst_id);
                }
            }
        }
    }

    for (const auto& [id, state] : node_states) {
        std::cout << "state for node id: " << id.id << D::dump_state(node_states[id])
            << std::endl;
    }

    return result;
}



