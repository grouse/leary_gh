/**
 * file:    allocator.h
 * created: 2017-03-20
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017-2018 - all rights reserved
 */

#define alloc_array(a, T, count) (T*)alloc(a, sizeof(T) * count)

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
            void *last_;
        };

        struct { // heap allocator
            FreeBlock *free;
        };
    };

    using alloc_t   = void* (Allocator *a, isize size);
    using dealloc_t = void  (Allocator *a, void *ptr);
    using realloc_t = void* (Allocator *a, void *ptr, isize size);
    using reset_t   = void  (Allocator *a, void *ptr);

    alloc_t   *alloc    = nullptr;
    dealloc_t *dealloc  = nullptr;
    realloc_t *realloc  = nullptr;
    reset_t      *reset = nullptr;
};

Allocator stack_allocator(void *mem, isize size);
Allocator linear_allocator(void *mem, isize size);
Allocator heap_allocator(void *mem, isize size);
Allocator system_allocator();

// TODO(jesper): replace these with macros so we can track allocation context
void* alloc(Allocator *a, isize size);
void dealloc(Allocator *a, void *ptr);

template<typename T>
T* ialloc(Allocator *a);

template<typename T>
T* ialloc_array(Allocator *a, i32 count, T value = {});