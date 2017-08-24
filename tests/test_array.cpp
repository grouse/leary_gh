/**
 * file:    test_array.cpp
 * created: 2017-08-24
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

bool test_array()
{
    TEST_START("array");
    bool result = true;

    SystemAllocator a = {};
    Array<i32> arr = {};
    arr.allocator = &a;

    for (i32 i = 0; i < 10; i++) {
        array_add(&arr, i);
    }
    CHECK(result, arr.count == 10);
    CHECK(result, arr.capacity >= arr.count);

    for (i32 i = 0; i < 10; i++) {
        CHECK(result, arr[i] == i);
    }

    array_remove(&arr, 2);
    CHECK(result, arr.count == 9);
    CHECK(result, arr.capacity >= arr.count);
    CHECK(result, arr[0] == 0);
    CHECK(result, arr[1] == 1);
    CHECK(result, arr[2] == 9);
    CHECK(result, arr[3] == 3);
    CHECK(result, arr[arr.count-1] == 8);

    array_remove_ordered(&arr, 1);
    CHECK(result, arr.count == 8);
    CHECK(result, arr.capacity >= arr.count);
    CHECK(result, arr[0] == 0);
    CHECK(result, arr[1] == 9);
    CHECK(result, arr[2] == 3);
    CHECK(result, arr[3] == 4);
    CHECK(result, arr[arr.count-1] == 8);

    return result;
}

