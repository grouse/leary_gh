/**
 * file:    allocator.cpp
 * created: 2017-08-21
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

bool allocators_test_heap()
{
    TEST_START("allocators::heap");
    bool result = true;

    isize size          = 64 * 1024;
    void *mem           = malloc(size);
    HeapAllocator *heap = new HeapAllocator(mem, size);
    defer {
        free(mem);
        delete heap;
    };

    CHECK(result, heap->free != nullptr);
    CHECK(result, heap->free == mem);
    CHECK(result, heap->free->size == size);

    void *p = heap->alloc(16); (void)p;
    CHECK(result, heap->free != mem);
    CHECK(result, heap->free->size == (size - 16));

    return result;
}


bool allocators_test()
{
    TEST_START("allocators");
    bool result = true;
    result = result && allocators_test_heap();
    return result;
}

