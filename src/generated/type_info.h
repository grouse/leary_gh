#ifndef TYPE_INFO_H
#define TYPE_INFO_H

enum VariableType {
	VariableType_int32,
	VariableType_uint32,
	VariableType_int16,
	VariableType_uint16,
	VariableType_resolution,
	VariableType_video_settings,
	VariableType_settings,
	VariableType_unknown
};

struct StructMemberInfo {
	VariableType type;
	const char   *name;
	size_t       offset;
};

StructMemberInfo Resolution_members[] = {
	{ VariableType_int32, "width", offsetof(Resolution, width) },
	{ VariableType_int32, "height", offsetof(Resolution, height) },
};

StructMemberInfo VideoSettings_members[] = {
	{ VariableType_resolution, "resolution", offsetof(VideoSettings, resolution) },
	{ VariableType_int16, "fullscreen", offsetof(VideoSettings, fullscreen) },
	{ VariableType_int16, "vsync", offsetof(VideoSettings, vsync) },
};

StructMemberInfo Settings_members[] = {
	{ VariableType_video_settings, "video", offsetof(Settings, video) },
};

#endif // TYPE_INFO
