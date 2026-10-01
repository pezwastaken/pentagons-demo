
#include <string>
#include <string_view>
#include <unordered_map>
#include "lexer.hh"

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


static inline bool more(const Scanner& scanner) {
    return scanner.current < scanner.source->length();
}

static inline Token new_token(Scanner& scanner, TokenType tt) {

    std::string_view lexeme =
        std::string_view(*scanner.source).substr(
            scanner.start, scanner.current - scanner.start);

    //int literal, convert to int first
    if (tt == NUMBER) {
        int value = std::stoi(std::string(lexeme));
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

    std::string_view text =
        std::string_view(*scanner.source).substr(
            scanner.start, scanner.current - scanner.start);

    std::string text_str = std::string(text);

    TokenType tt = IDENTIFIER;
    if (keywords.find(text_str) != keywords.end()) {
        tt = keywords[text_str];
    }

    Token t = new_token(scanner, tt);
    return t;
}

static inline Token scan_token(Scanner& scanner) {
    const std::string& source = *scanner.source;
    char c = source[scanner.current++];
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
        case '/': t = new_token(scanner, SLASH); break;
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


std::vector<Token> scan_tokens(const std::string& source) {

    populate_keywords();

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

