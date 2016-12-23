#ifndef META_DATA_H
#define META_DATA_H

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

struct StructMemberMetaData {
	VariableType variable_type;
	const char   *variable_name;
	size_t       offset;
};

StructMemberMetaData Resolution_MemberMetaData[] = {
	{ VariableType_int32, "width", offsetof(Resolution, width) },
	{ VariableType_int32, "height", offsetof(Resolution, height) },
};

StructMemberMetaData VideoSettings_MemberMetaData[] = {
	{ VariableType_resolution, "resolution", offsetof(VideoSettings, resolution) },
	{ VariableType_int16, "fullscreen", offsetof(VideoSettings, fullscreen) },
	{ VariableType_int16, "vsync", offsetof(VideoSettings, vsync) },
};

StructMemberMetaData Settings_MemberMetaData[] = {
	{ VariableType_video_settings, "video", offsetof(Settings, video) },
};

#endif // META_DATA_H
