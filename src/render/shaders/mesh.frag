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

	vec3 ambient  = vec3(0.3, 0.3, 0.3);
	vec3 diffuse  = max(dot(n, l), 0.0) * color;
	vec3 specular = pow(max(dot(r, v), 0.0), 16.0) * vec3(0.75);

	vec4 color = vec4((ambient + diffuse + specular) * color, 1.0);
	vec4 texel = texture(texture_sampler, uv);
	out_color = mix(color, texel, color.a);
}
