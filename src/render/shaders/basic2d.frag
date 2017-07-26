#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(binding = 0) uniform sampler2D texture_sampler;

layout(location = 0) in vec2 texture_coordinate;

layout(location = 0) out vec4 out_color;

void main()
{
	float r = texture(texture_sampler, texture_coordinate).r;
	out_color = vec4(r, r, r, 1.0f);
}
