#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#ifdef VERTEX_SHADER

// input bindings
layout(location = 0) in vec2 position;
layout(location = 1) in vec4 color;

// output bindings
layout(location = 0) out vec4 frag_color;

// push constants
layout(push_constant) uniform Constants {
	mat4 srt;
} constants;

void main()
{
	gl_Position = constants.srt * vec4(position, 0.0, 1.0);
	frag_color = color;
}

#endif // VERTEX_SHADER



#ifdef FRAGMENT_SHADER

// input bindings
layout(location = 0) in vec4 color;


// output bindings
layout(location = 0) out vec4 out_color;

void main()
{
	out_color = out_color;
}

#endif // FRAGMENT_SHADER
