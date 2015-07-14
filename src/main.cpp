#include <iostream>

#include <SDL2/SDL.h>

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"

#include "debug.h"
#include "util.h"
#include "settings.h"

#include "gamestate.h"

#include "rendering.h"
#include "physics.h"
#include "input.h"

int main(int argc, char* argv[]) {


	if (SDL_Init(SDL_INIT_VIDEO) != 0)
		DebugPrintf(EDebugType::FATAL, EDebugCode::SDL_INIT, "Failed to initialise SDL_VIDEO: %s", SDL_GetError());

	GameState game;
	game.settings = load_settings_from_ini("data/settings.ini");

	std::cout << game.settings.video.resolution_y << std::endl;

	game.input.init(&game);
	game.physics.init(&game);
	game.rendering.init(&game);
	
	game.run = true;

	unsigned int currentTime = SDL_GetTicks();
	unsigned int frameTime   = 0;    // Actual time since last frame update in millisecondsd
	
	float timeStep    = 0.001; // Fixed timestep with which to update physics and other fixed timestep systems
	float accumulator = 0.0;  // How many milliseconds that has accumulated since last physics update

	while (game.run) { 
		unsigned int newTime = SDL_GetTicks();
	
		frameTime = newTime - currentTime;
		currentTime = newTime;
		
		accumulator += frameTime;

		game.input.update();

		while (accumulator >= timeStep) {
			game.physics.simulate(timeStep);
			accumulator -= timeStep;
		}

		game.rendering.render();
	}

	SDL_Quit();
}
