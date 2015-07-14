#ifndef INPUT_H
#define INPUT_H

#include <SDL2/SDL.h>

struct GameState;

class Input {
	GameState* game;

	float mouseSensitivity = 1.0f;

public:
	void init(GameState*);
	void update();
};


#endif
