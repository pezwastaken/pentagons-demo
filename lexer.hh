#pragma once

#include <string>
#include <vector>


enum StaticType {
    INT,
    BOOL,
};

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

  ASSERT, INPUT,

  WHITESPACE, END_OF_FILE
};

struct Literal {

    StaticType type;
    union {
        isize int_value;
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

    ~Literal() { }
};


struct Token {
    std::string_view lexeme;
    Literal literal;
    u32 line;
    TokenType type;

    Token() {
        lexeme = "";
        literal = Literal();
        line = 0U;
        type = END_OF_FILE;
    }

    Token(std::string_view lexeme, Literal literal, u32 line, TokenType type) :
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

std::vector<Token> scan_tokens(const std::string& source);

