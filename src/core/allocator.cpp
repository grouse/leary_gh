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

// TODO(jesper): zalloc - allocate and zero memset
// TODO(jesper): ialloc - allocate and default initialise struct

struct AllocationHeader {
    isize size;
    void *unaligned;
};

void *align_address(void *address, u8 alignment, u8 header_size)
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

u8 align_address_adjustment(void *address, u8 alignment, u8 header_size)
{
    uptr aligned = (uptr)align_address(address, alignment, header_size);
    return (u8)(aligned - (uptr)address);
}


void* StackAllocator::alloc(isize asize)
{
    u8 header_size = sizeof(AllocationHeader);

    void *unaligned = this->sp;
    void *aligned   = align_address(unaligned, 16, header_size);

    this->sp = (void*)((uptr)aligned + asize);
    this->remaining = this->size - (isize)((uptr)this->sp - (uptr)this->mem);

    ASSERT(this->remaining > 0);

    auto header = (AllocationHeader*)((uptr)aligned - header_size);
    header->size      = asize;
    header->unaligned = unaligned;

    return aligned;
}

void *LinearAllocator::alloc(isize asize)
{
    u8 header_size = sizeof(AllocationHeader);

    void *unaligned = this->current;
    void *aligned   = align_address(unaligned, 16, header_size);

    this->current = (void*)((uptr)aligned + asize);
    this->remaining = this->size - (isize)((uptr)this->current - (uptr)this->mem);
    ASSERT((uptr)this->current < ((uptr)this->mem + this->size));

    AllocationHeader *header = (AllocationHeader*)((uptr)aligned - header_size);
    header->size      = asize;
    header->unaligned = unaligned;

    return aligned;
}

void *HeapAllocator::alloc(isize asize)
{
    u8 header_size = sizeof(AllocationHeader);
    isize required = asize + header_size + 16;

    FreeBlock *prev = nullptr;
    FreeBlock *fb = this->free;
    while(fb != nullptr) {
        u8 adjustment = align_address_adjustment(fb, 16, header_size);
        if (fb->size > (asize + adjustment)) {
            break;
        }

        // TODO(jesper): find best fitting free block
        prev = fb;
        fb   = free->next;
    }

    ASSERT(((uptr)fb + asize) < ((uptr)this->mem + this->size));
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
        this->remaining -= free_size;

        asize = free_size;

        if (prev != nullptr) {
            prev->next = fb->next;
        } else {
            this->free = this->free->next;
        }
    } else {
        this->remaining -= size;

        FreeBlock *nfree = (FreeBlock*)((uptr)aligned + asize);
        nfree->size = rem;
        nfree->next = free->next;

        if (prev != nullptr) {
            prev->next = nfree;
        } else {
            this->free = nfree;
        }
    }

    auto header = (AllocationHeader*)((uptr)aligned - header_size);
    header->size      = asize;
    header->unaligned = unaligned;

    return aligned;
}


StackAllocator::StackAllocator(void *mem, isize size)
{
    this->mem       = mem;
    this->size      = size;
    this->remaining = size;
    this->sp        = mem;
}

LinearAllocator::LinearAllocator(void *mem, isize size)
{
    this->mem       = mem;
    this->size      = size;
    this->remaining = size;
    this->current   = mem;
    this->last      = nullptr;
}

HeapAllocator::HeapAllocator(void *mem, isize size)
{
    this->mem        = mem;
    this->size       = size;
    this->remaining  = size;
    this->free       = (FreeBlock*)mem;
    this->free->size = size;
    this->free->next = nullptr;
}

void LinearAllocator::dealloc(void *ptr)
{
    if (ptr == nullptr) {
        return;
    }

    auto header = (AllocationHeader*)((uptr)ptr - sizeof(AllocationHeader));
    if ((uptr)ptr + header->size == (uptr)this->current) {
        this->current = header->unaligned;
        this->remaining = this->size - (isize)((uptr)this->current - (uptr)this->mem);
    } else {
        LOG("calling dealloc on linear allocator, leaking memory");
    }
}

void StackAllocator::dealloc(void *ptr)
{
    if (ptr == nullptr) {
        return;
    }

    auto header     = (AllocationHeader*)((uptr)ptr - sizeof(AllocationHeader));
    this->sp        = header->unaligned;
    this->remaining = this->size - (isize)((uptr)this->sp - (uptr)this->mem);
}

void HeapAllocator::dealloc(void *ptr)
{
    if (ptr == nullptr) {
        return;
    }

    auto  header = (AllocationHeader*)((uptr)ptr - sizeof(AllocationHeader));
    isize asize  = header->size;

    this->remaining += asize;

    uptr m = (uptr)header->unaligned;
    uptr end = m + asize;

    FreeBlock *nfree = (FreeBlock*)m;
    nfree->size      = asize;

    bool expanded   = false;
    bool insert     = false;
    FreeBlock *prev = nullptr;
    FreeBlock *fb   = this->free;
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

void* LinearAllocator::realloc(void *ptr, isize asize)
{
    if (ptr == nullptr) {
        return alloc(asize);
    }

    auto header = (AllocationHeader*)((uptr)ptr - sizeof(AllocationHeader));
    if ((uptr)ptr + header->size == (uptr)this->current) {
        isize extra = asize - header->size;
        ASSERT(extra > 0); // NOTE(jesper): untested

        this->current   = (void*)((uptr)this->current + extra);
        this->remaining = this->size - (isize)((uptr)this->current - (uptr)this->mem);
        header->size    = asize;
        return ptr;
    } else {
        LOG("can't expand linear allocation, leaking memory");
        void *nptr = alloc(asize);
        memcpy(nptr, ptr, header->size);
        return nptr;
    }
}

void* StackAllocator::realloc(void *ptr, isize asize)
{
    if (ptr == nullptr) {
        return alloc(asize);
    }

    // NOTE(jesper): reallocing can be bad as we'll almost certainly leak the
    // memory, but for the general use case this should be fine
    auto header = (AllocationHeader*)((uptr)ptr - sizeof(AllocationHeader));
    if ((uptr)ptr + header->size == (uptr)this->sp) {
        isize extra = asize - header->size;
        ASSERT(extra > 0); // NOTE(jesper): untested

        this->sp        = (void*)((uptr)this->sp + extra);
        this->remaining = this->size - (isize)((uptr)this->sp - (uptr)this->mem);
        ASSERT(this->remaining > 0);

        header->size = asize;
        return ptr;
    } else {
        LOG("can't expand stack allocation, leaking memory");
        void *nptr = alloc(asize);
        memcpy(nptr, ptr, header->size);
        return nptr;
    }
}

void* HeapAllocator::realloc(void *ptr, isize asize)
{
    if (ptr == nullptr) {
        return alloc(asize);
    }

    auto header = (AllocationHeader*)((uptr)ptr - sizeof(AllocationHeader));

    // TODO(jesper): try to find neighbour FreeBlock and expand
    void *nptr = alloc(asize);
    memcpy(nptr, ptr, header->size);
    dealloc(ptr);

    return nptr;
}

void LinearAllocator::reset()
{
    this->current   = this->mem;
    this->remaining = this->size - (isize)((uptr)this->current - (uptr)this->mem);
}

void StackAllocator::reset(void *ptr)
{
    this->sp        = ptr;
    this->remaining = this->size - (isize)((uptr)this->sp - (uptr)this->mem);
}

void* SystemAllocator::alloc(isize asize)
{
    return malloc(asize);
}

void SystemAllocator::dealloc(void *ptr)
{
    free(ptr);
}

void* SystemAllocator::realloc(void *ptr, isize asize)
{
    return ::realloc(ptr, asize);
}
