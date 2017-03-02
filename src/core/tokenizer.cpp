/**
 * file:    tokenizer.cpp
 * created: 2016-11-28
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2016 - all rights reserved
 */

#include <cstring>

struct Tokenizer {
	char *at;
	char *end;
};

struct Token {
	enum Type {
		open_curly_brace,
		close_curly_brace,
		open_paren,
		close_paren,
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

Tokenizer make_tokenizer(char *ptr, usize end)
{
	Tokenizer t = {};
	t.at  = ptr;
	t.end = ptr + end;
	return t;
}

bool is_whitespace(char c)
{
	return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

bool is_newline(char c)
{
	return (c == '\n' || c == '\r');
}

Token::Type get_token_type(char c)
{
	Token::Type type;

	switch (c) {
	case '{':  type = Token::open_curly_brace;  break;
	case '}':  type = Token::close_curly_brace; break;
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

Token next_token(Tokenizer &tokenizer)
{
	Token token = {};
	token.type = Token::eof;

	while (tokenizer.at < tokenizer.end) {
		if (is_whitespace(tokenizer.at[0])) {
			tokenizer.at++;
		} else if (get_token_type(tokenizer.at[0]) == Token::forward_slash &&
		           get_token_type(tokenizer.at[1]) == Token::forward_slash)
		{
			tokenizer.at += 2;

			while (tokenizer.at[0] && !is_newline(tokenizer.at[0])) {
				tokenizer.at++;
			}
		} else if (get_token_type(tokenizer.at[0]) == Token::forward_slash &&
		           get_token_type(tokenizer.at[1]) == Token::asterisk)
		{
			tokenizer.at += 2;

			while (tokenizer.at[0] &&
			       (get_token_type(tokenizer.at[0]) != Token::asterisk ||
			        get_token_type(tokenizer.at[1]) != Token::forward_slash))
			{
				tokenizer.at++;
			}
		} else {
			break;
		}
	}

	if (tokenizer.at < tokenizer.end) {
		token.type = get_token_type(tokenizer.at[0]);
		token.str = tokenizer.at;

		if (token.type == Token::identifier) {
			while (tokenizer.at < tokenizer.end) {
				if (is_whitespace(tokenizer.at[0]) ||
				    get_token_type(tokenizer.at[0]) != Token::identifier)
				{
					break;
				}

				tokenizer.at++;
			}

			token.length = (int32_t)(tokenizer.at - token.str);
		} else {
			tokenizer.at++;
			token.length = 1;
		}
	}

	return token;
}

Token peek_next_token(Tokenizer &in_tokenizer)
{
	Tokenizer tokenizer = in_tokenizer;
	return next_token(tokenizer);
}

bool is_identifier(Token token, const char *str)
{
	return (strncmp(token.str, str, token.length) == 0);
}
