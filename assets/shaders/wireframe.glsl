#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#ifdef VERTEX_SHADER

// uniform bindings
layout(binding = 0) uniform Camera {
    mat4 view_projection;
} camera;

layout(push_constant) uniform PushConstants {
    mat4 transform;
    vec3 color;
} pc;

// input bindings
layout(location = 0) in vec3 v;

// output bindings
layout(location = 0) out vec3 frag_color;

void main()
{
    vec4 pos    = pc.transform * vec4(v, 1.0);
    gl_Position = camera.view_projection * pos;
    frag_color  = pc.color;
}

#endif // VERTEX_SHADER

#ifdef FRAGMENT_SHADER

// input bindings
layout(location = 0) in vec3 color;


// output bindings
layout(location = 0) out vec4 out_color;

void main()
{
    out_color = vec4(color, 1.0f);
}

#endif // FRAGMENT_SHADER
