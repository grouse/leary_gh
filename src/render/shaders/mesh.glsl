#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


#ifdef VERTEX_SHADER

// uniform bindings
layout(binding = 0) uniform Camera {
	mat4 view_projection;
} camera;

layout(push_constant) uniform Model {
	mat4 transform;
} model;


// input bindings
layout(location = 0) in vec3 v;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

// output bindings
layout(location = 0) out vec3 frag_normal;
layout(location = 1) out vec3 frag_color;
layout(location = 2) out vec3 frag_view;
layout(location = 3) out vec3 frag_light;
layout(location = 4) out vec2 frag_uv;

void main()
{
	vec4 pos    = model.transform * vec4(v, 1.0);
	gl_Position = camera.view_projection * pos;

	vec3 light = mat3(model.transform) * vec3(0.0, -30.0, 0.0);

	frag_normal = mat3(model.transform) * normal;
	frag_color  = vec3(1.0, 1.0, 1.0);
	frag_view   = -pos.xyz;
	frag_light  = light - pos.xyz;
	frag_uv     = uv;
}

#endif // VERTEX_SHADER


#ifdef FRAGMENT_SHADER

// uniform bindings
layout(set = 1, binding = 0) uniform sampler2D texture_sampler;

// input bindings
layout(location = 0) in vec3 normal;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 view;
layout(location = 3) in vec3 light;
layout(location = 4) in vec2 uv;

// output bindings
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

#endif // FRAGMENT_SHADER
