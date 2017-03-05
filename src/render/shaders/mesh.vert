#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(binding = 0) uniform UBO {
	mat4 view_projection;
} ubo;

layout(push_constant) uniform Model {
	mat4 transform;
} model;


layout(location = 0) in vec3 v;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec4 frag_color;

void main()
{
	float distance = length(v - ubo.view_projection[3].xyz);

	vec3 light = vec3(1.0, 1.0, 1.0);
	vec3 diffuse = light / (distance * distance);


	gl_Position = ubo.view_projection * model.transform * vec4(v, 1.0);
	frag_color = vec4(diffuse * vec3(1.0, 1.0, 1.0), 1.0);
}
