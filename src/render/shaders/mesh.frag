#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(binding = 1) uniform sampler2D texture_sampler;

layout(location = 0) in vec3 normal;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 view;
layout(location = 3) in vec3 light;

layout(location = 0) out vec4 out_color;

void main()
{
	vec3 n = normalize(normal);
	vec3 l = normalize(light);
	vec3 v = normalize(view);
	vec3 r = reflect(-l, n);

	vec3 diffuse = max(dot(n, l), 0.0) * color;
	vec3 specular = pow(max(dot(r, v), 0.0), 16.0) * vec3(0.75);

	out_color = vec4(diffuse * color + specular, 1.0);
}
