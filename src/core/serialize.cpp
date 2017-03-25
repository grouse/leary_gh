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
		bytes = std::sprintf(buffer, "%s = %d",
		                     member.name, value);

		DEBUG_ASSERT(bytes < size);
		break;
	}
	case VariableType_uint32: {
		u32 value = *(u32*)(((char*)ptr) + member.offset);
		bytes = std::sprintf(buffer, "%s = %u",
		                     member.name, value);

		DEBUG_ASSERT(bytes < size);
		break;
	}
	case VariableType_int16: {
		i16 value = *(i16*)(((char*)ptr) + member.offset);
		bytes = std::sprintf(buffer, "%s = %" PRId16 , member.name, value);

		DEBUG_ASSERT(bytes < size);
		break;
	}
	case VariableType_uint16: {
		u16 value = *(u16*)(((char*)ptr) + member.offset);
		bytes = std::sprintf(buffer, "%s = %" PRIu16,
		                     member.name, value);

		DEBUG_ASSERT(bytes < size);
		break;
	}
	case VariableType_resolution: {
		i32 child_bytes = std::sprintf(buffer, "%s = { ",
		                                   member.name);

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
				child_bytes = std::sprintf(buffer, ", ");

				bytes  += child_bytes;
				buffer += child_bytes;
			}

			DEBUG_ASSERT(bytes < size);
		}

		child_bytes = std::sprintf(buffer, " }");

		bytes  += child_bytes;
		buffer += child_bytes;

		break;
	}
	case VariableType_video_settings: {
		i32 child_bytes = std::sprintf(buffer, "%s = { ",
		                                   member.name);

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
				child_bytes = std::sprintf(buffer, ", ");

				bytes  += child_bytes;
				buffer += child_bytes;
			}

			DEBUG_ASSERT(bytes < size);
		}

		child_bytes = std::sprintf(buffer, "}");

		bytes  += child_bytes;
		buffer += child_bytes;

		break;
	}
	default: {
		bytes = std::sprintf(buffer, "unknown type");
		DEBUG_ASSERT(bytes < size);
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
	if (!platform_file_exists(path) && !platform_file_create(path)) {
		DEBUG_LOG(Log_warning, "path does not exist or can't be created: %s",
		          path);
		DEBUG_ASSERT(false);
		return;
	}

	void *file_handle = platform_file_open(path, FileAccess_write);

	// TODO(jesper): rewrite this so that we do fewer file_write, potentially
	// so that we only open file and write into it after we've got an entire
	// buffer to write into it
	char buffer[2048];

	for (i32 i = 0; i < (i32)num_members; i++) {
		StructMemberInfo &member = members[i];
		i32 bytes = member_to_string(member, ptr, buffer, sizeof(buffer));

		platform_file_write(file_handle, buffer, (usize)bytes);
	}

	platform_file_close(file_handle);
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

i64
read_integer(Token token)
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

u64
read_unsigned_integer(Token token)
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

void
member_from_string(char **ptr,
                   usize size,
                   StructMemberInfo *members,
                   usize num_members,
                   void *out)
{
	Tokenizer tokenizer = make_tokenizer(*ptr, size);

	Token token = next_token(tokenizer);
	while (token.type != Token::eof &&
	       token.type != Token::close_curly_brace)
	{
		DEBUG_ASSERT(token.type == Token::identifier);
		Token name = token;

		StructMemberInfo *member = find_member(name.str, name.length,
		                                       members, num_members);
		DEBUG_ASSERT(member != nullptr);

		token = next_token(tokenizer);
		DEBUG_ASSERT(token.type == Token::equals);

		switch (member->type) {
		case VariableType_int32: {
			token = next_token(tokenizer);
			i64 value = read_integer(token);
			*(i32*)((u8*)out + member->offset) = (i32)value;
		} break;
		case VariableType_uint32: {
			token = next_token(tokenizer);
			u64 value = read_unsigned_integer(token);
			*(u32*)((u8*)out + member->offset) = (u32)value;
		} break;
		case VariableType_int16: {
			token = next_token(tokenizer);
			i64 value = read_integer(token);
			*(i16*)((u8*)out + member->offset) = (i16)value;
		} break;
		case VariableType_uint16: {
			token = next_token(tokenizer);
			u64 value = read_unsigned_integer(token);
			*(u16*)((u8*)out + member->offset) = (u16)value;
		} break;
		case VariableType_resolution: {
			token = next_token(tokenizer);
			DEBUG_ASSERT(token.type == Token::open_curly_brace);

			void *child = ((u8*)out + member->offset);
			member_from_string(&tokenizer.at, tokenizer.end - tokenizer.at,
			                   Resolution_members,
			                   sizeof(Resolution_members) /
			                   sizeof(StructMemberInfo),
			                   child);
		} break;
		case VariableType_video_settings: {
			token = next_token(tokenizer);
			DEBUG_ASSERT(token.type == Token::open_curly_brace);

			void *child = ((u8*)out + member->offset);
			member_from_string(&tokenizer.at, tokenizer.end - tokenizer.at,
			                   VideoSettings_members,
			                   sizeof(VideoSettings_members) /
			                   sizeof(StructMemberInfo),
			                   child);
		} break;
		default:
			DEBUG_LOG(Log_unimplemented, "unhandled case: %d", member->type);
		}

		do token = next_token(tokenizer);
		while (token.type == Token::comma);
	}

	*ptr = tokenizer.at;
}

void serialize_load_conf(char *path,
                         StructMemberInfo *members,
                         i32 num_members,
                         void *out)
{
	if (!platform_file_exists(path)) {
		DEBUG_LOG(Log_warning, "path does not exist: %s", path);
		return;
	}

	usize size;
	char *file, *ptr;
	file = ptr = platform_file_read(path, &size);
	member_from_string(&ptr, size, members, num_members, out);
	free(file);
}

