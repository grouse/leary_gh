#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#ifdef VERTEX_SHADER

// uniform bindings
layout(binding = 0) uniform UBO {
	mat4 view_projection;
} ubo;

// input bindings
layout(location = 0) in vec3 position;

// output bindings
layout(location = 0) out vec4 frag_color;

void main()
{
	gl_Position = ubo.view_projection * vec4(position.xyz, 1.0);
	frag_color = vec4(0.2, 0.2, 0.2, 1.0);
}

#endif // VERTEX_SHADER


#ifdef FRAGMENT_SHADER

// uniform bindings
layout(binding = 1) uniform sampler2D texture_sampler;

// input bindings
layout(location = 0) in vec4 color;

// output bindings
layout(location = 0) out vec4 out_color;

void main()
{
	out_color = color;
}

#endif // FRAGMENT_SHADER
