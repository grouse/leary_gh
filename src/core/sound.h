/**
 * file:    sound.h
 * created: 2018-08-13
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2018 - all rights reserved
 */

struct Sound {
    i32 cursor = 0;
    SoundData data = {};
};

void game_output_sound(i32 *sound_buffer, i32 samples_to_write);
