/**
 * @file:   settings.cpp
 * @author: Jesper Stefansson (grouse)
 * @email:  jesper.stefansson@gmail.com
 *
 * Copyright (c) 2015-2016 Jesper Stefansson
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgement in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include "prefix.h"
#include "settings.h"

#include "util/environment.h"

#include <cstdio>
#include <fstream>
#include <unordered_map>

Settings *Settings::m_instance = nullptr;

typedef std::unordered_map<std::string, std::string> ini_map_t;

inline void ini_save_value(FILE* const       file, 
                           const char* const key, 
                           const int32_t&    value)
{
	fprintf(file, "%s = %d\n", key, value);
}

inline void ini_save_value(FILE* const       file, 
                           const char* const key, 
                           const bool&       value)
{
	ini_save_value(file, key, static_cast<const int32_t&>(value));
}

inline void ini_load_value(const ini_map_t&  values, 
                           const char* const key, 
                           int32_t* const    value)
{
	const auto iter = values.find(key);
	if (iter == values.end())
		return;

	sscanf(iter->second.c_str(), "%d", value);
}

inline void ini_load_value(const ini_map_t&  values, 
                           const char* const key, 
                           bool* const       value)
{
	const auto iter = values.find(key);
	if (iter == values.end())
		return;

	uint8_t tmp;
	sscanf(iter->second.c_str(), "%c", &tmp);
	*value = tmp ? true : false;
}
void Settings::create()
{
	if (m_instance == nullptr)
		m_instance = new Settings();
}

void Settings::destroy()
{
	if (m_instance != nullptr)
		delete m_instance;
}

Settings* Settings::get() 
{
	return m_instance;
}

void Settings::load(const char* filename) 
{
	std::string file_path = 
		Environment::resolvePath(eEnvironmentFolder::UserPreferences, filename);

	std::fstream stream(file_path, std::ios::in);
	LEARY_ASSERT(stream.is_open());

	if (!stream.is_open())
		return;

	ini_map_t values;
	while (!stream.eof())
	{
		char delim;
		std::string key, value;

		stream >> key >> delim >> value;

		// skip comments
		if (key[0] == '/' && key[1] == '/')
			continue;

		values[key] = value;
	}

	ini_load_value(values, "video.resolution.width",  &video.resolution.width);
	ini_load_value(values, "video.resolution.height", &video.resolution.height);
	ini_load_value(values, "video.fullscreen",        &video.fullscreen);

	stream.close();
}

void Settings::save(const char* filename) const
{
	std::string file_path = 
		Environment::resolvePath(eEnvironmentFolder::UserPreferences, filename);

	FILE *file = fopen(file_path.c_str(), "w");
	LEARY_ASSERT(file != NULL);

	ini_save_value(file, "video.resolution.width",  video.resolution.width);
	ini_save_value(file, "video.resolution.height", video.resolution.height);
	ini_save_value(file, "video.fullscreen",        video.fullscreen);

	fclose(file);
}
