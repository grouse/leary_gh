#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#ifdef VERTEX_SHADER

// input bindings
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texture_coordinate;
layout(location = 2) in vec4 color;

// output bindings
layout(location = 0) out vec2 frag_texture_coordinate;
layout(location = 1) out vec4 frag_color;


// push constants
layout(push_constant) uniform Object {
    mat4 srt;
} object;

void main()
{
    gl_Position = object.srt * vec4(position, 0.0, 1.0);
    frag_texture_coordinate = texture_coordinate;
    frag_color = color;
}

#endif // VERTEX_SHADER


#ifdef FRAGMENT_SHADER

// uniform bindings
layout(binding = 0) uniform sampler2D texture_sampler;

// input bindings
layout(location = 0) in vec2 texture_coordinate;
layout(location = 1) in vec4 color;

// output bindings
layout(location = 0) out vec4 out_color;

void main()
{
    float a = texture(texture_sampler, texture_coordinate).a;
    out_color = color * a;
}

#endif // FRAGMENT_SHADER
