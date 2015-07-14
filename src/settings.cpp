#include <unordered_map>
#include <iostream>
#include <fstream>
#include <sstream>

#include "settings.h"

#include "debug.h"

enum ESettingsValue {
	GENERAL_TITLE      = 0,
	VIDEO_RESOLUTION_X = 1,
	VIDEO_RESOLUTION_Y = 2
};

std::unordered_map<std::string, ESettingsValue> setup_settings_map(Settings* settings) {
	std::unordered_map<std::string, ESettingsValue> settings_map;
	
	settings_map["general:title"] = ESettingsValue::GENERAL_TITLE;

	settings_map["video:resolution_y"] = ESettingsValue::VIDEO_RESOLUTION_Y;
	settings_map["video:resolution_x"] = ESettingsValue::VIDEO_RESOLUTION_X;

	return settings_map;
}

Settings load_settings_from_ini(std::string file) {
	Settings settings;

	std::unordered_map<std::string, ESettingsValue> settings_map = setup_settings_map(&settings);


	std::ifstream file_stream(file);
	if (!file_stream.is_open())
		return settings;

	std::string section = "";
	
	std::string key;
	std::string value;
	
	while (file_stream >> key) {
		// skip comments
		if (key[0] == '#') {
			file_stream.ignore(256, '\n');
			continue;
		}

		// A new section should be set
		if (key[0] == '[') {
			section = key.substr(1, key.length()-2);
			file_stream.ignore(256, '\n');
			continue;
		}

		// we can assume we're gonna be setting a value at this point
		auto setting_itr = settings_map.find(section + ":" + key);
		if (setting_itr == settings_map.end()) {
			DebugPrintf(EDebugType::WARNING, EDebugCode::SETTINGS_UNSUPPORTED_KEY, "Unsupported setting key: %s, in file: %s\n", key.c_str(), file.c_str());
			file_stream.ignore(256, '\n'); // skip to end of line
			continue;
		}
	
		// we can assume we're accessing a setting at this point		
		ESettingsValue setting = setting_itr->second;
		file_stream.ignore(10, '=');

		std::string str_value;
		
		switch (setting) {
			case GENERAL_TITLE:
				std::getline(file_stream, str_value);
				settings.general.title = str_value;
				break;
			
			case VIDEO_RESOLUTION_X:
				file_stream >> settings.video.resolution_x;
				break;
			case VIDEO_RESOLUTION_Y:
				file_stream >> settings.video.resolution_y;
				break;
		}
	}

	file_stream.close();
	return settings;
}
