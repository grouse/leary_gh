#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#ifdef VERTEX_SHADER

// uniform bindings
layout(binding = 0) uniform UBO {
	mat4 view_projection;
} ubo;

layout(push_constant) uniform Model {
	mat4 transform;
} model;


// input bindings
layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 texture_coordinate;

// output bindings
layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec2 frag_texture_coordinate;

void main()
{
	gl_Position = ubo.view_projection * model.transform * vec4(position.xy, 0.0, 1.0);
	frag_color = color;
	frag_texture_coordinate = texture_coordinate;
}

#endif // VERTEX_SHADER


#ifdef FRAGMENT_SHADER

// uniform bindings
layout(binding = 1) uniform sampler2D texture_sampler;

// input bindings
layout(location = 0) in vec4 color;
layout(location = 1) in vec2 texture_coordinate;

// output bindings
layout(location = 0) out vec4 out_color;

void main()
{
	out_color = color + texture(texture_sampler, texture_coordinate);
}

#endif // FRAGMENT_SHADER
