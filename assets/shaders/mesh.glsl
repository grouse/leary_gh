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
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec3 bitangent;
layout(location = 4) in vec2 uv;

// output bindings
layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec3 frag_normal;

layout(location = 2) out vec3 frag_pos;
layout(location = 3) out vec3 frag_eye_pos;
layout(location = 4) out vec3 frag_sun_dir;

layout(location = 5) out vec2 frag_uv;
layout(location = 6) out mat3 frag_tbn;

void main()
{
    mat4 mvp = camera.view_projection * model.transform;
    vec4 pos = model.transform * vec4(v, 1.0);
    gl_Position = mvp * vec4(v, 1.0);

    frag_color  = vec4(1.0, 1.0, 1.0, 1.0);

    mat3 nrm_mat = transpose(mat3(model.transform));
    vec3 t = normalize(nrm_mat * tangent);
    vec3 n = normalize(nrm_mat * normal);
    vec3 b = normalize(nrm_mat * bitangent);

    frag_normal = normal;
    frag_uv     = uv;
    frag_tbn    = mat3(t, b, n);

    frag_pos     = pos.xyz;
    frag_sun_dir = normalize(vec3(0.0, -100.0, 3.0));
    frag_eye_pos = camera.view_projection[3].xyz;
}

#endif // VERTEX_SHADER


#ifdef FRAGMENT_SHADER

// uniform bindings
layout(set = 1, binding = 0) uniform sampler2D tex_col;
layout(set = 1, binding = 1) uniform sampler2D tex_nrm;

// input bindings
layout(location = 0) in vec4 in_color;
layout(location = 1) in vec3 in_normal;

layout(location = 2) in vec3 in_pos;
layout(location = 3) in vec3 in_eye_pos;
layout(location = 4) in vec3 in_sun_dir;

layout(location = 5) in vec2 in_uv;
layout(location = 6) in mat3 in_tbn;

// output bindings
layout(location = 0) out vec4 out_color;

float sRGB_from_linear(float l)
{
    if (l > 1.0f) {
        return 1.0f;
    } else if (l < 0.0f) {
        return 0.0f;
    }

    float s = l*12.92f;
    if (l > 0.0031308f) {
        s = 1.055f*pow(l, 1.0f/2.4f) - 0.055f;
    }

    return s;
}

vec3 sRGB_from_linear(vec3 l)
{
    vec3 s;
    s.r = sRGB_from_linear(l.r);
    s.g = sRGB_from_linear(l.g);
    s.b = sRGB_from_linear(l.b);
    return s;
}

void main()
{
    vec3 light_dir = normalize(in_sun_dir);
    vec3 eye_dir   = normalize(in_eye_pos - in_pos);
    vec3 half_lv   = normalize(light_dir + eye_dir);

    vec3 color = texture(tex_col, in_uv).rgb;

    float ambient_intensity = 0.4;
    vec3 light_color = vec3(1.0, 1.0, 1.0);

    vec3 normal = sRGB_from_linear(texture(tex_nrm, in_uv).xyz);
    normal = normalize(normal * 2.0f - 1.0f);
    normal = normalize(in_tbn * normal);

    vec3 ambient = ambient_intensity * light_color * color;

    vec3 reflect_dir = reflect(-light_dir, normal);
    float coef = max(dot(light_dir, normal), 0.0);
    vec3 diffuse = light_color * coef * color;

    out_color = vec4(ambient + diffuse, 1.0);
}

#endif // FRAGMENT_SHADER
