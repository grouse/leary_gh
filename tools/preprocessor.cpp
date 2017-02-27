/**
 * file:    preprocessor.cpp
 * created: 2016-11-19
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2016 - all rights reserved
 */

#include <stdio.h>
#include <stdlib.h>
#include <memory>
#include <vector>
#include <cstring>

#define PROFILE_TIMERS_ENABLE 0

#include "platform/platform.h"

#if defined(_WIN32)
	#include "platform/win32_debug.cpp"
	#include "platform/win32_file.cpp"
#elif defined(__linux__)
	#include "platform/linux_debug.cpp"
	#include "platform/linux_file.cpp"
#else
	#error "unsupported platform"
#endif

#include "core/tokenizer.cpp"

enum VariableType {
	VariableType_int32,
	VariableType_uint32,
	VariableType_int16,
	VariableType_uint16,

	VariableType_f32,

	VariableType_resolution,
	VariableType_video_settings,
	VariableType_settings,

	VariableType_unknown,
	VariableType_num
};

struct TypeInfo {
	char *name;
	VariableType type;
};

struct StructInfo {
	char     *name;
	i32  num_members;
	TypeInfo *members;
};

char *string_duplicate(char *src, usize size)
{
	char *result = (char*)malloc(size + 1);

	char *dst = result;
	while (size && *src) {
		*dst++ = *src++;
		--size;
	}

	if (*dst != '\0') *dst = '\0';

	return result;
}

VariableType variable_type(Token token)
{
	VariableType result = VariableType_unknown;

	if (is_identifier(token, "int32_t") ||
	    is_identifier(token, "i32") ||
	    is_identifier(token, "int"))
	{
		result = VariableType_int32;
	} else if (is_identifier(token, "uint32_t") ||
	           is_identifier(token, "u32"))
	{
		result = VariableType_uint32;
	} else if (is_identifier(token, "int16_t") ||
	           is_identifier(token, "i16"))
	{
		result = VariableType_int16;
	} else if (is_identifier(token, "uint16_t") ||
	           is_identifier(token, "u16"))
	{
		result = VariableType_uint16;
	} else if (is_identifier(token, "f32")) {
		result = VariableType_f32;
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

	CASE_RETURN_ENUM_STR(VariableType_f32);

	CASE_RETURN_ENUM_STR(VariableType_resolution);
	CASE_RETURN_ENUM_STR(VariableType_video_settings);
	CASE_RETURN_ENUM_STR(VariableType_settings);

	DEFAULT_CASE_RETURN_ENUM_STR(VariableType_unknown);
	}
}

StructInfo parse_struct_type_info(Tokenizer tokenizer)
{
	StructInfo struct_info = {};

	Token token = next_token(tokenizer);
	DEBUG_ASSERT(token.type == Token::identifier);

	struct_info.name = string_duplicate(token.str, token.length);

	do token = next_token(tokenizer);
	while (token.type != Token::open_curly_brace);

	Tokenizer tmp = tokenizer;
	do {
		next_token(tmp);

		do token = next_token(tmp);
		while (token.type != Token::identifier);

		++struct_info.num_members;

		do {
			token = next_token(tmp);
			if (token.type == Token::comma) {
				++struct_info.num_members;
			}
		} while (token.type != Token::semicolon);

		token = peek_next_token(tmp);
	} while (token.type == Token::identifier);

	struct_info.members = new TypeInfo[struct_info.num_members];
	i32 index = 0;

	do {
		VariableType member_type = variable_type(next_token(tokenizer));

		do token = next_token(tokenizer);
		while (token.type != Token::identifier);
		char *member_name = string_duplicate(token.str, token.length);

		struct_info.members[index].name = member_name;
		struct_info.members[index].type = member_type;
		++index;

		do {
			token = next_token(tokenizer);
			if (token.type == Token::comma) {
				token = next_token(tokenizer);

				member_name = string_duplicate(token.str, token.length);
				struct_info.members[index].name = member_name;
				struct_info.members[index].type = member_type;
				++index;
			}
		} while (token.type != Token::semicolon);

		token = peek_next_token(tokenizer);
	} while (token.type == Token::identifier);

	return struct_info;
}

int main(int argc, char **argv)
{
	char *output_path = nullptr;
	char *input_root  = nullptr;

	char *exe_name = nullptr;
	char *ptr = argv[0];
	while (*ptr) {
		if (strcmp(ptr, FILE_SEP) == 0) {
			exe_name = ++ptr;
		} else {
			ptr++;
		}
	}

	for (i32 i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
			output_path = platform_resolve_path(argv[++i]);
		} else if (strcmp(argv[i], "-r") == 0 ||
		           strcmp(argv[i], "--root") == 0)
		{
			input_root = platform_resolve_path(argv[++i]);
		} else {
			std::printf("%s: invalid option: %s\n", exe_name, argv[i]);
			break;
		}
	}

	if (output_path == nullptr || input_root == nullptr) {
		std::printf("Usage: %s -o|--output PATH -r|--root PATH\n", exe_name);
		return 0;
	}

	char *output_file_path = (char*)malloc(strlen(output_path) +
	                                       strlen(FILE_SEP "type_info.h") + 1);

	char *dst = output_file_path;

	const char *src = output_path;
	while (*src) *dst++ = *src++;

	src = FILE_SEP "type_info.h";
	while (*src) *dst++ = *src++;
	*(dst+1) = '\0';

	FILE *output_file = fopen(output_file_path, "w");
	if (!output_file) {
		return 0;
	}

	std::fprintf(output_file, "#ifndef TYPE_INFO_H\n");
	std::fprintf(output_file, "#define TYPE_INFO_H\n\n");

	std::fprintf(output_file, "enum VariableType {\n");
	for (i32 i = 0; i < VariableType_num; i++) {
		std::fprintf(output_file, "\t%s", variable_type_str((VariableType)i));

		if (i == (VariableType_num - 1)) {
			std::fprintf(output_file, "\n");
		} else {
			std::fprintf(output_file, ",\n");
		}
	}
	std::fprintf(output_file, "};\n\n");

	std::fprintf(output_file, "struct StructMemberInfo {\n");
	std::fprintf(output_file, "\tVariableType type;\n");
	std::fprintf(output_file, "\tconst char   *name;\n");
	std::fprintf(output_file, "\tusize        offset;\n");
	std::fprintf(output_file, "};\n\n");

	const char *files[] = {
		FILE_SEP "core" FILE_SEP "settings.h",
		FILE_SEP "core" FILE_SEP "math.h"
	};

	std::vector<StructInfo> struct_infos;

	i32 num_files = (i32)sizeof(files) / sizeof(files[0]);

	for (i32 i = 0; i < num_files; ++i) {
		char *file_path = (char*)malloc(strlen(input_root) +
		                                strlen(files[i]) + 1);

		dst = file_path;

		src = input_root;
		while(*src) *dst++ = *src++;

		src = files[i];
		while(*src) *dst++ = *src++;
		*(dst) = '\0';

		usize size;
		char *file = platform_file_read(file_path, &size);

		if (file == nullptr) {
			return 0;
		}

		Tokenizer tokenizer;
		tokenizer.at = file;

		while (tokenizer.at[0]) {
			Token token = next_token(tokenizer);

			if (token.type != Token::eof) {
				if (is_identifier(token, "INTROSPECT") &&
				    is_identifier(next_token(tokenizer), "struct"))
				{
					struct_infos.push_back(parse_struct_type_info(tokenizer));
				}
			}
		}
		free(file_path);
	}

	for (auto &struct_info : struct_infos) {
		std::fprintf(output_file,
				 	 "StructMemberInfo %s_members[] = {\n",
				 	 struct_info.name);
		for (i32 j = 0; j < struct_info.num_members; ++j) {
			TypeInfo &type_info = struct_info.members[j];
			std::fprintf(output_file,
					 	 "\t{ %s, \"%s\", offsetof(%s, %s) },\n",
					 	 variable_type_str(type_info.type),
					 	 type_info.name,
					 	 struct_info.name,
					 	 type_info.name);
		}

		std::fprintf(output_file, "};\n\n");
	}

	std::fprintf(output_file, "#endif // TYPE_INFO\n");
	fclose(output_file);

	return 0;
}

