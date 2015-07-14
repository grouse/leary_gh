#ifndef PHYSICS_H
#define PHYSICS_H

#include <glm/glm.hpp>

struct GameState;

class Physics {
	GameState* game;

	float speed = 0.05f;

	glm::vec3 position;
	glm::vec3 direction;
	glm::vec3 up;

	float horizontalAngle;
	float verticalAngle;

public:
	void init(GameState*);

	void simulate(float dt);

	void moveForward(int);
	void moveRight(int);
	void moveMouseRelative(int, int);
	
	glm::vec2 inputMouse = glm::vec2(0, 0);
	glm::vec3 inputMove  = glm::vec3(0.0f, 0.0f, 0.0f);
};


#endif
