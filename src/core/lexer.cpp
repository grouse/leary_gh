/**
 * file:    lexer.cpp
 * created: 2016-11-28
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2016-2017 - all rights reserved
 */

#include <cstring>
#include <cstdio>

struct Lexer {
    char *at;
    char *end;
    usize size;
    i32   line_number;
};

struct Token {
    enum Type {
        number,

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
    l.at          = ptr;
    l.end         = ptr + size;
    l.size        = size;
    l.line_number = 1;
    return l;
}

bool is_whitespace(char c)
{
    return (c == ' ' || c == '\t');
}

bool is_newline(char c)
{
    return (c == '\n' || c == '\r');
}

void eat_newline(Lexer *l)
{
    if (l->at[0] == '\r') {
        l->at++;
    }

    if (l->at[0] == '\n') {
        l->at++;
    }
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

f32 read_f32(Token t)
{
    f32 result = 0.0f;
    // TODO(jesper): replace with own sscanf for floats
    sscanf(t.str, "%f", &result);
    return result;
}

Token::Type token_type(char c)
{
    if (c >= '0' && c <= '9') {
        return Token::number;
    }

    switch (c) {
    case '{':  return Token::open_curly_brace;
    case '}':  return Token::close_curly_brace;
    case '[':  return Token::open_square_brace;
    case ']':  return Token::close_square_brace;
    case '(':  return Token::open_paren;
    case ')':  return Token::close_paren;
    case '<':  return Token::greater_than;
    case '>':  return Token::less_than;
    case ';':  return Token::semicolon;
    case ':':  return Token::colon;
    case '=':  return Token::equals;
    case '*':  return Token::asterisk;
    case ',':  return Token::comma;
    case '.':  return Token::period;
    case '&':  return Token::ampersand;
    case '#':  return Token::hash;
    case '/':  return Token::forward_slash;
    case '"':  return Token::double_quote;
    case '\'': return Token::single_quote;
    case '\0': return Token::eof;
    case '\n': return Token::eol;
    case '\r': return Token::eol;
    default:   return Token::identifier;
    }
}

Token next_token(Lexer *lexer)
{
    Token token = {};
    token.type = Token::eof;

    while (lexer->at < lexer->end) {
        if (is_whitespace(lexer->at[0])) {
            lexer->at++;
        } else if (is_newline(lexer->at[0])) {
            lexer->line_number++;
            eat_newline(lexer);
        } else if (token_type(lexer->at[0]) == Token::forward_slash &&
                   token_type(lexer->at[1]) == Token::forward_slash)
        {
            lexer->at += 2;

            while (lexer->at[0] && !is_newline(lexer->at[0])) {
                lexer->at++;
            }
            lexer->line_number++;
            eat_newline(lexer);
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
        } else if (token.type == Token::number) {
            while (lexer->at < lexer->end) {
                if (is_whitespace(lexer->at[0]) ||
                    token_type(lexer->at[0]) != Token::number)
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
