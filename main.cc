#include <iostream>
#include <string>
#include "arena.hh"
#include "cfg.hh"
#include "environment.hh"

int main() {

    std::string source = read_file("./readme.txt");
    std::cout << source << std::endl;
    std::cout << "-------end source-------" << std::endl;


    std::vector<Token> tokens = scan_tokens(source);

    for (Token t : tokens) {
        std::cout << t.lexeme << std::endl;
        std::cout << t.type << std::endl;
        std::cout << std::endl;
    }

    Arena* mem_arena = arena_create(1000000);
    Parser p = Parser {tokens, {}, 0U, mem_arena};

    std::vector<Stmt*> program = parse_program(p, {});

    std::cout << "program has " << program.size() << " statements" << std::endl;

    for (auto s : program) {
        std::cout << dump_stmt(*s) << std::endl;
    }

    Node* cfg_root = get_cfg(program);

    bool done = write_cfg_dot(cfg_root, "out.dot");

    arena_destroy(p.arena);

    return 0;
}

