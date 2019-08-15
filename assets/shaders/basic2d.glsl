#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#ifdef VERTEX_SHADER

// input bindings
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texture_coordinate;

// output bindings
layout(location = 0) out vec2 frag_texture_coordinate;

// push constants
layout(push_constant) uniform Object {
	mat4 srt;
} object;

void main()
{
	gl_Position = object.srt * vec4(position, 0.0, 1.0);
	frag_texture_coordinate = texture_coordinate;
}

#endif // VERTEX_SHADER

#ifdef FRAGMENT_SHADER

// uniform bindings
layout(binding = 0) uniform sampler2D texture_sampler;

// input bindings
layout(location = 0) in vec2 texture_coordinate;

// output bindings
layout(location = 0) out vec4 out_color;

void main()
{
	out_color = texture(texture_sampler, texture_coordinate);
}

#endif // FRAGMENT_SHADER
