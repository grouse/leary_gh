#ifndef TYPE_INFO_H
#define TYPE_INFO_H

enum VariableType {
	VariableType_int32,
	VariableType_uint32,
	VariableType_int16,
	VariableType_uint16,
	VariableType_f32,
	VariableType_resolution,
	VariableType_video_settings,
	VariableType_settings,
	VariableType_unknown
};

struct StructMemberInfo {
	VariableType type;
	const char   *name;
	usize        offset;
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

StructMemberInfo Vector3_members[] = {
	{ VariableType_f32, "x", offsetof(Vector3, x) },
	{ VariableType_f32, "y", offsetof(Vector3, y) },
	{ VariableType_f32, "z", offsetof(Vector3, z) },
};

StructMemberInfo Vector4_members[] = {
	{ VariableType_f32, "x", offsetof(Vector4, x) },
	{ VariableType_f32, "y", offsetof(Vector4, y) },
	{ VariableType_f32, "z", offsetof(Vector4, z) },
	{ VariableType_f32, "w", offsetof(Vector4, w) },
};

#endif // TYPE_INFO
