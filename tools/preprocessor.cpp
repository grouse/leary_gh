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
#include "core/allocator.cpp"
#include "core/array.cpp"

enum VariableType {
	VariableType_int32,
	VariableType_uint32,
	VariableType_int16,
	VariableType_uint16,

	VariableType_f32,

	VariableType_array,

	// TODO(jesper): come up with a better system for these.
	// Meta program the meta program? dawg!
	VariableType_resolution,
	VariableType_video_settings,
	VariableType_settings,
	VariableType_Vector4,

	VariableType_unknown,
	VariableType_num
};

struct ArrayTypeInfo {
	VariableType underlying;
	i32 size;
};

struct TypeInfo {
	char *name;
	VariableType type;
	union {
		ArrayTypeInfo   array;
	};
};

struct StructInfo {
	char *name;
	Array<TypeInfo, SystemAllocator> members;
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
	// TODO(jesper): come up with a better system for these.
	// Meta program the meta program? dawg!
	} else if (is_identifier(token, "Resolution")) {
		result = VariableType_resolution;
	} else if (is_identifier(token, "VideoSettings")) {
		result = VariableType_video_settings;
	} else if (is_identifier(token, "Vector4")) {
		result = VariableType_Vector4;
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


	CASE_RETURN_ENUM_STR(VariableType_array);

	// TODO(jesper): come up with a better system for these.
	// Meta program the meta program? dawg!
	CASE_RETURN_ENUM_STR(VariableType_resolution);
	CASE_RETURN_ENUM_STR(VariableType_video_settings);
	CASE_RETURN_ENUM_STR(VariableType_settings);
	CASE_RETURN_ENUM_STR(VariableType_Vector4);

	DEFAULT_CASE_RETURN_ENUM_STR(VariableType_unknown);
	}
}

void skip_struct_function(Tokenizer &tokenizer)
{
	Token token;

	i32 curly = 0;
	i32 paren = 0;
	do {
		token = next_token(tokenizer);

		if (token.type == Token::semicolon) {
			break;
		}

		if (token.type == Token::open_paren) {
			paren++;
			do {
				token = next_token(tokenizer);
				if (token.type == Token::open_paren) {
					paren++;
				} else if (token.type == Token::close_paren) {
					paren--;
				}
			} while (paren > 0);

			continue;
		}

		if (token.type == Token::open_curly_brace) {
			curly++;
			do {
				token = next_token(tokenizer);
				if (token.type == Token::open_curly_brace) {
					curly++;
				} else if (token.type == Token::close_curly_brace) {
					curly--;
				}
			} while (curly > 0);

			continue;
		}
	} while (true);
}

void parse_struct_type_info(Tokenizer tokenizer,
                            Array<StructInfo, SystemAllocator> *struct_infos)
{
	StructInfo struct_info = {};

	Token token = next_token(tokenizer);
	DEBUG_ASSERT(token.type == Token::identifier);

	struct_info.name = string_duplicate(token.str, token.length);

	do token = next_token(tokenizer);
	while (token.type != Token::open_curly_brace);

	struct_info.members = make_array<TypeInfo>(struct_infos->allocator);

	do {
		Tokenizer line_start = make_tokenizer(tokenizer.at, tokenizer.size);

		token = next_token(tokenizer);

		if (is_identifier(token, "inline")) {
			skip_struct_function(tokenizer);

			token = peek_next_token(tokenizer);
			continue;
		}

		VariableType type = variable_type(token);
		do token = next_token(tokenizer);
		while (token.type != Token::identifier);

		if (is_identifier(token, "operator")) {
			skip_struct_function(tokenizer);

			token = peek_next_token(tokenizer);
			continue;
		}

		TypeInfo tinfo = {};
		tinfo.name = string_duplicate(token.str, token.length);
		tinfo.type = type;

		isize i = array_add(&struct_info.members, tinfo);

		Token next = peek_next_token(tokenizer);
		if (next.type == Token::open_paren) {
			skip_struct_function(line_start);
			tokenizer = line_start;

			token = peek_next_token(tokenizer);
			continue;
		} else if (next.type == Token::comma ||
		           next.type == Token::open_square_brace)
		{
			token = next_token(tokenizer);
			do {
				if (token.type == Token::comma) {
					token = next_token(tokenizer);

					tinfo.name = string_duplicate(token.str, token.length);
					array_add(&struct_info.members, tinfo);
				} else if (token.type == Token::open_square_brace) {
					token = next_token(tokenizer);
					DEBUG_ASSERT(token.type != Token::close_square_brace);

					i64 size = read_integer(token);

					VariableType underlying = struct_info.members[i].type;
					struct_info.members[i].type             = VariableType_array;
					struct_info.members[i].array.underlying = underlying;
					struct_info.members[i].array.size       = size;

					token = next_token(tokenizer);
					DEBUG_ASSERT(token.type == Token::close_square_brace);
				}
				token = next_token(tokenizer);
			} while (token.type != Token::semicolon);
		} else {
			do {
				token = next_token(tokenizer);
			} while (token.type != Token::semicolon);
		}

		token = peek_next_token(tokenizer);
	} while (token.type == Token::identifier);

	array_add(struct_infos, struct_info);
}

int main(int argc, char **argv)
{
	SystemAllocator allocator = {};

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
			output_path = platform_resolve_relative(argv[++i]);
		} else if (strcmp(argv[i], "-r") == 0 ||
		           strcmp(argv[i], "--root") == 0)
		{
			input_root = platform_resolve_relative(argv[++i]);
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

	std::fprintf(output_file, "struct ArrayTypeInfo {\n");
	std::fprintf(output_file, "\tVariableType underlying;\n");
	std::fprintf(output_file, "\tisize size;\n");
	std::fprintf(output_file, "};\n\n");

	std::fprintf(output_file, "struct StructMemberInfo {\n");
	std::fprintf(output_file, "\tVariableType type;\n");
	std::fprintf(output_file, "\tconst char   *name;\n");
	std::fprintf(output_file, "\tusize        offset;\n");
	std::fprintf(output_file, "\tArrayTypeInfo array;\n");
	std::fprintf(output_file, "};\n\n");

	const char *files[] = {
		//FILE_SEP "platform" FILE_SEP "platform.h",
		FILE_SEP "core" FILE_SEP "math.h"
	};

	auto struct_infos = make_array<StructInfo>(&allocator);

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

		Tokenizer tokenizer = make_tokenizer(file, size);

		while (tokenizer.at < tokenizer.end) {
			Token token = next_token(tokenizer);

			if (token.type != Token::eof) {
				if (is_identifier(token, "INTROSPECT") &&
				    is_identifier(next_token(tokenizer), "struct"))
				{
					parse_struct_type_info(tokenizer, &struct_infos);
				}
			}
		}
		free(file_path);
	}

	for (i32 i = 0; i < struct_infos.count; i++) {
		StructInfo &struct_info = struct_infos[i];

		std::fprintf(output_file,
		             "StructMemberInfo %s_members[] = {\n",
		             struct_info.name);
		for (i32 j = 0; j < struct_info.members.count; ++j) {
			TypeInfo &type_info = struct_info.members[j];

			if (type_info.type == VariableType_array) {
				std::fprintf(output_file,
				             "\t{ %s, \"%s\", offsetof(%s, %s), { %s, %d } },\n",
				             variable_type_str(type_info.type),
				             type_info.name,
				             struct_info.name,
				             type_info.name,
				             variable_type_str(type_info.array.underlying),
				             type_info.array.size);
			} else {
				std::fprintf(output_file,
				             "\t{ %s, \"%s\", offsetof(%s, %s), {} },\n",
				             variable_type_str(type_info.type),
				             type_info.name,
				             struct_info.name,
				             type_info.name);
			}
		}

		std::fprintf(output_file, "};\n\n");
	}

	std::fprintf(output_file, "#endif // TYPE_INFO\n");
	fclose(output_file);

	return 0;
}

