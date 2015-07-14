#ifndef GAMESTATE_H
#define GAMESTATE_H

#include "settings.h"

#include "input.h"
#include "physics.h"
#include "rendering.h"

struct GameState {
	Settings settings;

	Input input;
	Physics physics;
	Rendering rendering;

	bool run = true;
};

#endif
