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

#include "leary.h"
#include "platform/platform.h"

#include "core/lexer.cpp"
#include "core/allocator.cpp"
#include "core/array.cpp"

SystemAllocator *g_system;
LinearAllocator *g_debug_frame;

#if defined(_WIN32)
    #include "platform/win32_debug.cpp"
    #include "platform/win32_file.cpp"
#elif defined(__linux__)
    #include "platform/linux_debug.cpp"
    #include "platform/linux_file.cpp"
#else
    #error "unsupported platform"
#endif


enum VariableType {
    VariableType_int32,
    VariableType_uint32,
    VariableType_int16,
    VariableType_uint16,

    VariableType_f32,

    VariableType_carray,
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
    Array<TypeInfo> members;
};

struct Parameter {
    char *type;
    char *name;
};

struct ArrayFunction {
    char *ret;
    char *fname;
    Array<Parameter> params;
    char *body;
};

struct ArrayStruct {
    char *name;
    char *body;
};

struct PreprocessorOutput {
    Array<StructInfo>    structs;
    Array<char*>         arrays;
    Array<char*>         sarrays;
    Array<ArrayFunction> afuncs;
    Array<ArrayStruct>   astructs;
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


    CASE_RETURN_ENUM_STR(VariableType_carray);
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

void skip_struct_function(Lexer *lexer)
{
    Token token;

    i32 curly = 0;
    i32 paren = 0;
    do {
        token = next_token(lexer);

        if (token.type == Token::semicolon) {
            break;
        }

        if (token.type == Token::open_paren) {
            paren++;
            do {
                token = next_token(lexer);
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
                token = next_token(lexer);
                if (token.type == Token::open_curly_brace) {
                    curly++;
                } else if (token.type == Token::close_curly_brace) {
                    curly--;
                }
            } while (curly > 0);

            break;
        }
    } while (true);
}

void parse_array_type(Lexer lexer, Array<char*> *types)
{
    Token t = next_token(&lexer);
    assert(t.type == Token::open_paren);

    t = next_token(&lexer);

    if (t.str[0] == 'T') {
        return;
    }

    for (i32 i = 0; i < types->count; i++) {
        if (strncmp((*types)[i], t.str, t.length) == 0) {
            return;
        }
    }

    char *tn = string_duplicate(t.str, t.length);
    array_add(types, tn);
}

void parse_array_struct(Lexer l, PreprocessorOutput *out)
{
    Token t;
    ArrayStruct s = {};

    t = next_token(&l);
    assert(is_identifier(t, "struct"));

    t = next_token(&l);
    s.name = string_duplicate(t.str, t.length);

    t = next_token(&l);
    assert(t.type == Token::open_curly_brace);

    Token start = t;

    i32 curly = 1;
    while (curly > 0) {
        t = next_token(&l);

        if (t.type == Token::open_curly_brace) {
            curly++;
        } else if (t.type == Token::close_curly_brace) {
            curly--;
        }
    }

    s.body = string_duplicate(start.str, (isize)((uptr)t.str + t.length - (uptr)start.str));
    array_add(&out->astructs, s);
}

void parse_array_function(Lexer lexer,
                          PreprocessorOutput *output,
                          Allocator *allocator)
{
    Token t, rettype;

    ArrayFunction f = {};
    f.params = create_array<Parameter>(allocator);

    rettype = next_token(&lexer);
    if (is_identifier(rettype, "ARRAY")) {
        t = next_token(&lexer);
        assert(t.type == Token::open_paren);

        t = next_token(&lexer);

        t = next_token(&lexer);
        assert(t.type == Token::close_paren);

        rettype.length = (isize)((uptr)t.str + t.length - (uptr)rettype.str);
    }
    f.ret = string_duplicate(rettype.str, rettype.length);

    t = next_token(&lexer);
    f.fname = string_duplicate(t.str, t.length);

    t = next_token(&lexer);
    assert(t.type == Token::open_paren);

    while (t.type != Token::close_paren) {
        t = next_token(&lexer);
        Token type = t;

        t = next_token(&lexer);

        if (t.type == Token::open_paren) {
            do t = next_token(&lexer);
            while (t.type != Token::close_paren);
            t = next_token(&lexer);
        }

        if (t.type == Token::asterisk) {
            type.length = (isize)((uptr)t.str + t.length - (uptr)type.str);
            t = next_token(&lexer);
        }

        Token name = t;
        Parameter p = {};
        p.type = string_duplicate(type.str, type.length);
        p.name = string_duplicate(name.str, name.length);
        array_add(&f.params, p);

        t = next_token(&lexer);
        while (t.type != Token::comma && t.type != Token::close_paren) {
            t = next_token(&lexer);
        }
    }

    t = next_token(&lexer);
    assert(t.type == Token::open_curly_brace);

    i32 curly = 1;

    Token body = t;
    do {
        t = next_token(&lexer);
        if (t.type == Token::open_curly_brace) {
            curly++;
        } else if (t.type == Token::close_curly_brace) {
            curly--;
        }
    } while (curly > 0);
    body.length = (isize)((uptr)t.str + t.length - (uptr)body.str);
    f.body = string_duplicate(body.str, body.length);

    array_add(&output->afuncs, f);
}

void parse_struct_type_info(Lexer lexer, PreprocessorOutput *output)
{
    StructInfo struct_info = {};

    Token token = next_token(&lexer);
    assert(token.type == Token::identifier);

    struct_info.name = string_duplicate(token.str, token.length);

    do token = next_token(&lexer);
    while (token.type != Token::open_curly_brace);

    struct_info.members = create_array<TypeInfo>(output->structs.allocator);

    do {
        Lexer line_start = lexer;

        token = next_token(&lexer);

        if (is_identifier(token, "static")) {
            continue;
        }

        if (is_identifier(token, "inline")) {
            skip_struct_function(&lexer);

            token = peek_next_token(lexer);
            continue;
        }

        VariableType type = variable_type(token);
        do token = next_token(&lexer);
        while (token.type != Token::identifier);

        if (is_identifier(token, "operator")) {
            skip_struct_function(&lexer);

            token = peek_next_token(lexer);
            continue;
        }

        if (is_identifier(token, "ARRAY")) {
            parse_array_type(line_start, &output->arrays);
            do token = next_token(&lexer);
            while (token.type != Token::semicolon);
            continue;
        }

        if (is_identifier(token, "SARRAY")) {
            parse_array_type(line_start, &output->arrays);
            do token = next_token(&lexer);
            while (token.type != Token::semicolon);
            continue;
        }

        TypeInfo tinfo = {};
        tinfo.name = string_duplicate(token.str, token.length);
        tinfo.type = type;

        i32 i = array_add(&struct_info.members, tinfo);

        Token next = peek_next_token(lexer);
        if (next.type == Token::open_paren) {
            skip_struct_function(&line_start);
            lexer = line_start;

            array_remove(&struct_info.members, i);

            token = peek_next_token(lexer);
            continue;
        } else if (next.type == Token::comma ||
                   next.type == Token::open_square_brace)
        {
            token = next_token(&lexer);
            do {
                if (token.type == Token::comma) {
                    token = next_token(&lexer);

                    tinfo.name = string_duplicate(token.str, token.length);
                    array_add(&struct_info.members, tinfo);
                } else if (token.type == Token::open_square_brace) {
                    token = next_token(&lexer);
                    assert(token.type != Token::close_square_brace);

                    i64 size = read_i64(token);

                    VariableType underlying = struct_info.members[i].type;
                    struct_info.members[i].type             = VariableType_carray;
                    struct_info.members[i].array.underlying = underlying;
                    struct_info.members[i].array.size       = size;

                    token = next_token(&lexer);
                    assert(token.type == Token::close_square_brace);
                }
                token = next_token(&lexer);
            } while (token.type != Token::semicolon);
        } else {
            do {
                token = next_token(&lexer);
            } while (token.type != Token::semicolon);
        }

        token = peek_next_token(lexer);
    } while (token.type == Token::identifier);

    array_add(&output->structs, struct_info);
}

int main(int argc, char **argv)
{
    SystemAllocator allocator = {};
    g_system = &allocator;

    isize debug_frame_size = 64  * 1024 * 1024;
    void *debug_frame_mem = malloc(debug_frame_size);
    g_debug_frame = new LinearAllocator(debug_frame_mem, debug_frame_size);

    init_paths(g_system);

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
            output_path = resolve_relative(argv[++i]);
        } else if (strcmp(argv[i], "-r") == 0 ||
                   strcmp(argv[i], "--root") == 0)
        {
            input_root = resolve_relative(argv[++i]);
        } else {
            printf("%s: invalid option: %s\n", exe_name, argv[i]);
            break;
        }
    }

    if (output_path == nullptr || input_root == nullptr) {
        printf("Usage: %s -o|--output PATH -r|--root PATH\n", exe_name);
        return 0;
    }

    char *type_info_path = (char*)malloc(strlen(output_path) +
                                         strlen(FILE_SEP "type_info.h") + 1);
    strcpy(type_info_path, output_path);
    strcat(type_info_path, FILE_SEP "type_info.h");

    FILE *type_info_file = fopen(type_info_path, "w");
    if (!type_info_file) {
        return 0;
    }

    fprintf(type_info_file, "#ifndef TYPE_INFO_H\n");
    fprintf(type_info_file, "#define TYPE_INFO_H\n\n");

    fprintf(type_info_file, "enum VariableType {\n");
    for (i32 i = 0; i < VariableType_num; i++) {
        fprintf(type_info_file, "\t%s", variable_type_str((VariableType)i));

        if (i == (VariableType_num - 1)) {
            fprintf(type_info_file, "\n");
        } else {
            fprintf(type_info_file, ",\n");
        }
    }
    fprintf(type_info_file, "};\n\n");

    fprintf(type_info_file, "struct ArrayTypeInfo {\n");
    fprintf(type_info_file, "\tVariableType underlying;\n");
    fprintf(type_info_file, "\tisize size;\n");
    fprintf(type_info_file, "};\n\n");

    fprintf(type_info_file, "struct StructMemberInfo {\n");
    fprintf(type_info_file, "\tVariableType type;\n");
    fprintf(type_info_file, "\tconst char   *name;\n");
    fprintf(type_info_file, "\tusize        offset;\n");
    fprintf(type_info_file, "\tArrayTypeInfo array;\n");
    fprintf(type_info_file, "};\n\n");

    const char *files[] = {
        FILE_SEP "platform" FILE_SEP "platform.h",
        FILE_SEP "core" FILE_SEP "math.h",
        FILE_SEP "core" FILE_SEP "array.h",
        FILE_SEP "core" FILE_SEP "array.cpp",
        FILE_SEP "vulkan_render.cpp",
        FILE_SEP "vulkan_render.h",
        FILE_SEP "leary.cpp"
    };

    PreprocessorOutput output = {};
    output.structs  = create_array<StructInfo>(&allocator);
    output.arrays   = create_array<char*>(&allocator);
    output.sarrays  = create_array<char*>(&allocator);
    output.afuncs   = create_array<ArrayFunction>(&allocator);
    output.astructs = create_array<ArrayStruct>(&allocator);


    i32 num_files = ARRAY_SIZE(files);

    for (i32 i = 0; i < num_files; ++i) {
        char *file_path = (char*)malloc(strlen(input_root) +
                                        strlen(files[i]) + 1);
        defer { free(file_path); };

        strcpy(file_path, input_root);
        strcat(file_path, files[i]);

        usize size;
        char *file = read_file(file_path, &size, g_system);

        if (file == nullptr) {
            return 0;
        }

        Lexer lexer = create_lexer(file, size);

        Token prev;
        Token token = next_token(&lexer);

        while (lexer.at < lexer.end) {

            if (is_identifier(token, "INTROSPECT") &&
                is_identifier(next_token(&lexer), "struct"))
            {
                parse_struct_type_info(lexer, &output);
#if 0
            } else if ((is_identifier(token, "ARRAY") ||
                        is_identifier(token, "ARRAY_CREATE")) &&
                       !is_identifier(prev, "define"))
            {
                parse_array_type(lexer, &output.arrays);
            } else if ((is_identifier(token, "SARRAY") ||
                        is_identifier(token, "SARRAY_CREATE")) &&
                       !is_identifier(prev, "define"))
            {
                parse_array_type(lexer, &output.sarrays);
            } else if (is_identifier(token, "ARRAY_TEMPLATE") &&
                       !is_identifier(prev, "define"))
            {
                if (is_identifier(peek_next_token(&lexer), "struct")) {
                    parse_array_struct(lexer, &output);
                } else {
                    parse_array_function(lexer, &output, &allocator);
                }
#endif
            }

            prev = token;
            token = next_token(&lexer);
        }
    }

    for (i32 i = 0; i < output.structs.count; i++) {
        StructInfo &struct_info = output.structs[i];

        fprintf(type_info_file,
                "StructMemberInfo %s_members[] = {\n",
                struct_info.name);
        for (i32 j = 0; j < struct_info.members.count; ++j) {
            TypeInfo &type_info = struct_info.members[j];

            if (type_info.type == VariableType_carray) {
                fprintf(type_info_file,
                        "\t{ %s, \"%s\", offsetof(%s, %s), { %s, %d } },\n",
                        variable_type_str(type_info.type),
                        type_info.name,
                        struct_info.name,
                        type_info.name,
                        variable_type_str(type_info.array.underlying),
                        type_info.array.size);
            } else {
                fprintf(type_info_file,
                        "\t{ %s, \"%s\", offsetof(%s, %s), {} },\n",
                        variable_type_str(type_info.type),
                        type_info.name,
                        struct_info.name,
                        type_info.name);
            }
        }

        fprintf(type_info_file, "};\n\n");
    }

#if 0
    char *ah_path = (char*)malloc(strlen(output_path) + strlen(FILE_SEP "array.h") + 1);
    char *ac_path = (char*)malloc(strlen(output_path) + strlen(FILE_SEP "array.cpp") + 1);

    defer {
        free(ah_path);
        free(ac_path);
    };

    strcpy(ah_path, output_path);
    strcat(ah_path, FILE_SEP "array.h");

    strcpy(ac_path, output_path);
    strcat(ac_path, FILE_SEP "array.cpp");

    FILE *ah_file = fopen(ah_path, "w");
    if (!ah_file) return 0;
    defer { fclose(ah_file); };

    FILE *ac_file = fopen(ac_path, "w");
    if (!ac_file) return 0;
    defer { fclose(ac_file); };


    for (i32 i = 0; i < output.arrays.count; i++) {
        char *t = output.arrays[i];

        for (i32 j = 0; j < output.afuncs.count; j++) {
            ArrayFunction &f = output.afuncs[j];

            if (strncmp("ARRAY", f.ret, strlen("ARRAY")) == 0) {
                fprintf(ac_file, "Array_%s ", t);
            } else {
                fprintf(ac_file, "%s ", f.ret);
            }

            if (strcmp("array_create", f.fname) == 0) {
                fprintf(ac_file, "%s_%s(", f.fname, t);
            } else {
                fprintf(ac_file, "%s(", f.fname);
            }

            if (f.params.count > 1) {
                for (i32 k = 0; k < f.params.count - 1; k++) {
                    Parameter &p = f.params[k];

                    if (strncmp("ARRAY", p.type, strlen("ARRAY")) == 0) {
                        fprintf(ac_file, "Array_%s *%s, ", t, p.name);
                    } else if (strcmp("T", p.type) == 0) {
                        fprintf(ac_file, "%s %s", t, p.name);
                    } else {
                        fprintf(ac_file, "%s %s, ", p.type, p.name);
                    }
                }
            }

            Parameter &p = f.params[f.params.count - 1];

            if (strncmp("ARRAY", p.type, strlen("ARRAY")) == 0) {
                fprintf(ac_file, "Array_%s *%s", t, p.name);
            } else if (strcmp("T", p.type) == 0) {
                fprintf(ac_file, "%s %s", t, p.name);
            } else {
                fprintf(ac_file, "%s %s", p.type, p.name);
            }

            fprintf(ac_file, ")\n");

            Tokenizer tn = make_lexer(f.body, strlen(f.body));

            Token tk = next_token(tn);
            assert(tk.type == Token::open_curly_brace);

            char *s = f.body;
            i32 curly = 1;

            while (curly > 0) {
                tk = next_token(tn);

                if (is_identifier(tk, "ARRAY")) {
                    fprintf(ac_file, "%.*s", (i32)((uptr)tk.str - (uptr)s), s);
                    fprintf(ac_file, "Array_%s", t);

                    s = tk.str + tk.length + strlen("(T)");
                }

                if (tk.type == Token::open_curly_brace) {
                    curly++;
                } else if (tk.type == Token::close_curly_brace) {
                    curly--;
                }
            }

            fprintf(ac_file, "%.*s\n\n", (i32)((uptr)f.body + strlen(f.body) - (uptr)s), s);
        }

        for (i32 j = 0; j < output.astructs.count; j++) {
            ArrayStruct &s = output.astructs[j];

            fprintf(ah_file, "struct %s_%s ", s.name, t);

            Tokenizer tn = make_tokenizer(s.body, strlen(s.body));

            Token tk = next_token(tn);
            assert(tk.type == Token::open_curly_brace);

            char *str = s.body;
            i32 curly = 1;
            while (curly > 0) {
                tk = next_token(tn);

                if (is_identifier(tk, "T")) {
                    fprintf(ah_file, "%.*s", (i32)((uptr)tk.str - (uptr)str), str);
                    fprintf(ah_file, "%s", t);

                    str = tk.str + tk.length;
                }

                if (tk.type == Token::open_curly_brace) {
                    curly++;
                } else if (tk.type == Token::close_curly_brace) {
                    curly--;
                }
            }

            fprintf(ah_file, "%.*s;\n\n", (i32)((uptr)s.body + strlen(s.body) - (uptr)str), str);
        }
    }

    for (i32 i = 0; i < output.arrays.count; i++) {
        printf("Array_%s\n", output.arrays[i]);
    }

    for (i32 i = 0; i < output.sarrays.count; i++) {
        printf("StaticArray_%s\n", output.sarrays[i]);
    }
#endif

    fprintf(type_info_file, "#endif // TYPE_INFO\n");
    fclose(type_info_file);

    return 0;
}

