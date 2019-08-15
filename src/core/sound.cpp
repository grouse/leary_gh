/**
 * file:    sound.cpp
 * created: 2018-08-13
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2018 - all rights reserved
 */

Array<Sound> g_active_mono_sounds;
Array<Sound> g_active_stereo_sounds;
Array<Sound> g_active_looping_mono_sounds;

void init_sound()
{
    init_array(&g_active_mono_sounds, g_heap);
    init_array(&g_active_stereo_sounds, g_heap);
    init_array(&g_active_looping_mono_sounds, g_heap);
}

void game_output_sound(i32 *sound_buffer, i32 samples_to_write)
{
    PROFILE_FUNCTION();
    // TODO(jesper): support run-time resampling or do it in asset stage?
    // TODO(jesper): bother with handling i32 overflow? could happen in theory
    // TODO(jesper): support 24 bit audio?
    // TODO(jesper): volume control
    
    f32 master_volume = 0.0f;

    i32 *output = sound_buffer;
    for (i32 j = 0; j < g_active_mono_sounds.count; j++) {
        Sound &snd = g_active_mono_sounds[j];

        output = sound_buffer;
        for (i32 i = 0; i < samples_to_write; i++) {
            *output++ += master_volume * snd.data.samples[snd.cursor];
            *output++ += master_volume * snd.data.samples[snd.cursor];
            snd.cursor++;

            if (snd.cursor >= snd.data.num_samples) {
                array_remove(&g_active_mono_sounds, j--);
                break;
            }
        }
    }

    for (i32 j = 0; j < g_active_stereo_sounds.count; j++) {
        Sound &snd = g_active_stereo_sounds[j];

        output = sound_buffer;
        for (i32 i = 0; i < samples_to_write; i++) {
            *output++ += master_volume * snd.data.samples[snd.cursor++];
            *output++ += master_volume * snd.data.samples[snd.cursor++];

            if (snd.cursor >= snd.data.num_samples) {
                array_remove(&g_active_stereo_sounds, j--);
                break;
            }
        }
    }

    for (i32 j = 0; j < g_active_looping_mono_sounds.count; j++) {
        Sound &snd = g_active_looping_mono_sounds[j];

        output = sound_buffer;
        for (i32 i = 0; i < samples_to_write; i++) {
            *output++ += master_volume * snd.data.samples[snd.cursor];
            *output++ += master_volume * snd.data.samples[snd.cursor];
            snd.cursor++;

            if (snd.cursor >= snd.data.num_samples) {
                snd.cursor = 0;
            }
        }
    }
}

void play_sound(SoundData data)
{
    Sound sound = {};
    sound.data = data;

    switch (data.num_channels) {
    case 1:
        array_add(&g_active_mono_sounds, sound);
        break;
    case 2:
        array_add(&g_active_stereo_sounds, sound);
        break;
    default:
        LOG("unsupported number of channels: %d", data.num_channels);
        break;
    }
}

void play_looping_sound(SoundData data)
{
    Sound sound = {};
    sound.data = data;

    switch (data.num_channels) {
    case 1:
        array_add(&g_active_looping_mono_sounds, sound);
        break;
    default:
        LOG("unsupported number of channels: %d", data.num_channels);
        break;
    }
}
