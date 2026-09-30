#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

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
  PRINT, RETURN, SUPER, THIS, TRUE, VAR, INTID, WHILE,

  WHITESPACE, END_OF_FILE
};


static std::unordered_map<std::string, TokenType> keywords;

static void populate_keywords() {
    keywords["and"] = AND;
    keywords["class"] = CLASS;
    keywords["else"] =  ELSE;
    keywords["false"] = FALSE;
    keywords["for"] =   FOR;
    keywords["fun"] =   FUN;
    keywords["if"] =    IF;
    keywords["nil"] =   NIL;
    keywords["or"] =    OR;
    keywords["print"] = PRINT;
    keywords["return"] =RETURN;
    keywords["super"] = SUPER;
    keywords["this"] =  THIS;
    keywords["true"] =  TRUE;
    keywords["var"] =   VAR;
    keywords["int"] =   INTID;
    keywords["while"] = WHILE;
}


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

    explicit Literal(StaticType t, void* v_ptr) {
        if (t == INT) {
            type = INT;
            value.int_value =  * (int*) v_ptr;
        }
        if (t == BOOL) {
            type = BOOL;
            value.bool_value =  * (bool*) v_ptr;
        }


    }
};


struct Token {
    std::string lexeme;
    Literal literal;
    u32 line;
    TokenType type;

    Token() {
        lexeme = "";
        literal = Literal();
        line = 0U;
        type = END_OF_FILE;
    }

    Token(std::string lexeme, Literal literal, u32 line, TokenType type) :
        lexeme(lexeme), literal(literal), line(line), type(type) { }

};

static Token DEFAULT_TOKEN = Token ("", Literal(), 0U, WHITESPACE);

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
    std::string lexeme = scanner.source->substr(scanner.start, scanner.current - scanner.start);
    std::cout << "returning token with lexeme: " << lexeme << std::endl;

    //int literal, convert to int first
    if (tt == NUMBER) {
        int value = std::stoi(lexeme);
        return Token(lexeme, Literal(StaticType::INT, &value), scanner.line, tt);
    }

    return Token(lexeme, Literal(), scanner.line, tt);
}


static inline bool match_forward(Scanner& scanner, char c) {
    if (!more(scanner)) return false;

    if (scanner.source->at(scanner.current) != c) return false;
    ++scanner.current;
    return true;
}

static inline char peek(Scanner& scanner) {
    if (!more(scanner)) return '\0';
    return (*scanner.source)[scanner.current];
}

static inline bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static inline bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static inline bool is_alphanumeric(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

static inline char advance(Scanner& scanner) {
    return (*scanner.source)[scanner.current++];
}

static inline Token compute_number_literal(Scanner& scanner) {
    while (is_digit(peek(scanner))) advance(scanner);

    Token t = new_token(scanner, NUMBER);
    return t;
}

static inline Token compute_identifier(Scanner& scanner) {
    while (is_alphanumeric(peek(scanner))) advance(scanner);

    std::string text = (*scanner.source).substr(scanner.start, scanner.current - scanner.start);

    TokenType tt = IDENTIFIER;
    if (keywords.find(text) != keywords.end())
        TokenType tt = keywords[text];

    Token t = new_token(scanner, tt);
    return t;
}

static inline Token scan_token(Scanner& scanner) {
    const std::string& source = *scanner.source;
    char c = source[scanner.current++];
    std::cout << "token goes from: " << scanner.start << " to " << scanner.current << std::endl;
    Token t = DEFAULT_TOKEN;
    switch (c) {
        case ' ': break;
        case '\r': break;
        case '\n': ++scanner.line; break;
        case '\t': break;
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
                    t = new_token(scanner, BANG_EQUAL);
                else t = new_token(scanner, BANG);
                break;
            }
        case '=':
            {
                if (match_forward(scanner, '='))
                    t = new_token(scanner, EQUAL_EQUAL);
                else t = new_token(scanner, EQUAL);
                break;
            }
        case '<':
            {
                if (match_forward(scanner, '='))
                    t = new_token(scanner, LESS_EQUAL);
                else t = new_token(scanner, LESS);
                break;
            }
        case '>':
            {
                if (match_forward(scanner, '='))
                    t = new_token(scanner, GREATER_EQUAL);
                else t = new_token(scanner, GREATER);
                break;
            }

        default:
            if (is_digit(c)) {
                t = compute_number_literal(scanner);
            }
            else if (is_alpha(c)) {
                t = compute_identifier(scanner);
            }
            break;
    }
    return t;
}

inline std::vector<Token> scan_tokens(const std::string& source) {

    std::vector<Token> tokens;
    Scanner scanner = Scanner(source);

    while (more(scanner)) {

        scanner.start = scanner.current;
        Token t = scan_token(scanner);

        if (t.type == WHITESPACE)
            continue;

        tokens.push_back(t);
    }

    tokens.push_back(Token ("", Literal(), scanner.line, END_OF_FILE));
    return tokens;
}

