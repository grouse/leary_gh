/**
 * @file:   settings.cpp
 * @author: Jesper Stefansson (grouse)
 * @email:  jesper.stefansson@gmail.com
 *
 * Copyright (c) 2015-2016 Jesper Stefansson
 */


INTROSPECT struct Resolution
{
	i32 width  = 1280;
	i32 height = 720;
};

INTROSPECT struct VideoSettings
{
	Resolution resolution;

	// NOTE: these are integers to later support different fullscreen and vsync techniques
	i16 fullscreen = 0;
	i16 vsync      = 1;
};

INTROSPECT struct Settings
{
	VideoSettings video;
};
