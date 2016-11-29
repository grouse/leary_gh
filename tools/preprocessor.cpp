/**
 * file:    preprocessor.cpp
 * created: 2016-11-19
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2016 - all rights reserved
 */

#include <stdio.h>
#include <memory>

#include "core/tokenizer.cpp"

#if defined(_WIN32)
#include "platform/win32_debug.cpp"
#else
#error "unsupported platform"
#endif

char *read_entire_file(const char* filename, size_t *out_size)
{
	FILE *file = fopen(filename, "rb");

	size_t size  = 0;
	char *buffer = nullptr;

	if (file) {
		fseek(file, 0, SEEK_END);
		size = ftell(file);

		if (size != 0) {
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


enum VariableType {
	VariableType_int32,
	VariableType_uint32,
	VariableType_int16,
	VariableType_uint16,

	VariableType_resolution,
	VariableType_video_settings,
	VariableType_settings,

	VariableType_unknown,
	VariableType_num
};


VariableType variable_type(Token token)
{
	VariableType result = VariableType_unknown;

	if (is_identifier(token, "int32_t") ||
	    is_identifier(token, "int"))
	{
		result = VariableType_int32;
	} else if (is_identifier(token, "uint32_t")) {
		result = VariableType_uint32;
	} else if (is_identifier(token, "int16_t")) {
		result = VariableType_int16;
	} else if (is_identifier(token, "uint16_t")) {
		result = VariableType_uint16;
	} else if (is_identifier(token, "Resolution")) {
		result = VariableType_resolution;
	} else if (is_identifier(token, "VideoSettings")) {
		result = VariableType_video_settings;
	}

	return result;
}

#define CASE_RETURN_ENUM_STR(c)         case c: return #c;
#define DEFAULT_CASE_RETURN_ENUM_STR(c) default: return #c;

const char *variable_type_str(VariableType type)
{
	switch (type) {
	CASE_RETURN_ENUM_STR(VariableType_int32);
	CASE_RETURN_ENUM_STR(VariableType_uint32);
	CASE_RETURN_ENUM_STR(VariableType_int16);
	CASE_RETURN_ENUM_STR(VariableType_uint16);

	CASE_RETURN_ENUM_STR(VariableType_resolution);
	CASE_RETURN_ENUM_STR(VariableType_video_settings);
	CASE_RETURN_ENUM_STR(VariableType_settings);

	DEFAULT_CASE_RETURN_ENUM_STR(VariableType_unknown);
	}
}

int main(int argc, char **argv)
{
	VAR_UNUSED(argc);
	VAR_UNUSED(argv);

	FILE *output_file = fopen("C:/Users/grouse/projects/leary/src/generated/meta_data.h", "w");
	if (!output_file) {
		return 0;
	}

	std::fprintf(output_file, "#ifndef META_DATA_H\n");
	std::fprintf(output_file, "#define META_DATA_H\n\n");

	std::fprintf(output_file, "enum VariableType\n{\n");
	for (int32_t i = 0; i < VariableType_num; i++) {
		std::fprintf(output_file, "\t%s", variable_type_str((VariableType)i));

		if (i == (VariableType_num - 1)) {
			std::fprintf(output_file, "\n");
		} else {
			std::fprintf(output_file, ",\n");
		}
	}
	std::fprintf(output_file, "};\n\n");

	std::fprintf(output_file, "struct StructMemberMetaData\n{\n");
	std::fprintf(output_file, "\tVariableType variable_type;\n");
	std::fprintf(output_file, "\tconst char   *variable_name;\n");
	std::fprintf(output_file, "\tsize_t       offset;\n");
	std::fprintf(output_file, "};\n\n");

	size_t size;
	char *file = read_entire_file("C:/Users/grouse/projects/leary/src/core/settings.h", &size);

	if (file == nullptr) {
		return 0;
	}

	Tokenizer tokenizer;
	tokenizer.at = file;

	while (tokenizer.at[0])
	{
		Token token = next_token(tokenizer);

		if (token.type != TokenType_eof) {
			if (is_identifier(token, "INTROSPECT")) {
				next_token(tokenizer); // eat struct

				Token struct_name = next_token(tokenizer);
				DEBUG_ASSERT(struct_name.type == TokenType_identifier);

				if (next_token(tokenizer).type == TokenType_open_curly_brace) {
					token = next_token(tokenizer);

					std::fprintf(output_file, 
					             "StructMemberMetaData %.*s_MemberMetaData[] = {\n",
					             struct_name.length, struct_name.str);

					while (token.type == TokenType_identifier) {
						Token member_type = token;
						Token member_name = next_token(tokenizer);

						const char *member_type_str =
							variable_type_str(variable_type(member_type));

						std::fprintf(output_file, 
						             "\t{ %s, \"%.*s\", offsetof(%.*s, %.*s) }",
						             member_type_str,
						             member_name.length, member_name.str,
						             struct_name.length, struct_name.str,
						             member_name.length, member_name.str);
						do {
							token = next_token(tokenizer);
						} while (token.type != TokenType_semicolon);

						token = next_token(tokenizer);

						if (token.type == TokenType_close_curly_brace) {
							std::fprintf(output_file, "\n");
						} else {
							std::fprintf(output_file, ",\n");
						}
					}

					DEBUG_ASSERT(token.type == TokenType_close_curly_brace);
					std::fprintf(output_file, "};\n\n");

					token = next_token(tokenizer);
				}
			}
		}
	}

	std::fprintf(output_file, "#endif // META_DATA_H\n");
	fclose(output_file);

	return 0;
}

