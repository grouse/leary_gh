#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(binding = 1) uniform sampler2D texture_sampler;

layout(location = 0) in vec2 texture_coordinate;

layout(location = 0) out vec4 out_color;

void main()
{
	float a = texture(texture_sampler, texture_coordinate).a;
	if (a > 0.0) {
		out_color = vec4(1.0) * a;
	} else {
		discard;
	}
}
