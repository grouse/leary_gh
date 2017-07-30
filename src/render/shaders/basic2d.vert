#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texture_coordinate;

layout(location = 0) out vec2 frag_texture_coordinate;

layout(push_constant) uniform Object {
	mat4 srt;
} object;

void main()
{
	gl_Position = object.srt * vec4(position, 0.0, 1.0);
	frag_texture_coordinate = texture_coordinate;
}
