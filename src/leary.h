/**
 * file:    leary.h
 * created: 2017-03-18
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#ifndef LEARY_H
#define LEARY_H

#ifndef INTROSPECT
#define INTROSPECT
#endif

#include <math.h>
#include <assert.h>
#include <utility>

#include "leary_macros.h"

#include "core/types.h"
#include "core/allocator.h"
#include "core/string.h"

INTROSPECT struct Resolution
{
    i32 width  = 1280;
    i32 height = 720;
};

INTROSPECT struct VideoSettings
{
    Resolution resolution;

    i16 fullscreen = 0;
    i16 vsync      = 1;
};

INTROSPECT struct Settings
{
    VideoSettings video;
};

#endif /* LEARY_H */

