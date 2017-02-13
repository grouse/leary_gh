/**
 * file:    types.h
 * created: 2017-01-29
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef size_t   usize;
#if defined (_WIN32)
#include <BaseTsd.h>
typedef SSIZE_T isize;
#else
typedef ssize_t  isize;
#endif

typedef float    f32;
typedef double   f64;


#endif /* TYPES_H */

