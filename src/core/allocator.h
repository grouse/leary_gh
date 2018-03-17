/**
 * file:    allocator.h
 * created: 2017-03-20
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "core/types.h"
#include "platform/thread.h"
#include "leary_macros.h"

struct Allocator {
    void  *mem;
    isize size;
    isize remaining;

    Mutex mutex;

    struct FreeBlock {
        isize size;
        FreeBlock *next;
    };

    union {
        struct { // linear allocator
            void *current;
            void *last;
        };

        struct { // stack allocator
            void *sp;
        };

        struct { // heap allocator
            FreeBlock *free;
        };
    };

    using alloc_t   = void* (Allocator *a, isize size);
    using dealloc_t = void  (Allocator *a, void *ptr);
    using realloc_t = void* (Allocator *a, void *ptr, isize size);
    using reset_t      = void  (Allocator *a, void *ptr);

    alloc_t   *alloc   = nullptr;
    dealloc_t *dealloc = nullptr;
    realloc_t *realloc = nullptr;
    reset_t      *reset   = nullptr;
};

void* alloc(Allocator *a, isize size)
{
    ASSERT(a->alloc != nullptr);
    return a->alloc(a, size);
}

void dealloc(Allocator *a, void *ptr)
{
    ASSERT(a->dealloc != nullptr);
    a->dealloc(a, ptr);
}

void* realloc(Allocator *a, void *ptr, isize size)
{
    ASSERT(a->realloc != nullptr);
    return a->realloc(a, ptr, size);
}

void reset(Allocator *a, void *ptr)
{
    ASSERT(a->reset != nullptr);
    a->reset(a, ptr);
}

template<typename T>
T* alloc_array(Allocator *a, i32 count)
{
    return (T*)alloc(a, sizeof(T) * count);
}

template<typename T>
T* realloc_array(Allocator *a, T *current, i32 capacity)
{
    return (T*)realloc(a, current, capacity * sizeof(T));
}

template<typename T>
T* ialloc(Allocator *a)
{
    T *ptr = (T*)alloc(a, sizeof(T));
    *ptr = {};
    return ptr;
}

template<typename T>
T* ialloc_array(Allocator *a, i32 count, T value = {})
{
    T *ptr = (T*)alloc(a, sizeof(T) * count);
    for (i32 i = 0; i < count; i++) {
        ptr[i] = value;
    }
    return ptr;
}

#endif /* ALLOCATOR_H */

