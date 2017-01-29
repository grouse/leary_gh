/**
 * file:    serialize.cpp
 * created: 2016-11-20
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2016 - all rights reserved
 */

#include <inttypes.h>
#include "generated/meta_data.h"
#include "platform/debug.h"
#include "platform/file.h"

#define SERIALIZE_SAVE_CONF(file, name, ptr)              \
	serialize_save_conf(file, name ## _MemberMetaData,    \
	                    sizeof(name ## _MemberMetaData) / \
	                    sizeof(StructMemberMetaData),     \
	                    ptr)

#define SERIALIZE_LOAD_CONF(file, name, ptr)              \
	serialize_load_conf(file, name ## _MemberMetaData,    \
	                    sizeof(name ## _MemberMetaData) / \
	                    sizeof(StructMemberMetaData),     \
	                    ptr)
i32
member_to_string(StructMemberMetaData &member,
                 void *ptr,
                 char *buffer,
                 i32 size)
{
	i32 bytes = 0;

	switch (member.variable_type) {
	case VariableType_int32: {
		i32 value = *(i32*)(((char*)ptr) + member.offset);
		bytes = std::sprintf(buffer, "%s = %d",
		                     member.variable_name, value);

		DEBUG_ASSERT(bytes < size);
		break;
	}
	case VariableType_uint32: {
		u32 value = *(u32*)(((char*)ptr) + member.offset);
		bytes = std::sprintf(buffer, "%s = %u",
		                     member.variable_name, value);

		DEBUG_ASSERT(bytes < size);
		break;
	}
	case VariableType_int16: {
		i16 value = *(i16*)(((char*)ptr) + member.offset);
		bytes = std::sprintf(buffer, "%s = %" PRId16 , member.variable_name, value);

		DEBUG_ASSERT(bytes < size);
		break;
	}
	case VariableType_uint16: {
		u16 value = *(u16*)(((char*)ptr) + member.offset);
		bytes = std::sprintf(buffer, "%s = %" PRIu16,
		                     member.variable_name, value);

		DEBUG_ASSERT(bytes < size);
		break;
	}
	case VariableType_resolution: {
		i32 child_bytes = std::sprintf(buffer, "%s = { ",
		                                   member.variable_name);

		bytes  += child_bytes;
		buffer += child_bytes;

		i32 num_members = (i32)(sizeof(Resolution_MemberMetaData) /
		                                sizeof(StructMemberMetaData));

		for (i32 i = 0; i < (i32)num_members; i++) {
			StructMemberMetaData &child = Resolution_MemberMetaData[i];
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
		                                   member.variable_name);

		bytes  += child_bytes;
		buffer += child_bytes;

		i32 num_members = (i32)(sizeof(VideoSettings_MemberMetaData) /
		                                sizeof(StructMemberMetaData));

		for (i32 i = 0; i < (i32)num_members; i++) {
			StructMemberMetaData &child = VideoSettings_MemberMetaData[i];
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

void
serialize_save_conf(const char *path,
                   StructMemberMetaData *members,
                   size_t num_members,
                   void *ptr)
{
	if (!file_exists(path) && !file_create(path)) {
		DEBUG_ASSERT(false);
		return;
	}

	void *file_handle = file_open(path, FileMode::write);

	// TODO(jesper): rewrite this so that we do fewer file_write, potentially
	// so that we only open file and write into it after we've got an entire
	// buffer to write into it
	char buffer[2048];

	for (i32 i = 0; i < (i32)num_members; i++) {
		StructMemberMetaData &member = members[i];
		i32 bytes = member_to_string(member, ptr, buffer, sizeof(buffer));

		file_write(file_handle, buffer, (size_t)bytes);
	}

	file_close(file_handle);
}

StructMemberMetaData *
find_member(char *name,
            i32 length,
            StructMemberMetaData *members,
            size_t num_members)
{
	for (i32 i = 0; i < (i32)num_members; i++) {
		if (strncmp(name, members[i].variable_name, length) == 0) {
			return &members[i];
		}
	}

	return nullptr;
}

i64
read_integer(char **ptr)
{
	i64 result = 0;

	while (**ptr && **ptr >= '0' && **ptr <= '9') {
		result *= 10;
		result += (**ptr) - '0';
		(*ptr)++;
	}

	return result;
}

u64
read_unsigned_integer(char **ptr)
{
	u64 result = 0;

	while (**ptr && **ptr >= '0' && **ptr <= '9') {
		result *= 10;
		result += (**ptr) - '0';
		(*ptr)++;
	}

	return result;
}

void
member_from_string(char **ptr,
                   StructMemberMetaData *members,
                   size_t num_members,
                   void *out)
{
	while (**ptr) {
		while (**ptr && **ptr == ' ') (*ptr)++;
		if (**ptr == '}') return;

		char *name = *ptr;

		while (**ptr && **ptr != ' ') (*ptr)++;
		i32 name_length = (i32)((*ptr) - name);

		StructMemberMetaData *member = find_member(name, name_length,
		                                           members, num_members);

		DEBUG_ASSERT(member != nullptr);


		while (**ptr && **ptr != '=') (*ptr)++;
		(*ptr)++;
		while (**ptr && **ptr == ' ') (*ptr)++;


		switch (member->variable_type) {
		case VariableType_int32: {
			i64 value = read_integer(ptr);
			*(i32*)((u8*)out + member->offset) = (i32)value;
			break;
		}
		case VariableType_uint32: {
			u64 value = read_unsigned_integer(ptr);
			*(u32*)((u8*)out + member->offset) = (u32)value;
			break;
		}
		case VariableType_int16: {
			i64 value = read_integer(ptr);
			*(i16*)((u8*)out + member->offset) = (i16)value;
			break;
		}
		case VariableType_uint16: {
			u64 value = read_unsigned_integer(ptr);
			*(u16*)((u8*)out + member->offset) = (u16)value;
			break;
		}
		case VariableType_resolution: {
			while (**ptr && **ptr != '{') (*ptr)++;
			(*ptr)++;

			void *child = ((u8*)out + member->offset);

			member_from_string(ptr, Resolution_MemberMetaData,
			                   sizeof(Resolution_MemberMetaData) /
			                   sizeof(StructMemberMetaData),
			                   child);

			while (**ptr && **ptr != '}') (*ptr)++;
			(*ptr)++;

			break;
		}
		case VariableType_video_settings: {
			while (**ptr && **ptr != '{') (*ptr)++;
			(*ptr)++;

			void *child = ((u8*)out + member->offset);

			member_from_string(ptr, VideoSettings_MemberMetaData,
			                   sizeof(VideoSettings_MemberMetaData) /
			                   sizeof(StructMemberMetaData),
			                   child);

			while (**ptr && **ptr != '}') (*ptr)++;
			(*ptr)++;

			break;
		}
		default:
			DEBUG_LOGF(LogType::unimplemented, "unhandled case: %d", member->variable_type);
			break;
		}

		while (**ptr && (**ptr == ' ' || **ptr == ',')) (*ptr)++;
	}
}

void
serialize_load_conf(const char *path,
                    StructMemberMetaData *members,
                    size_t num_members,
                    void *out)
{
	VAR_UNUSED(path);
	VAR_UNUSED(num_members);
	VAR_UNUSED(members);

	char *ptr = (char*)"video = { resolution = { width = 720, height = 640 }, vsync = 0}";
	member_from_string(&ptr, members, num_members, out);
}

