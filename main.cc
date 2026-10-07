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

    Arena* mem_arena = arena_create(1000000);
    Parser p = Parser {tokens, {}, 0U, mem_arena};

    std::cout << "before parser" << std::endl;

    std::vector<Stmt*> program = parse_program(p, {});

    std::cout << "program has " << program.size() << " statements" << std::endl;

    for (auto s : program) {
        std::cout << dump_stmt(*s) << std::endl;
    }

    CFG cfg = get_cfg(program);

    bool done = write_cfg_dot(cfg, "out.dot");

    fixpoint_analyze<NonRelational<Interval>>(cfg);

    arena_destroy(p.arena);

    return 0;
}

