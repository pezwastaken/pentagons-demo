#include <fstream>
#include <iostream>
#include <sstream>

#define TODO (exit(1))

using i32 = int;
using isize = long long;
using u32 = unsigned int;
using usize = unsigned long long;

inline std::string read_file(std::string name) {
    std::ifstream t(name);
    std::stringstream buffer;
    buffer << t.rdbuf();
    return buffer.str();
}


enum TokenType {
  LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
  COMMA, DOT, MINUS, PLUS, SEMICOLON, SLASH, STAR,

  BANG, BANG_EQUAL,
  EQUAL, EQUAL_EQUAL,
  GREATER, GREATER_EQUAL,
  LESS, LESS_EQUAL,

  IDENTIFIER, STRING, NUMBER,

  AND, CLASS, ELSE, FALSE, FUN, FOR, IF, NIL, OR,
  PRINT, RETURN, SUPER, THIS, TRUE, VAR, WHILE,

  END_OF_FILE
};


enum StaticType {
    INT,
    BOOL,
};

struct Literal {

    StaticType type;
    union {

        i32 int_value;
        bool bool_value;
    } value;

    explicit Literal() {
        type = INT;
        value.int_value = 0;
    }
};


struct Token {
    std::string lexeme;
    Literal literal;
    u32 line;
    TokenType type;

    Token(std::string lexeme, Literal literal, u32 line, TokenType type) :
        lexeme(lexeme), literal(literal), line(line), type(type) { }

};

static Token DEFAULT_TOKEN = Token ("", Literal(), 0U, END_OF_FILE);

struct Scanner {
    const std::string* source;
    u32 start;
    u32 current;
    u32 line;

    Scanner(const std::string& source) :
        source(&source),
        start(0U),
        current(0U),
        line(1U) { }
};

static inline bool more(const Scanner& scanner) {
    return scanner.current < scanner.source->length();
}

static inline Token new_token(Scanner& scanner, TokenType tt) {
    return Token("", Literal(), scanner.line, tt);
}

static inline bool match_forward(Scanner& scanner, char c) {
    if (!more(scanner)) return false;

    if (scanner.source->at(scanner.current) != c) return false;
    ++scanner.current;
    return true;
}

static inline Token scan_token(Scanner& scanner) {
    const std::string& source = *scanner.source;
    char c = source[scanner.current++];
    Token t = DEFAULT_TOKEN;
    switch (c) {
        case '(': t = new_token(scanner, LEFT_PAREN); break;
        case ')': t = new_token(scanner, RIGHT_PAREN); break;
        case '{': t = new_token(scanner, LEFT_BRACE); break;
        case '}': t = new_token(scanner, RIGHT_BRACE); break;
        case ',': t = new_token(scanner, COMMA); break;
        case '.': t = new_token(scanner, DOT); break;
        case '-': t = new_token(scanner, MINUS); break;
        case '+': t = new_token(scanner, PLUS); break;
        case ';': t = new_token(scanner, SEMICOLON); break;
        case '*': t = new_token(scanner, STAR); break;
        case '!':
            {
                if (match_forward(scanner, '='))
                    new_token(scanner, BANG_EQUAL);
                else new_token(scanner, BANG);
                break;
            }
        default: break;
    }
    return t;
}

std::vector<Token> scan_tokens(const std::string& source) {

    std::vector<Token> tokens;
    Scanner scanner = Scanner(source);

    std::cout << *scanner.source << std::endl;

    while (more(scanner)) {

        scanner.start = scanner.current;
        Token t = scan_token(scanner);
        tokens.push_back(t);
    }

    tokens.push_back(Token ("", Literal(), scanner.line, END_OF_FILE));
    return tokens;
}


int main() {

    std::string source = read_file("./readme.txt");
    std::cout << source << std::endl;
    std::cout << "-------end source-------" << std::endl;


    std::vector<Token> tokens = scan_tokens(source);

    for (Token t : tokens) {
        std::cout << t.type << std::endl;
    }


    return 0;

}

