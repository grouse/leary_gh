/**
 * file:    serialize.cpp
 * created: 2016-11-20
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2016 - all rights reserved
 */

#include <inttypes.h>
#include "generated/type_info.h"

i32
member_to_string(StructMemberInfo &member,
                 void *ptr,
                 char *buffer,
                 i32 size)
{
    i32 bytes = 0;

    switch (member.type) {
    case VariableType_int32: {
        i32 value = *(i32*)(((char*)ptr) + member.offset);
        bytes = sprintf(buffer, "%s = %d", member.name, value);

        assert(bytes < size);
        break;
    }
    case VariableType_uint32: {
        u32 value = *(u32*)(((char*)ptr) + member.offset);
        bytes = sprintf(buffer, "%s = %u", member.name, value);

        assert(bytes < size);
        break;
    }
    case VariableType_int16: {
        i16 value = *(i16*)(((char*)ptr) + member.offset);
        bytes = sprintf(buffer, "%s = %" PRId16 , member.name, value);

        assert(bytes < size);
        break;
    }
    case VariableType_uint16: {
        u16 value = *(u16*)(((char*)ptr) + member.offset);
        bytes = sprintf(buffer, "%s = %" PRIu16, member.name, value);

        assert(bytes < size);
        break;
    }
    case VariableType_resolution: {
        i32 child_bytes = sprintf(buffer, "%s = { ", member.name);

        bytes  += child_bytes;
        buffer += child_bytes;

        i32 num_members = (i32)(sizeof(Resolution_members) /
                                        sizeof(StructMemberInfo));

        for (i32 i = 0; i < (i32)num_members; i++) {
            StructMemberInfo &child = Resolution_members[i];
            child_bytes = member_to_string(child, ptr, buffer, size);

            bytes  += child_bytes;
            buffer += child_bytes;

            if (i != (num_members - 1)) {
                child_bytes = sprintf(buffer, ", ");

                bytes  += child_bytes;
                buffer += child_bytes;
            }

            assert(bytes < size);
        }

        child_bytes = sprintf(buffer, " }");

        bytes  += child_bytes;
        buffer += child_bytes;

        break;
    }
    case VariableType_video_settings: {
        i32 child_bytes = sprintf(buffer, "%s = { ", member.name);

        bytes  += child_bytes;
        buffer += child_bytes;

        i32 num_members = (i32)(sizeof(VideoSettings_members) /
                                        sizeof(StructMemberInfo));

        for (i32 i = 0; i < (i32)num_members; i++) {
            StructMemberInfo &child = VideoSettings_members[i];
            child_bytes = member_to_string( child, ptr, buffer, size);

            bytes  += child_bytes;
            buffer += child_bytes;

            if (i != (num_members - 1)) {
                child_bytes = sprintf(buffer, ", ");

                bytes  += child_bytes;
                buffer += child_bytes;
            }

            assert(bytes < size);
        }

        child_bytes = sprintf(buffer, "}");

        bytes  += child_bytes;
        buffer += child_bytes;

        break;
    }
    default: {
        bytes = sprintf(buffer, "unknown type");
        assert(bytes < size);
        break;
    }
    }

    return bytes;
}

void serialize_save_conf(char *path,
                         StructMemberInfo *members,
                         i32 num_members,
                         void *ptr)
{
    if (!file_exists(path) && !create_file(path)) {
        LOG(Log_warning, "path does not exist or can't be created: %s",
                  path);
        assert(false);
        return;
    }

    void *file_handle = open_file(path, FileAccess_write);

    // TODO(jesper): rewrite this so that we do fewer file_write, potentially
    // so that we only open file and write into it after we've got an entire
    // buffer to write into it
    char buffer[2048];

    for (i32 i = 0; i < (i32)num_members; i++) {
        StructMemberInfo &member = members[i];
        i32 bytes = member_to_string(member, ptr, buffer, sizeof(buffer));

        write_file(file_handle, buffer, (usize)bytes);
    }

    close_file(file_handle);
}

StructMemberInfo *
find_member(char *name,
            i32 length,
            StructMemberInfo *members,
            usize num_members)
{
    for (i32 i = 0; i < (i32)num_members; i++) {
        if (strncmp(name, members[i].name, length) == 0) {
            return &members[i];
        }
    }

    return nullptr;
}


void
member_from_string(char **ptr,
                   usize size,
                   StructMemberInfo *members,
                   usize num_members,
                   void *out)
{
    Lexer lexer = create_lexer(*ptr, size);

    Token token = next_token(&lexer);
    while (token.type != Token::eof &&
           token.type != Token::close_curly_brace)
    {
        assert(token.type == Token::identifier);
        Token name = token;

        StructMemberInfo *member = find_member(name.str, name.length,
                                               members, num_members);
        assert(member != nullptr);

        token = next_token(&lexer);
        assert(token.type == Token::equals);

        switch (member->type) {
        case VariableType_int32: {
            token = next_token(&lexer);
            i64 value = read_i64(token);
            *(i32*)((u8*)out + member->offset) = (i32)value;
        } break;
        case VariableType_uint32: {
            token = next_token(&lexer);
            u64 value = read_u64(token);
            *(u32*)((u8*)out + member->offset) = (u32)value;
        } break;
        case VariableType_int16: {
            token = next_token(&lexer);
            i64 value = read_i64(token);
            *(i16*)((u8*)out + member->offset) = (i16)value;
        } break;
        case VariableType_uint16: {
            token = next_token(&lexer);
            u64 value = read_u64(token);
            *(u16*)((u8*)out + member->offset) = (u16)value;
        } break;
        case VariableType_resolution: {
            token = next_token(&lexer);
            assert(token.type == Token::open_curly_brace);

            void *child = ((u8*)out + member->offset);
            member_from_string(&lexer.at, lexer.end - lexer.at,
                               Resolution_members,
                               sizeof(Resolution_members) /
                               sizeof(StructMemberInfo),
                               child);
        } break;
        case VariableType_video_settings: {
            token = next_token(&lexer);
            assert(token.type == Token::open_curly_brace);

            void *child = ((u8*)out + member->offset);
            member_from_string(&lexer.at, lexer.end - lexer.at,
                               VideoSettings_members,
                               sizeof(VideoSettings_members) /
                               sizeof(StructMemberInfo),
                               child);
        } break;
        default:
            LOG(Log_unimplemented, "unhandled case: %d", member->type);
        }

        do token = next_token(&lexer);
        while (token.type == Token::comma);
    }

    *ptr = lexer.at;
}

void serialize_load_conf(char *path,
                         StructMemberInfo *members,
                         i32 num_members,
                         void *out)
{
    if (!file_exists(path)) {
        LOG(Log_warning, "path does not exist: %s", path);
        return;
    }

    usize size;
    char *file, *ptr;
    file = ptr = read_file(path, &size, g_frame);
    member_from_string(&ptr, size, members, num_members, out);
}

