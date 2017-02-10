#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(binding = 0) uniform Camera {
	mat4 view;
	mat4 projection;
} camera;

layout(push_constant) uniform Model {
	mat4 transform;
} model;


layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texture_coordinate;

layout(location = 0) out vec2 frag_texture_coordinate;

void main()
{
	gl_Position = camera.projection * model.transform * vec4(position, 1.0);
	frag_texture_coordinate = texture_coordinate;
}
