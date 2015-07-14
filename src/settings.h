#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>

struct Settings {
	struct {
		std::string title = "Project Leary";
	} general;

	struct {
		unsigned int resolution_x = 1280;
		unsigned int resolution_y = 720;
	} video;
};

Settings load_settings_from_ini(std::string);

#endif
