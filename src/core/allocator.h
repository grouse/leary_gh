/**
 * file:    allocator.h
 * created: 2017-03-20
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#ifndef ALLOCATOR_H
#define ALLOCATOR_H


struct Allocator {
    void  *mem;
    isize size;
    isize remaining;

    virtual void* alloc  (isize size)             = 0;
    virtual void  dealloc(void  *ptr)             = 0;
    virtual void* realloc(void  *ptr, isize size) = 0;
    virtual void  reset  (void  *) {}
    virtual void  reset  ()        {}

    template <typename T>
    inline T* talloc()
    {
        return (T*)alloc(sizeof(T));
    }

    template <typename T>
    inline T* ialloc()
    {
        T *mem = (T*)alloc(sizeof(T));
        T e    = {};
        memcpy(mem, &e, sizeof(T));
        return mem;
    }

    template <typename T>
    inline T* alloc_array(isize count)
    {
        return (T*)alloc(sizeof(T) * count);
    }

    template <typename T>
    inline T* zalloc_array(isize count)
    {
        T *ptr = (T*)alloc(sizeof(T) * count);
        memset(ptr, 0, sizeof(T) * count);
        return ptr;
    }

    template <typename T>
    inline T* ialloc_array(isize count)
    {
        T *ptr = (T*)alloc(sizeof(T) * count);
        T val  = {};
        for (i32 i = 0; i < count; i++) {
            ptr[i] = val;
        }
        return ptr;
    }

    template <typename T>
    inline T* ialloc_array(isize count, T val)
    {
        T *ptr = (T*)alloc(sizeof(T) * count);
        for (i32 i = 0; i < count; i++) {
            ptr[i] = val;
        }
        return ptr;
    }

    template<typename T>
    inline T* realloc_array(T *ptr, isize capacity)
    {
        isize size = capacity * sizeof(T);
        return (T*)realloc(ptr, size);
    }
};


struct SystemAllocator : public Allocator {
    void* alloc  (isize size)             override;
    void  dealloc(void  *ptr)             override;
    void* realloc(void  *ptr, isize size) override;
};

struct LinearAllocator : public Allocator {
    void *current;
    void *last;

    LinearAllocator(void *mem, isize size);

    void* alloc  (isize size)             override;
    void  dealloc(void  *ptr)             override;
    void* realloc(void  *ptr, isize size) override;
    void  reset  ()                       override;
};

struct StackAllocator : public Allocator {
    void *sp;

    StackAllocator(void *mem, isize size);

    void* alloc  (isize size)             override;
    void  dealloc(void  *ptr)             override;
    void* realloc(void  *ptr, isize size) override;
    void  reset  (void  *ptr)             override;
};

struct HeapAllocator : public Allocator {
    struct FreeBlock {
        isize size;
        FreeBlock *next;
    };

    FreeBlock *free;

    HeapAllocator(void *mem, isize size);

    void* alloc  (isize size)             override;
    void  dealloc(void  *ptr)             override;
    void* realloc(void  *ptr, isize size) override;
};

#endif /* ALLOCATOR_H */

