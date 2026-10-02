// #include "fixpoint.hh"
// #include "cfg.hh"
// #include <deque>
// #include <iostream>
// #include <string>
// #include <unordered_map>
//
//
// template <StateDomain D>
// typename D::State transfer_declaration(const typename D::State& entry_state, Node* n) {
//
//     assert(n->stmt.kind == CFG_DECLARATION);
//
//     std::string declaring_name = std::string(n->stmt.name.lexeme);
//     assert(declaring_name != "");
//
//     typename D::State out = D::assign(entry_state, declaring_name, *n->stmt.rhs);
//
//     assert(!out.env.empty());
//     return out;
// }
//
// template <StateDomain D>
// typename D::State transfer_assign(const typename D::State& entry_state, Node* n) {
//
//     assert(n->stmt.kind == CFG_DECLARATION);
//
//     std::string declaring_name = std::string(n->stmt.lhs->token.lexeme);
//     assert(declaring_name != "");
//
//     typename D::State out = D::assign(entry_state, declaring_name, *n->stmt.rhs);
//
//     assert(!out.env.empty());
//     return out;
// }
//
//
// template <StateDomain D>
// typename D::State transfer(typename D::State entry_state, Node* n) {
//
//     switch (n->stmt.kind) {
//     case CFG_DECLARATION:
//         return transfer_declaration(entry_state, n);
//     case CFG_ASSIGNMENT:
//         return transfer_assign(entry_state, n);
//     case CFG_EXPR:
//     case CFG_NOP:
//       break;
//     }
// }
//
// template <StateDomain D>
// void fixpoint_analyze(const CFG& cfg) {
//
//     using S = typename D::State;
//
//     std::unordered_map<usize, S> node_states;
//
//     auto entry_state = D::bot_state();
//     node_states[0U] = entry_state;
//
//     Node* root = cfg.root;
//
//     std::deque<Node*> que;
//     que.push_back(root);
//
//     while (!que.empty()) {
//
//         Node* x = que.front();
//         que.pop_front();
//
//         const S& entry_state = node_states[x->id];
//
//         S out_state = transfer<D>(entry_state, x);
//
//         // Node* out_1 = x->true_edge;
//
//     }
//
//     std::cout << D::dump_state(node_states[0]) << std::endl;
//
// }
//
