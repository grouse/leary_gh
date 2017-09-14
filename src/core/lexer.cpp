/**
 * file:    lexer.cpp
 * created: 2016-11-28
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2016 - all rights reserved
 */

#include <cstring>

struct Lexer {
    char *at;
    char *end;
    usize size;
};

struct Token {
    enum Type {
        open_curly_brace,
        close_curly_brace,
        open_paren,
        close_paren,
        open_square_brace,
        close_square_brace,
        less_than,
        greater_than,
        semicolon,
        colon,
        equals,
        asterisk,
        comma,
        period,
        ampersand,
        hash,
        forward_slash,
        double_quote,
        single_quote,

        identifier,
        eol,
        eof
    } type;

    int32_t length;
    char *str;
};

Lexer create_lexer(char *ptr, usize size)
{
    Lexer l = {};
    l.at    = ptr;
    l.end   = ptr + size;
    l.size  = size;
    return l;
}

bool is_whitespace(char c)
{
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

bool is_newline(char c)
{
    return (c == '\n' || c == '\r');
}

i64 read_i64(Token token)
{
    i64 result = 0;
    i32 i = token.length;
    while (i && token.str[0] && token.str[0] >= '0' && token.str[0] <= '9') {
        result *= 10;
        result += token.str[0] - '0';

        ++token.str;
        --i;
    }

    return result;
}

u64 read_u64(Token token)
{
    u64 result = 0;
    i32 i = token.length;
    while (i && token.str[0] && token.str[0] >= '0' && token.str[0] <= '9') {
        result *= 10;
        result += token.str[0] - '0';

        ++token.str;
        --i;
    }

    return result;
}

Token::Type token_type(char c)
{
    Token::Type type;

    switch (c) {
    case '{':  type = Token::open_curly_brace;  break;
    case '}':  type = Token::close_curly_brace; break;
    case '[':  type = Token::open_square_brace;  break;
    case ']':  type = Token::close_square_brace; break;
    case '(':  type = Token::open_paren;        break;
    case ')':  type = Token::close_paren;       break;
    case '<':  type = Token::greater_than;      break;
    case '>':  type = Token::less_than;         break;
    case ';':  type = Token::semicolon;         break;
    case ':':  type = Token::colon;             break;
    case '=':  type = Token::equals;            break;
    case '*':  type = Token::asterisk;          break;
    case ',':  type = Token::comma;             break;
    case '.':  type = Token::period;            break;
    case '&':  type = Token::ampersand;         break;
    case '#':  type = Token::hash;              break;
    case '/':  type = Token::forward_slash;     break;
    case '"':  type = Token::double_quote;      break;
    case '\'': type = Token::single_quote;      break;
    case '\0': type = Token::eof;               break;
    case '\n': type = Token::eol;               break;
    case '\r': type = Token::eol;               break;
    default:   type = Token::identifier;        break;
    }

    return type;
}

Token next_token(Lexer *lexer)
{
    Token token = {};
    token.type = Token::eof;

    while (lexer->at < lexer->end) {
        if (is_whitespace(lexer->at[0])) {
            lexer->at++;
        } else if (token_type(lexer->at[0]) == Token::forward_slash &&
                   token_type(lexer->at[1]) == Token::forward_slash)
        {
            lexer->at += 2;

            while (lexer->at[0] && !is_newline(lexer->at[0])) {
                lexer->at++;
            }
        } else if (token_type(lexer->at[0]) == Token::forward_slash &&
                   token_type(lexer->at[1]) == Token::asterisk)
        {
            lexer->at += 2;

            while (lexer->at[0] &&
                   (token_type(lexer->at[0]) != Token::asterisk ||
                    token_type(lexer->at[1]) != Token::forward_slash))
            {
                lexer->at++;
            }
        } else {
            break;
        }
    }

    if (lexer->at < lexer->end) {
        token.type = token_type(lexer->at[0]);
        token.str  = lexer->at;

        if (token.type == Token::identifier) {
            while (lexer->at < lexer->end) {
                if (is_whitespace(lexer->at[0]) ||
                    token_type(lexer->at[0]) != Token::identifier)
                {
                    break;
                }

                lexer->at++;
            }

            token.length = (int32_t)(lexer->at - token.str);
        } else {
            lexer->at++;
            token.length = 1;
        }
    }

    return token;
}

Token peek_next_token(Lexer l)
{
    return next_token(&l);
}

bool is_identifier(Token token, const char *str)
{
    return (strncmp(token.str, str, token.length) == 0);
}
