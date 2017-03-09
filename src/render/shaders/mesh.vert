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

layout(location = 0) out vec3 frag_normal;
layout(location = 1) out vec3 frag_color;
layout(location = 2) out vec3 frag_view;
layout(location = 3) out vec3 frag_light;

void main()
{
	gl_Position = ubo.view_projection * model.transform * vec4(v, 1.0);

	vec4 pos   = model.transform * vec4(v, 1.0);
	vec3 light = mat3(model.transform) * vec3(0.0, -30.0, 0.0);

	frag_normal = mat3(model.transform) * normal;
	frag_color  = vec3(1.0, 1.0, 1.0);
	frag_view   = -pos.xyz;
	frag_light  = light - pos.xyz;
}
