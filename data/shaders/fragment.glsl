#version 330

in vec2 UV;
// in vec3 fragmentColour;

out vec3 colour;

uniform sampler2D textureSampler;

void main() {
	// colour = fragmentColour;
	colour = texture(textureSampler, UV).rgb;
}
