/**
 * file:    allocator.cpp
 * created: 2017-03-12
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include "allocator.h"

#include "platform/platform.h"
#include "leary_macros.h"

#include <stdlib.h>

// TODO(jesper): zalloc - alloc and zero memset
// TODO(jesper): ialloc - alloc and default initialise struct

struct AllocationHeader {
    isize size;
    void *unaligned;
};

static void *align_address(void *address, u8 alignment, u8 header_size)
{
    u8 adjustment = alignment - ((uptr)address & ((uptr)alignment - 1));
    if (adjustment == alignment) {
        adjustment = 0;
    }

    u8 required = header_size;
    if (adjustment < required) {
        required -= adjustment;

        adjustment += alignment * (required / alignment);
        if (required % alignment > 0) {
            adjustment += alignment;
        }
    }

    return (void*)((uptr)address + adjustment);
}

static u8 align_address_adjustment(void *address, u8 alignment, u8 header_size)
{
    uptr aligned = (uptr)align_address(address, alignment, header_size);
    return (u8)(aligned - (uptr)address);
}


void* stack_alloc(Allocator *a, isize size);
void* stack_realloc(Allocator *a, void *ptr, isize size);
void stack_dealloc(Allocator *a, void *ptr);
void stack_reset(Allocator *a, void *ptr);

void* linear_alloc(Allocator *a, isize size);
void* linear_realloc(Allocator *a, void *ptr, isize size);
void linear_dealloc(Allocator *a, void *ptr);
void linear_reset(Allocator *a, void *ptr);

void* heap_alloc(Allocator *a, isize size);
void* heap_realloc(Allocator *a, void *ptr, isize size);
void heap_dealloc(Allocator *a, void *ptr);

void* system_alloc(Allocator *a, isize size);
void* system_realloc(Allocator *a, void *ptr, isize size);
void system_dealloc(Allocator *a, void *ptr);

Allocator stack_allocator(void *mem, isize size)
{
    Allocator a = {};
    init_mutex(&a.mutex);

    a.mem       = mem;
    a.size      = size;
    a.remaining = a.size;

    a.sp = a.mem;

    a.alloc   = &stack_alloc;
    a.dealloc = &stack_dealloc;
    a.realloc = &stack_realloc;
    a.reset   = &stack_reset;

    return a;
}

Allocator linear_allocator(void *mem, isize size)
{
    Allocator a = {};
    init_mutex(&a.mutex);

    a.mem       = mem;
    a.size      = size;
    a.remaining = a.size;

    a.current = a.mem;
    a.last    = nullptr;

    a.alloc   = &linear_alloc;
    a.dealloc = &linear_dealloc;
    a.realloc = &linear_realloc;
    a.reset   = &linear_reset;

    return a;
}

Allocator heap_allocator(void *mem, isize size)
{
    Allocator a = {};
    init_mutex(&a.mutex);

    a.mem       = mem;
    a.size      = size;
    a.remaining = a.size;

    a.free       = (Allocator::FreeBlock*)a.mem;
    a.free->size = a.size;
    a.free->next = nullptr;

    a.alloc   = &heap_alloc;
    a.dealloc = &heap_dealloc;
    a.realloc = &heap_realloc;
    a.reset   = nullptr;

    return a;
}

Allocator system_allocator()
{
    Allocator a = {};

    a.alloc   = &system_alloc;
    a.dealloc = &system_dealloc;
    a.realloc = &system_realloc;
    a.reset   = nullptr;

    return a;
}



void* stack_alloc(Allocator *a, isize asize)
{
    u8 header_size = sizeof(AllocationHeader);

    void *unaligned = a->sp;
    void *aligned   = align_address(unaligned, 16, header_size);

    a->sp = (void*)((uptr)aligned + asize);
    a->remaining = a->size - (isize)((uptr)a->sp - (uptr)a->mem);

    ASSERT(a->remaining > 0);

    auto header = (AllocationHeader*)((uptr)aligned - header_size);
    header->size      = asize;
    header->unaligned = unaligned;

    return aligned;
}

void stack_dealloc(Allocator *a, void *ptr)
{
    if (ptr == nullptr) {
        return;
    }

    auto header  = (AllocationHeader*)((uptr)ptr - sizeof(AllocationHeader));
    a->sp        = header->unaligned;
    a->remaining = a->size - (isize)((uptr)a->sp - (uptr)a->mem);
}

void* stack_realloc(Allocator *a, void *ptr, isize asize)
{
    if (ptr == nullptr) {
        return stack_alloc(a, asize);
    }

    // NOTE(jesper): reallocing can be bad as we'll almost certainly leak the
    // memory, but for the general use case this should be fine
    auto header = (AllocationHeader*)((uptr)ptr - sizeof(AllocationHeader));
    if ((uptr)ptr + header->size == (uptr)a->sp) {
        isize extra = asize - header->size;
        ASSERT(extra > 0); // NOTE(jesper): untested

        a->sp        = (void*)((uptr)a->sp + extra);
        a->remaining = a->size - (isize)((uptr)a->sp - (uptr)a->mem);
        ASSERT(a->remaining > 0);

        header->size = asize;
        return ptr;
    } else {
        //LOG("can't expand stack allocation, leaking memory");
        void *nptr = stack_alloc(a, asize);
        memcpy(nptr, ptr, header->size);
        return nptr;
    }
}

void stack_reset(Allocator *a, void *ptr)
{
    a->sp        = ptr;
    a->remaining = a->size - (isize)((uptr)a->sp - (uptr)a->mem);
}



void* linear_alloc(Allocator *a, isize asize)
{
    lock_mutex(&a->mutex);
    defer { unlock_mutex(&a->mutex); };

    u8 header_size = sizeof(AllocationHeader);

    void *unaligned = a->current;
    void *aligned   = align_address(unaligned, 16, header_size);

    a->current = (void*)((uptr)aligned + asize);
    a->remaining = a->size - (isize)((uptr)a->current - (uptr)a->mem);
    ASSERT((uptr)a->current < ((uptr)a->mem + a->size));

    AllocationHeader *header = (AllocationHeader*)((uptr)aligned - header_size);
    header->size      = asize;
    header->unaligned = unaligned;

    return aligned;
}

void linear_dealloc(Allocator *a, void *ptr)
{
    if (ptr == nullptr) {
        return;
    }

    lock_mutex(&a->mutex);
    defer { unlock_mutex(&a->mutex); };

    auto header = (AllocationHeader*)((uptr)ptr - sizeof(AllocationHeader));
    if ((uptr)ptr + header->size == (uptr)a->current) {
        a->current = header->unaligned;
        a->remaining = a->size - (isize)((uptr)a->current - (uptr)a->mem);
    } else {
        LOG("calling dealloc on linear allocator, leaking memory");
    }
}

void* linear_realloc(Allocator *a, void *ptr, isize asize)
{
    if (ptr == nullptr) {
        return linear_alloc(a, asize);
    }

    lock_mutex(&a->mutex);
    defer { unlock_mutex(&a->mutex); };

    auto header = (AllocationHeader*)((uptr)ptr - sizeof(AllocationHeader));
    if ((uptr)ptr + header->size == (uptr)a->current) {
        isize extra = asize - header->size;
        ASSERT(extra > 0); // NOTE(jesper): untested

        a->current   = (void*)((uptr)a->current + extra);
        a->remaining = a->size - (isize)((uptr)a->current - (uptr)a->mem);
        header->size = asize;
        return ptr;
    } else {
        //LOG("can't expand linear allocation, leaking memory");
        void *nptr = linear_alloc(a, asize);
        memcpy(nptr, ptr, header->size);
        return nptr;
    }
}

void linear_reset(Allocator *a, void*)
{
    lock_mutex(&a->mutex);
    defer { unlock_mutex(&a->mutex); };

    a->current   = a->mem;
    a->remaining = a->size - (isize)((uptr)a->current - (uptr)a->mem);
}



void* heap_alloc(Allocator *a, isize asize)
{
    using FreeBlock = Allocator::FreeBlock;

    u8 header_size = sizeof(AllocationHeader);
    isize required = asize + header_size + 16;

    FreeBlock *prev = nullptr;
    FreeBlock *fb = a->free;
    while(fb != nullptr) {
        u8 adjustment = align_address_adjustment(fb, 16, header_size);
        if (fb->size > (asize + adjustment)) {
            break;
        }

        // TODO(jesper): find best fitting free block
        prev = fb;
        fb   = a->free->next;
    }

    ASSERT(((uptr)fb + asize) < ((uptr)a->mem + a->size));
    ASSERT(fb && fb->size >= required);
    if (fb == nullptr || fb->size < required) {
        return nullptr;
    }

    void *unaligned = (void*)fb;
    void *aligned   = align_address(unaligned, 16, header_size);

    isize free_size = fb->size;
    isize rem       = free_size - asize;

    // TODO(jesper): double check suitable value of minimum remaining
    if (rem < (header_size + 48)) {
        a->remaining -= free_size;

        asize = free_size;

        if (prev != nullptr) {
            prev->next = fb->next;
        } else {
            a->free = a->free->next;
        }
    } else {
        a->remaining -= asize;

        FreeBlock *nfree = (FreeBlock*)((uptr)aligned + asize);
        nfree->size = rem;
        nfree->next = a->free->next;

        if (prev != nullptr) {
            prev->next = nfree;
        } else {
            a->free = nfree;
        }
    }

    auto header = (AllocationHeader*)((uptr)aligned - header_size);
    header->size      = asize;
    header->unaligned = unaligned;

    return aligned;
}

void heap_dealloc(Allocator *a, void *ptr)
{
    using FreeBlock = Allocator::FreeBlock;

    if (ptr == nullptr) {
        return;
    }

    auto  header = (AllocationHeader*)((uptr)ptr - sizeof(AllocationHeader));
    isize asize  = header->size;

    a->remaining += asize;

    uptr m = (uptr)header->unaligned;
    uptr end = m + asize;

    FreeBlock *nfree = (FreeBlock*)m;
    nfree->size      = asize;

    bool expanded   = false;
    bool insert     = false;
    FreeBlock *prev = nullptr;
    FreeBlock *fb   = a->free;
    FreeBlock *next = nullptr;

    while (fb != nullptr) {
        if (end == (uptr)fb) {
            nfree->size = nfree->size + fb->size;
            nfree->next = fb->next;

            if (prev != nullptr) {
                prev->next = nfree;
            }
            expanded = true;
            break;
        } else if (((uptr)fb + fb->size) == m) {
            fb->size = fb->size + nfree->size;
            expanded = true;
            break;
        } else if ((uptr)fb > m) {
            prev = fb;
            next = fb->next;
            insert = true;
            break;
        }

        prev = fb;
        fb   = fb->next;
    }

    ASSERT(insert || expanded);
    if (!expanded) {
        prev->next  = nfree;
        nfree->next = next;
    }
}

void* heap_realloc(Allocator *a, void *ptr, isize asize)
{
    if (ptr == nullptr) {
        return heap_alloc(a, asize);
    }

    auto header = (AllocationHeader*)((uptr)ptr - sizeof(AllocationHeader));

    // TODO(jesper): try to find neighbour FreeBlock and expand
    void *nptr = heap_alloc(a, asize);
    memcpy(nptr, ptr, header->size);
    heap_dealloc(a, ptr);

    return nptr;
}


void* system_alloc(Allocator *a, isize asize)
{
    (void)a;
    return malloc(asize);
}

void system_dealloc(Allocator *a, void *ptr)
{
    (void)a;
    free(ptr);
}

void* system_realloc(Allocator *a, void *ptr, isize asize)
{
    (void)a;
    return ::realloc(ptr, asize);
}
