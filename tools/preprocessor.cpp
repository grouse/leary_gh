/**
 * file:    preprocessor.cpp
 * created: 2016-11-19
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2016 - all rights reserved
 */

#include <stdio.h>
#include <memory>

// TODO(jesper): make use of the engine for this stuff
#if defined(_MSC_VER)
	#define DEBUG_BREAK()       __debugbreak()
#else
	#define DEBUG_BREAK()       asm("int $3")
#endif

#define VAR_UNUSED(var) (void)(var)

#define DEBUG_ASSERT(condition) if (!(condition)) DEBUG_BREAK()

char *
read_entire_file(const char* filename, size_t *out_size)
{
	FILE *file = fopen(filename, "rb");

	size_t size  = 0;
	char *buffer = nullptr;

	if (file) {
		fseek(file, 0, SEEK_END);
		size = ftell(file);

		if (size != 0) 
		{
			rewind(file);

			buffer = (char*)malloc(size + 1);
			size_t result = fread(buffer, 1, size, file);
			DEBUG_ASSERT(result == size);

			buffer[size] = '\0';
			fclose(file);
		}
	}

	*out_size = size;
	return buffer;
}

struct Tokenizer
{
	char *at;
};

enum TokenType 
{
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

enum VariableType
{
	VariableType_int32,
	VariableType_uint32,
	VariableType_int16,
	VariableType_uint16,

	VariableType_unknown,
	VariableType_num
};

struct Token 
{
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

	while (tokenizer.at[0] && is_whitespace(tokenizer.at[0]))
	{
		tokenizer.at++;
	}

	if (tokenizer.at[0])
	{
		token.type = get_token_type(tokenizer.at[0]);
		token.str = tokenizer.at;

		if (token.type == TokenType_identifier)
		{

			while (tokenizer.at[0])
			{
				if (is_whitespace(tokenizer.at[0]) ||
					get_token_type(tokenizer.at[0]) != TokenType_identifier)
				{
					break;
				}

				tokenizer.at++;
			}

			token.length = tokenizer.at - token.str;
		} 
		else
		{
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

VariableType
get_member_variable_type(Token token)
{
	VariableType result = VariableType_unknown;

	if (is_identifier(token, "int32_t") ||
	    is_identifier(token, "int"))
	{
		result = VariableType_int32;
	} 
	else if (is_identifier(token, "uint32_t"))
	{
		result = VariableType_uint32;
	} 
	else if (is_identifier(token, "int16_t"))
	{
		result = VariableType_int16;
	} 
	else if (is_identifier(token, "uint16_t"))
	{
		result = VariableType_uint16;
	} 

	return result;
}

#define CASE_RETURN_ENUM_STR(c) case c: return #c;

const char* 
get_variable_type_str(VariableType type)
{
	switch (type) {
	CASE_RETURN_ENUM_STR(VariableType_int32);
	CASE_RETURN_ENUM_STR(VariableType_uint32);
	CASE_RETURN_ENUM_STR(VariableType_int16);
	CASE_RETURN_ENUM_STR(VariableType_uint16);
	CASE_RETURN_ENUM_STR(VariableType_unknown);
	}
}

int
main(int argc, char **argv)
{
	VAR_UNUSED(argc);
	VAR_UNUSED(argv);

	std::printf("enum VariableType\n{\n");
	for (int32_t i = 0; i < VariableType_num; i++)
	{
		std::printf("\t%s", get_variable_type_str((VariableType)i));

		if (i == (VariableType_num - 1))
			std::printf("\n");
		else
			std::printf(",\n");

	}
	std::printf("};\n\n");

	std::printf("struct StructMemberMetaData\n{\n");
	std::printf("\tVariableType variable_type;\n");
	std::printf("\tconst char   *variable_name;\n");
	std::printf("\tsize_t       offset;\n");
	std::printf("};\n\n");


	size_t size;
	char *file = read_entire_file("C:/Users/grouse/projects/leary/src/core/settings.h", &size);

	if (file == nullptr) return 0;

	Tokenizer tokenizer;
	tokenizer.at = file;

	while (tokenizer.at[0])
	{
		Token token = get_next_token(tokenizer);

		if (token.type != TokenType_eof) 
		{
#if 0
			std::printf("token.type = %d, token.length %d, token.str = %.*s\n",
			            token.type, token.length, token.length, token.str);
#endif

			if (is_identifier(token, "INTROSPECT"))
			{
				get_next_token(tokenizer);

				Token struct_name = get_next_token(tokenizer);
				DEBUG_ASSERT(struct_name.type == TokenType_identifier);

				if (get_next_token(tokenizer).type == TokenType_open_curly_brace)
				{
					token = get_next_token(tokenizer);

					std::printf("StructMemberMetaData %.*s_MemberMetaData[] = {\n",
					            struct_name.length, struct_name.str);

					while (token.type == TokenType_identifier)
					{
						Token member_type = token;
						Token member_name = get_next_token(tokenizer);

						VariableType member_variable_type = get_member_variable_type(member_type);
						const char *member_type_str = get_variable_type_str(member_variable_type);

						std::printf("\t{ %s, \"%.*s\", offsetof(%.*s, %.*s) }",
						            member_type_str,
						            member_name.length, member_name.str,
						            struct_name.length, struct_name.str,
						            member_name.length, member_name.str);

						do 
						{
							token = get_next_token(tokenizer);
						}
						while (token.type != TokenType_semicolon);

						token = get_next_token(tokenizer);

						if (token.type == TokenType_close_curly_brace)
							std::printf("\n");
						else
							std::printf(",\n");
					}

					DEBUG_ASSERT(token.type == TokenType_close_curly_brace);
					std::printf("};\n\n");

					token = get_next_token(tokenizer);
				}
			}
		}
	}

	return 0;
}

