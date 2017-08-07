/**
 * file:    allocator.cpp
 * created: 2017-03-12
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include "allocator.h"

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


void* StackAllocator::alloc(isize size)
{
    u8 header_size = sizeof(AllocationHeader);

    void *unaligned = this->sp;
    void *aligned   = align_address(unaligned, 16, header_size);

    this->sp = (void*)((uptr)aligned + size);
    this->remaining = this->size - (isize)((uptr)this->sp - (uptr)this->mem);

    DEBUG_ASSERT(this->remaining > 0);

    auto header = (AllocationHeader*)((uptr)aligned - header_size);
    header->size      = size;
    header->unaligned = unaligned;

    return aligned;
}

void *LinearAllocator::alloc(isize size)
{
    u8 header_size = sizeof(AllocationHeader);

    void *unaligned = this->current;
    void *aligned   = align_address(unaligned, 16, header_size);

    this->current = (void*)((uptr)aligned + size);
    this->remaining = this->size - (isize)((uptr)this->current - (uptr)this->mem);
    DEBUG_ASSERT((uptr)this->current < ((uptr)this->mem + this->size));

    AllocationHeader *header = (AllocationHeader*)((uptr)aligned - header_size);
    header->size      = size;
    header->unaligned = unaligned;

    return aligned;
}

void *HeapAllocator::alloc(isize size)
{
    u8 header_size = sizeof(AllocationHeader);
    isize required = size + header_size + 16;

    FreeBlock *prev = nullptr;
    FreeBlock *free = this->free;
    while(free != nullptr) {
        u8 adjustment = align_address_adjustment(free, 16, header_size);
        if (free->size > (size + adjustment)) {
            break;
        }

        // TODO(jesper): find best fitting free block
        prev = free;
        free = free->next;
    }

    DEBUG_ASSERT(((uptr)free + size) < ((uptr)this->mem + this->size));
    DEBUG_ASSERT(free && free->size >= required);
    if (free == nullptr || free->size < required) {
        return nullptr;
    }

    void *unaligned = (void*)free;
    void *aligned   = align_address(unaligned, 16, header_size);

    isize free_size = free->size;
    isize remaining = free_size - size;

    // TODO(jesper): double check suitable value of minimum remaining
    if (remaining < (header_size + 48)) {
        this->remaining -= free_size;

        size = free_size;

        if (prev != nullptr) {
            prev->next = free->next;
        } else {
            this->free = this->free->next;
        }
    } else {
        this->remaining -= size;

        FreeBlock *nfree = (FreeBlock*)((uptr)aligned + size);
        nfree->size = remaining;
        nfree->next = free->next;

        if (prev != nullptr) {
            prev->next = nfree;
        } else {
            this->free = nfree;
        }
    }

    auto header = (AllocationHeader*)((uptr)aligned - header_size);
    header->size      = size;
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
        DEBUG_LOG("calling dealloc on linear allocator, leaking memory");
    }
}

void StackAllocator::dealloc(void *ptr)
{
    auto header     = (AllocationHeader*)((uptr)ptr - sizeof(AllocationHeader));
    this->sp        = header->unaligned;
    this->remaining = this->size - (isize)((uptr)this->sp - (uptr)this->mem);
}

void HeapAllocator::dealloc(void *ptr)
{
    auto  header = (AllocationHeader*)((uptr)ptr - sizeof(AllocationHeader));
    isize size   = header->size;

    this->remaining += size;

    uptr mem = (uptr)header->unaligned;
    uptr end = mem + size;

    FreeBlock *nfree = (FreeBlock*)mem;
    nfree->size      = size;

    bool expanded   = false;
    bool insert     = false;
    FreeBlock *prev = nullptr;
    FreeBlock *free = this->free;
    FreeBlock *next = nullptr;

    while (free != nullptr) {
        if (end == (uptr)free) {
            nfree->size = nfree->size + free->size;
            nfree->next = free->next;

            if (prev != nullptr) {
                prev->next = nfree;
            }
            expanded = true;
            break;
        } else if (((uptr)free + free->size) == mem) {
            free->size = free->size + nfree->size;
            expanded = true;
            break;
        } else if ((uptr)free > mem) {
            prev = free;
            next = free->next;
            insert = true;
            break;
        }

        prev = free;
        free = free->next;
    }

    DEBUG_ASSERT(insert || expanded);
    if (!expanded) {
        prev->next  = nfree;
        nfree->next = next;
    }
}

void* LinearAllocator::realloc(void *ptr, isize size)
{
    if (ptr == nullptr) {
        return alloc(size);
    }

    auto header = (AllocationHeader*)((uptr)ptr - sizeof(AllocationHeader));
    if ((uptr)ptr + header->size == (uptr)this->current) {
        isize extra = size - header->size;
        DEBUG_ASSERT(extra > 0); // NOTE(jesper): untested

        this->current   = (void*)((uptr)this->current + extra);
        this->remaining = this->size - (isize)((uptr)this->current - (uptr)this->mem);
        header->size    = size;
        return ptr;
    } else {
        DEBUG_LOG("can't expand linear allocation, leaking memory");
        void *nptr = alloc(size);
        memcpy(nptr, ptr, header->size);
        return nptr;
    }
}

void* StackAllocator::realloc(void *ptr, isize size)
{
    if (ptr == nullptr) {
        return alloc(size);
    }

    // NOTE(jesper): reallocing can be bad as we'll almost certainly leak the
    // memory, but for the general use case this should be fine
    auto header = (AllocationHeader*)((uptr)ptr - sizeof(AllocationHeader));
    if ((uptr)ptr + header->size == (uptr)this->sp) {
        isize extra = size - header->size;
        DEBUG_ASSERT(extra > 0); // NOTE(jesper): untested

        this->sp        = (void*)((uptr)this->sp + extra);
        this->remaining = this->size - (isize)((uptr)this->sp - (uptr)this->mem);
        DEBUG_ASSERT(this->remaining > 0);

        header->size = size;
        return ptr;
    } else {
        DEBUG_LOG("can't expand stack allocation, leaking memory");
        void *nptr = alloc(size);
        memcpy(nptr, ptr, header->size);
        return nptr;
    }
}

void* HeapAllocator::realloc(void *ptr, isize size)
{
    if (ptr == nullptr) {
        return alloc(size);
    }

    auto header = (AllocationHeader*)((uptr)ptr - sizeof(AllocationHeader));

    // TODO(jesper): try to find neighbour FreeBlock and expand
    void *nptr = alloc(size);
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

void* SystemAllocator::alloc(isize size)
{
    return malloc(size);
}

void SystemAllocator::dealloc(void *ptr)
{
    free(ptr);
}

void* SystemAllocator::realloc(void *ptr, isize size)
{
    return ::realloc(ptr, size);
}
