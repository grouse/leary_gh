#include "physics.h"

#include "gamestate.h"

#include <iostream>

void Physics::init(GameState* gs) {
	this->game = gs;
}

void Physics::simulate(float dt) {
	if (inputMove.z != 0.0f) 
		game->rendering.camera.position += game->rendering.camera.forward * dt * speed * inputMove.z;
	
	if (inputMove.x != 0.0f) 
		game->rendering.camera.position += game->rendering.camera.right * dt * speed * inputMove.x;
	
	if (inputMove.y != 0.0f) 
		game->rendering.camera.position += game->rendering.camera.up * dt * speed * inputMove.y;

	if (inputMouse.x != 0 || inputMouse.y != 0) {
		game->rendering.camera.horizontalAngle += inputMouse.x * dt;
		game->rendering.camera.verticalAngle   += inputMouse.y * dt;

		inputMouse = glm::vec2(0);
	}
}
