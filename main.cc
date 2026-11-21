#include "core_inc.hh"
#include "core_inc.cc"

#include <iostream>
#include <string>


int main() {

    std::string source = read_file("./readme.txt");
    std::cout << source << std::endl;
    std::cout << "-------end source-------" << std::endl;


    std::vector<Token> tokens = scan_tokens(source);

    std::cout << "scanned " << tokens.size() << " tokens" << std::endl;

    // for (Token t : tokens) {
    //     std::cout << t.lexeme << std::endl;
    //     std::cout << t.type << std::endl;
    //     std::cout << std::endl;
    // }

    Parser p = Parser {.ast = Ast {}, .tokens = tokens, .errors = {}, .pos = 0U};

    std::cout << "before parser" << std::endl;

    Program program = parse_program(p, {});

    std::cout << "program has " << program.inner.size() << " statements" << std::endl;

    for (auto s : program.inner) {
        std::cout << dump_stmt(p.ast, s) << std::endl;
    }

    std::cout << "after parsing dump" << std::endl;

    Arena* mem_arena = arena_create(1000000);
    cfg_set_arena(mem_arena);
    CFG cfg = get_cfg(program);

    std::string cfg_text = cfg_dump_as_text(*program.ast, cfg);

    std::cout << "===============\n\n" << std::endl;
    std::cout << cfg_text << std::endl;

    // bool done = write_cfg_dot(*program.ast, cfg, "out.dot");

    // auto fixpoint_result = fixpoint_analyze<NonRelational<Interval>>(cfg);
    //
    // std::cout << "have results for " << fixpoint_result.asserts_sat.size() << " asserts" << std::endl;
    //
    // for (const auto& [node, sat] : fixpoint_result.asserts_sat) {
    //
    //     switch (sat) {
    //     case SAT_UNKNOWN:
    //         std::cerr << "assert at line: " << node->pp.line << " MAY not hold" << std::endl;
    //         break;
    //     case SAT_SAT:
    //         std::cerr << "assert at line: " << node->pp.line << " ALWAYS holds" << std::endl;
    //         break;
    //     case SAT_UNSAT:
    //         std::cerr << "assert at line: " << node->pp.line << " NEVER holds" << std::endl;
    //         break;
    //     case SAT_BOT:
    //         std::cerr << "assert at line: " << node->pp.line << " has bottom state" << std::endl;
    //         break;
    //     }
    // }
    //
    // html_dump_fixpoint(cfg, fixpoint_result);
    //
    // arena_destroy(p.arena);

    return 0;
}

