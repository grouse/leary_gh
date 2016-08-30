#ifndef LEARY_PLATFORM_H
#define LEARY_PLATFORM_H

#include <string>

enum class EnvironmentFolder {
	GameData,        // Game data,        r,   e.g. shaders, textures, models
	UserPreferences, // Game preferences, r/w
	UserData,        // User data,        r/w, e.g. save games.
};

std::string resolve_path(EnvironmentFolder type, const char* filename);

bool directory_exists(const char* path);
void create_directory(const char* path);


#endif // LEARY_PLATFORM_H
