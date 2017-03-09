#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(binding = 0) uniform UBO {
	mat4 view_projection;
} ubo;

layout(location = 0) in vec3 position;

layout(location = 0) out vec4 frag_color;

void main()
{
	gl_Position = ubo.view_projection * vec4(position.xyz, 1.0);
	frag_color = vec4(0.2, 0.2, 0.2, 1.0);
}
