/**
 * file:    serialize.cpp
 * created: 2016-11-20
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2016 - all rights reserved
 */

#include "generated/meta_data.h"
#include "platform/debug.h"
#include "platform/file.h"

#define SERIALIZE_SAVE_CONF(file, name, ptr) \
	serialize_save_conf(file, name ## _MemberMetaData, \
	                    sizeof(name ## _MemberMetaData) / sizeof(StructMemberMetaData), \
	                    ptr)

int32_t
member_to_string(StructMemberMetaData &member, void *ptr, char *buffer, size_t size)
{
	int32_t bytes = 0;

	switch (member.variable_type) {
	case VariableType_int32:
	{
		int32_t value = *(int32_t*)(((char*)ptr) + member.offset);
		bytes = std::sprintf(buffer, "%s = %d",
		                     member.variable_name, value);

		DEBUG_ASSERT(bytes < size);
		break;
	}
	case VariableType_uint32:
	{
		uint32_t value = *(uint32_t*)(((char*)ptr) + member.offset);
		bytes = std::sprintf(buffer, "%s = %u",
		                     member.variable_name, value);

		DEBUG_ASSERT(bytes < size);
		break;
	}
	case VariableType_int16:
	{
		int16_t value = *(int16_t*)(((char*)ptr) + member.offset);
		bytes = std::sprintf(buffer, "%s = %" PRId16,
		                     member.variable_name, value);

		DEBUG_ASSERT(bytes < size);
		break;
	}
	case VariableType_uint16:
	{
		uint16_t value = *(uint16_t*)(((char*)ptr) + member.offset);
		bytes = std::sprintf(buffer, "%s = %" PRIu16,
		                     member.variable_name, value);

		DEBUG_ASSERT(bytes < size);
		break;
	}
	case VariableType_resolution:
	{
		int32_t child_bytes = std::sprintf(buffer, "%s = { ", member.variable_name);

		bytes  += child_bytes;
		buffer += child_bytes;

		size_t num_members = sizeof(Resolution_MemberMetaData) / sizeof(StructMemberMetaData);
		for (int32_t i = 0; i < (int32_t)num_members; i++)
		{
			StructMemberMetaData &child = Resolution_MemberMetaData[i];
			child_bytes = member_to_string(child, ptr, buffer, size);

			bytes  += child_bytes;
			buffer += child_bytes;

			if (i != (num_members - 1))
			{
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
	case VariableType_video_settings:
	{
		int32_t child_bytes = std::sprintf(buffer, "%s = { ", member.variable_name);

		bytes  += child_bytes;
		buffer += child_bytes;

		size_t num_members = sizeof(VideoSettings_MemberMetaData) / sizeof(StructMemberMetaData);
		for (int32_t i = 0; i < (int32_t)num_members; i++)
		{
			StructMemberMetaData &child = VideoSettings_MemberMetaData[i];
			child_bytes = member_to_string( child, ptr, buffer, size);

			bytes  += child_bytes;
			buffer += child_bytes;

			if (i != (num_members - 1))
			{
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
	default:
	{
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
	if (!file_exists(path) && !file_create(path))
	{
		DEBUG_ASSERT(false);
		return;
	}

	void *file_handle = file_open(path, FileMode::write);

	// TODO(jesper): rewrite this so that we do fewer file_write, potentially so that we only open
	// file and write into it after we've got an entire buffer to write into it
	char buffer[2048];

	for (int32_t i = 0; i < (int32_t)num_members; i++)
	{
		StructMemberMetaData &member = members[i];
		int32_t bytes = member_to_string(member, ptr, buffer, sizeof(buffer));

		file_write(file_handle, buffer, (size_t)bytes);
	}

	file_close(file_handle);
}

