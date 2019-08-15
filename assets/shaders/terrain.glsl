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
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec3 bitangent;
layout(location = 4) in vec2 uv;

// output bindings
layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec3 frag_normal;

layout(location = 2) out vec3 frag_pos;
layout(location = 3) out vec3 frag_eye_pos;
layout(location = 4) out vec3 frag_sun_pos;

layout(location = 5) out vec2 frag_uv;
layout(location = 6) out mat3 frag_tbn;

void main()
{
    vec4 pos    = vec4(position.xyz, 1.0);
    gl_Position = ubo.view_projection * pos;

    frag_color  = vec4(1.0, 1.0, 1.0, 1.0);

    mat3 nrm_mat = transpose(mat3(1.0));
    vec3 t = normalize(nrm_mat * tangent);
    vec3 n = normalize(nrm_mat * normal);
    vec3 b = normalize(nrm_mat * bitangent);

    mat3 tbn = transpose(mat3(t, b, n));
    frag_tbn = tbn;

    frag_normal = normal;
    frag_uv     = uv;

    frag_pos = tbn * pos.xyz;
    frag_sun_pos = tbn * vec3(40.0, -100.0, 20.0);
    frag_eye_pos = tbn * ubo.view_projection[3].xyz;
}

#endif // VERTEX_SHADER


#ifdef FRAGMENT_SHADER

// uniform bindings
layout(set = 1, binding = 0) uniform sampler2D tex0_col;
layout(set = 1, binding = 1) uniform sampler2D tex0_nrm;
layout(set = 1, binding = 2) uniform sampler2D tex1_col;
layout(set = 1, binding = 3) uniform sampler2D tex1_nrm;
layout(set = 1, binding = 4) uniform sampler2D texture_map;

// input bindings
layout(location = 0) in vec4 in_color;
layout(location = 1) in vec3 in_normal;

layout(location = 2) in vec3 in_pos;
layout(location = 3) in vec3 in_eye_pos;
layout(location = 4) in vec3 in_sun_pos;

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

vec4 sRGB_from_linear(vec4 l)
{
    vec4 s;
    s.r = sRGB_from_linear(l.r);
    s.g = sRGB_from_linear(l.g);
    s.b = sRGB_from_linear(l.b);
    s.a = sRGB_from_linear(l.a);
    return s;
}

void main()
{
    vec3 light_dir = normalize(in_sun_pos - in_pos);
    vec3 eye_dir   = normalize(in_eye_pos - in_pos);
    vec3 half_lv   = normalize(light_dir + eye_dir);

    // TODO(jesper): this is dependent on the terrain size
    vec4 weights = sRGB_from_linear(texture(texture_map, in_uv * (12.0 / 100.0)));

    float ambient_intensity = 0.1;
    vec3 light_color = vec3(1.0, 1.0, 1.0);

    vec3 ambient = vec3(0.0, 0.0, 0.0);
    vec3 diffuse = vec3(0.0, 0.0, 0.0);

    {
        vec3 color = texture(tex0_col, in_uv).rgb;
        ambient = mix(ambient, color, weights.r);

        vec3 normal = sRGB_from_linear(texture(tex0_nrm, in_uv).xyz);
        normal = normalize(normal * 2.0f - 1.0f);
        normal = normalize(normal);

        vec3 reflect_dir = reflect(-light_dir, normal);
        float coef = max(dot(light_dir, normal), 0.0);
        vec3 d = light_color * coef * color;
        diffuse = mix(diffuse, d, weights.r);
    }

    {
        vec3 color = texture(tex1_col, in_uv).rgb;
        ambient = mix(ambient, color, weights.g);

        vec3 normal = sRGB_from_linear(texture(tex1_nrm, in_uv).xyz);
        normal = normalize(normal * 2.0f - 1.0f);
        normal = normalize(normal);

        vec3 reflect_dir = reflect(-light_dir, normal);
        float coef = max(dot(light_dir, normal), 0.0);
        vec3 d = light_color * coef * color;
        diffuse = mix(diffuse, d, weights.g);
    }

    ambient = ambient * ambient_intensity * light_color;
    if (true) {
        out_color = vec4(ambient + diffuse, 1.0);
    } else {
        out_color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
}

#endif // FRAGMENT_SHADER
