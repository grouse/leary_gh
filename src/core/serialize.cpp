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
int32_t

member_to_string(StructMemberMetaData &member,
                 void *ptr,
                 char *buffer,
                 size_t size)
{
	int32_t bytes = 0;

	switch (member.variable_type) {
	case VariableType_int32: {
		int32_t value = *(int32_t*)(((char*)ptr) + member.offset);
		bytes = std::sprintf(buffer, "%s = %d",
		                     member.variable_name, value);

		DEBUG_ASSERT(bytes < size);
		break;
	}
	case VariableType_uint32: {
		uint32_t value = *(uint32_t*)(((char*)ptr) + member.offset);
		bytes = std::sprintf(buffer, "%s = %u",
		                     member.variable_name, value);

		DEBUG_ASSERT(bytes < size);
		break;
	}
	case VariableType_int16: {
		int16_t value = *(int16_t*)(((char*)ptr) + member.offset);
		bytes = std::sprintf(buffer, "%s = %" PRId16 , member.variable_name, value);

		DEBUG_ASSERT(bytes < size);
		break;
	}
	case VariableType_uint16: {
		uint16_t value = *(uint16_t*)(((char*)ptr) + member.offset);
		bytes = std::sprintf(buffer, "%s = %" PRIu16,
		                     member.variable_name, value);

		DEBUG_ASSERT(bytes < size);
		break;
	}
	case VariableType_resolution: {
		int32_t child_bytes = std::sprintf(buffer, "%s = { ",
		                                   member.variable_name);

		bytes  += child_bytes;
		buffer += child_bytes;

		size_t num_members = sizeof(Resolution_MemberMetaData) /
		                     sizeof(StructMemberMetaData);

		for (int32_t i = 0; i < (int32_t)num_members; i++) {
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
		int32_t child_bytes = std::sprintf(buffer, "%s = { ",
		                                   member.variable_name);

		bytes  += child_bytes;
		buffer += child_bytes;

		size_t num_members = sizeof(VideoSettings_MemberMetaData) /
		                     sizeof(StructMemberMetaData);

		for (int32_t i = 0; i < (int32_t)num_members; i++) {
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

	for (int32_t i = 0; i < (int32_t)num_members; i++) {
		StructMemberMetaData &member = members[i];
		int32_t bytes = member_to_string(member, ptr, buffer, sizeof(buffer));

		file_write(file_handle, buffer, (size_t)bytes);
	}

	file_close(file_handle);
}

StructMemberMetaData *
find_member(char *name,
            int32_t length,
            StructMemberMetaData *members,
            size_t num_members)
{
	for (int32_t i = 0; i < (int32_t)num_members; i++) {
		if (strncmp(name, members[i].variable_name, length) == 0) {
			return &members[i];
		}
	}

	return nullptr;
}

int64_t
read_integer(char **ptr)
{
	int64_t result = 0;

	while (**ptr && **ptr >= '0' && **ptr <= '9') {
		result *= 10;
		result += (**ptr) - '0';
		(*ptr)++;
	}

	return result;
}

uint64_t
read_unsigned_integer(char **ptr)
{
	uint64_t result = 0;

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
		int32_t name_length = (int32_t)((*ptr) - name);

		StructMemberMetaData *member = find_member(name, name_length,
		                                           members, num_members);

		DEBUG_ASSERT(member != nullptr);


		while (**ptr && **ptr != '=') (*ptr)++;
		(*ptr)++;
		while (**ptr && **ptr == ' ') (*ptr)++;


		switch (member->variable_type) {
		case VariableType_int32: {
			int64_t value = read_integer(ptr);
			*(int32_t*)((uint8_t*)out + member->offset) = (int32_t)value;
			break;
		}
		case VariableType_uint32: {
			uint64_t value = read_unsigned_integer(ptr);
			*(uint32_t*)((uint8_t*)out + member->offset) = (uint32_t)value;
			break;
		}
		case VariableType_int16: {
			int64_t value = read_integer(ptr);
			*(int16_t*)((uint8_t*)out + member->offset) = (int16_t)value;
			break;
		}
		case VariableType_uint16: {
			uint64_t value = read_unsigned_integer(ptr);
			*(int16_t*)((uint8_t*)out + member->offset) = (uint16_t)value;
			break;
		}
		case VariableType_resolution: {
			while (**ptr && **ptr != '{') (*ptr)++;
			(*ptr)++;

			void *child = ((uint8_t*)out + member->offset);

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

			void *child = ((uint8_t*)out + member->offset);

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

