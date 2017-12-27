/**
 * file:    types.h
 * created: 2017-01-29
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#ifndef TYPES_H
#define TYPES_H

#include <stddef.h>
#include <stdint.h>

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef intptr_t  iptr;
typedef uintptr_t uptr;

typedef size_t   usize;
typedef iptr     isize;

typedef float    f32;
typedef double   f64;

#define F32_MAX 3.402823466e+38F

#define U64_MAX UINT64_MAX
#define I64_MAX INT64_MAX
#define I32_MAX INT32_MAX


#endif /* TYPES_H */

