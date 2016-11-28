/**
 * file:    tokenizer.cpp
 * created: 2016-11-28
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2016 - all rights reserved
 */

struct Tokenizer {
	char *at;
};

enum TokenType {
	TokenType_open_curly_brace,
	TokenType_close_curly_brace,
	TokenType_open_paren,
	TokenType_close_paren,
	TokenType_less_than,
	TokenType_greater_than,
	TokenType_semicolon,
	TokenType_colon,
	TokenType_equals,
	TokenType_asterisk,
	TokenType_comma,
	TokenType_ampersand,
	TokenType_hash,
	TokenType_forward_slash,
	TokenType_double_quote,
	TokenType_single_quote,

	TokenType_identifier,
	TokenType_eof
};

struct Token {
	TokenType type;
	int32_t length;
	char *str;
};

bool is_whitespace(char c)
{
	return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

bool is_newline(char c)
{
	return (c == '\n' || c == '\r');
}

TokenType
get_token_type(char c)
{
	TokenType type;

	switch (c) {
	case '{':  type = TokenType_open_curly_brace;  break;
	case '}':  type = TokenType_close_curly_brace; break;
	case '(':  type = TokenType_open_paren;        break;
	case ')':  type = TokenType_close_paren;       break;
	case '<':  type = TokenType_greater_than;      break;
	case '>':  type = TokenType_less_than;         break;
	case ';':  type = TokenType_semicolon;         break;
	case ':':  type = TokenType_colon;             break;
	case '=':  type = TokenType_equals;            break;
	case '*':  type = TokenType_asterisk;          break;
	case ',':  type = TokenType_comma;             break;
	case '&':  type = TokenType_ampersand;         break;
	case '#':  type = TokenType_hash;              break;
	case '/':  type = TokenType_forward_slash;     break;
	case '"':  type = TokenType_double_quote;      break;
	case '\'': type = TokenType_single_quote;      break;
	case '\0': type = TokenType_eof;               break;
	default:   type = TokenType_identifier;        break;
	}
	
	return type;
}

Token
get_next_token(Tokenizer &tokenizer)
{
	Token token = {};
	token.type = TokenType_eof;

	while (tokenizer.at[0]) {
		if (is_whitespace(tokenizer.at[0])) {
			tokenizer.at++;
		} else if (get_token_type(tokenizer.at[0]) == TokenType_forward_slash &&
		           get_token_type(tokenizer.at[1]) == TokenType_forward_slash)
		{
			tokenizer.at += 2;

			while (tokenizer.at[0] && !is_newline(tokenizer.at[0])) {
				tokenizer.at++;
			}
		} else if (get_token_type(tokenizer.at[0]) == TokenType_forward_slash &&
		           get_token_type(tokenizer.at[1]) == TokenType_asterisk)
		{
			tokenizer.at += 2;

			while (tokenizer.at[0] &&
			       (get_token_type(tokenizer.at[0]) != TokenType_asterisk ||
			        get_token_type(tokenizer.at[1]) != TokenType_forward_slash))
			{
				tokenizer.at++;
			}
		} else {
			break;
		}
	}

	if (tokenizer.at[0]) {
		token.type = get_token_type(tokenizer.at[0]);
		token.str = tokenizer.at;

		if (token.type == TokenType_identifier) {
			while (tokenizer.at[0]) {
				if (is_whitespace(tokenizer.at[0]) ||
				    get_token_type(tokenizer.at[0]) != TokenType_identifier)
				{
					break;
				}

				tokenizer.at++;
			}

			token.length = (int32_t)(tokenizer.at - token.str);
		} 
		else {
			tokenizer.at++;
			token.length = 1;
		}
	}

	return token;
}

Token
peek_next_token(Tokenizer &in_tokenizer)
{
	Tokenizer tokenizer = in_tokenizer;
	return get_next_token(tokenizer);
}

bool
is_identifier(Token token, const char *str)
{
	return (strncmp(token.str, str, token.length) == 0);
}
