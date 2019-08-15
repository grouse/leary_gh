#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#ifdef VERTEX_SHADER

// uniform bindings
layout(binding = 0) uniform Camera {
    mat4 view_projection;
} camera;


// input bindings
layout(location = 0) in vec3 v;
layout(location = 1) in vec4 color;

// output bindings
layout(location = 0) out vec4 frag_color;

void main()
{
    gl_Position = camera.view_projection * vec4(v, 1.0);
    frag_color  = color;
}

#endif // VERTEX_SHADER

#ifdef FRAGMENT_SHADER

// input bindings
layout(location = 0) in vec4 color;


// output bindings
layout(location = 0) out vec4 out_color;

void main()
{
    out_color = color;
}

#endif // FRAGMENT_SHADER
