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
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv_scale;

// output bindings
layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec3 frag_normal;
layout(location = 2) out vec3 frag_view;
layout(location = 3) out vec3 frag_light;
layout(location = 4) out vec2 frag_uv;

void main()
{
    vec4 pos       = vec4(position.xyz, 1.0);
    vec4 world_pos = ubo.view_projection * pos;
    gl_Position    = world_pos;

    vec3 light = vec3(0.0, -30.0, 0.0);

    frag_color = vec4(0.2, 0.2, 0.2, 1.0);
    frag_normal =  normal;
    frag_view   = -pos.xyz;
    frag_light  = light - pos.xyz;

    frag_uv = position.xz * uv_scale;
}

#endif // VERTEX_SHADER


#ifdef FRAGMENT_SHADER

// uniform bindings
layout(set = 1, binding = 0) uniform sampler2D diffuse_sampler;

// input bindings
layout(location = 0) in vec4 color;
layout(location = 1) in vec3 normal;
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
    vec4 diffuse  = vec4(max(dot(n, l), 0.0) * color.rgb, 1.0);

    out_color = texture(diffuse_sampler, uv) * (ambient + diffuse);
}

#endif // FRAGMENT_SHADER
