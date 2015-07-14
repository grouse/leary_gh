#include "input.h"

#include "gamestate.h"

void Input::init(GameState* gs) { 
	this->game = gs;

	SDL_SetRelativeMouseMode(SDL_TRUE);
}

void Input::update() {
	// @ToDo: cleanup input and load actions/keybinds from unordered hashmap/similar
	
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT) 
			game->run = false;

		if (e.type == SDL_KEYUP) {
			switch (e.key.keysym.sym) {
				case SDLK_ESCAPE:
					game->run = false;
					break;
				
				case SDLK_w:
				case SDLK_s:
					game->physics.inputMove.z = 0;
					break;
				
				case SDLK_a:
				case SDLK_d:
					game->physics.inputMove.x = 0;
					break;

				case SDLK_SPACE:
				case SDLK_LCTRL:
					game->physics.inputMove.y = 0;
					break;
			}
		}

		if (e.type == SDL_KEYDOWN) {
			switch (e.key.keysym.sym) {
				case SDLK_w:
					game->physics.inputMove.z = 1;
					break;

				case SDLK_s:
					game->physics.inputMove.z = -1;
					break;
				
				case SDLK_a:
					game->physics.inputMove.x = -1;
					break;

				case SDLK_d:
					game->physics.inputMove.x = 1;
					break;
				
				case SDLK_SPACE:
					game->physics.inputMove.y = 1;
					break;
				
				case SDLK_LCTRL:
					game->physics.inputMove.y = -1;
					break;
			}

		}

		if (e.type == SDL_MOUSEMOTION) {
			game->physics.inputMouse.x += mouseSensitivity * e.motion.xrel;
			game->physics.inputMouse.y += mouseSensitivity * -e.motion.yrel;
		}
	}

}
