#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 1, binding = 0) uniform sampler2D texture_sampler;

layout(location = 0) in vec3 normal;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 view;
layout(location = 3) in vec3 light;
layout(location = 4) in vec2 uv;

layout(location = 0) out vec4 out_color;

void main()
{
	vec3 n = normalize(normal);
	vec3 l = normalize(light);
	vec3 v = normalize(view);
	vec3 r = reflect(-l, n);

	vec4 ambient  = vec4(0.3, 0.3, 0.3, 1.0);
	vec4 diffuse  = vec4(max(dot(n, l), 0.0) * color, 1.0);

	out_color = texture(texture_sampler, uv) * (ambient + diffuse);
}
