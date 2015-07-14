#ifndef RENDERING_H
#define RENDERING_H

#include <iostream>

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "debug.h"
#include "util.h"

struct GameState;

struct Camera {
	float verticalAngle   = 3.14f;
	float horizontalAngle = 0.0f;
	float FoV             = 45.0f;
	float aspect_ratio    = 16.0f / 9.0f;

	glm::vec3 position = glm::vec3(4.0f, 3.0f, 3.0f);
	glm::vec3 forward  = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 up       = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 right    = glm::vec3(0.0f, 0.0f, 0.0f);

	glm::mat4 projection, view;
};

class Rendering {
	GameState* game;

	SDL_Window* window;
	SDL_GLContext glcontext;	

public:
	Rendering();
	~Rendering();

	void init(GameState*);
	void render();
	
	GLuint shaderProgram;

	GLuint MVPLocation;
	GLuint samplerLocation;

	GLuint vao;  // Vertex Array Object
	GLuint vbo;  // Vertex Buffer Object
	GLuint cbo;  // Colour Buffer Object
	GLuint uvbo; // UV Buffer Object
	unsigned int vertices_size;
	GLuint texture;

	glm::mat4 modelMatrix;
	glm::mat4 MVP;	

	glm::mat4 projectionMatrix;	
	glm::mat4 viewMatrix;

	Camera camera;
};


#endif
