/**
 * file:    benchmark.cpp
 * created: 2017-03-17
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include <time.h>

#include "platform/platform.h"
#include "platform/linux_debug.cpp"
#include "core/allocator.cpp"

timespec get_time()
{
	timespec ts;
	i32 result = clock_gettime(CLOCK_MONOTONIC, &ts);
	(void)result;
	return ts;
}

i64 get_time_difference(timespec start, timespec end)
{
	i64 difference = (end.tv_sec - start.tv_sec) * 1000000000 +
	                 (end.tv_nsec - start.tv_nsec);
	return difference;
}

#define NUM_16B   (50000)
#define NUM_32B   (50000)
#define NUM_64B   (50000)
#define NUM_128B  (50000)
#define NUM_256B  (50000)
#define NUM_512B  (50000)
#define NUM_1KB   (50000)
#define NUM_2KB   (50000)
#define NUM_4KB   (50000)
#define NUM_8KB   (25600)
#define NUM_16KB  (12800)
#define NUM_32KB  (6400)
#define NUM_64KB  (3200)
#define NUM_128KB (1600)
#define NUM_256KB (800)
#define NUM_512KB (400)
#define NUM_1MB   (200)
#define NUM_2MB   (100)

void print_header(const char *name)
{
	printf("\n%s\n", name);
	printf("allocations\tsize\ttotal duration (us)\tper allocation (us)\n");
}

void print_result(i64 duration, isize size, i32 count)
{
	i32 psize       = 0;
	const char *fmt = nullptr;

	if (size >= 1024 * 1024) {
		psize = size / (1024 * 1024);
		fmt   = "%d\t%d MB\t%f\t%f\n";
	} else if (size >= 1024) {
		psize = size / (1024);
		fmt   = "%d\t%d KB\t%f\t%f\n";
	} else {
		psize = size;
		fmt   = "%d\t%d B\t%f\t%f\n";
	}

	printf(fmt, count, psize, duration / 1000.0f, duration / 1000.0f / count);
}


void benchmark_stack(isize alloc_size, i32 count)
{
	isize size = (alloc_size + 16) * 2;
	void *buffer = malloc(size);

	StackAllocator stack = make_stack_allocator(buffer, size);

	timespec start = get_time();

	for (i32 i = 0; i < count; i++) {
		void *tmp = alloc(&stack, alloc_size);
		dealloc(&stack, tmp);
	}

	timespec end = get_time();

	i64 duration = get_time_difference(start, end);
	print_result(duration, alloc_size, count);

	free(buffer);
}

void benchmark_linear(isize alloc_size, i32 count)
{
	isize size = (alloc_size + 24) * count;
	void *buffer = malloc(size);

	LinearAllocator a = make_linear_allocator(buffer, size);
	timespec start = get_time();

	for (i32 i = 0; i < count; i++) {
		void *tmp = alloc(&a, alloc_size);
		(void)tmp;
	}

	reset(&a);

	timespec end = get_time();

	i64 duration = get_time_difference(start, end);
	print_result(duration, alloc_size, count);

	free(buffer);
}

void benchmark_free_list(isize alloc_size, i32 count)
{
	isize size = (alloc_size + 24) * count;
	void *buffer = malloc(size);

	auto a = make_free_list_allocator(buffer, size);

	timespec start = get_time();

	for (i32 i = 0; i < count; i++) {
		void *tmp = alloc(&a, alloc_size);
		dealloc(&a, tmp);
	}

	timespec end = get_time();

	i64 duration = get_time_difference(start, end);
	print_result(duration, alloc_size, count);

	free(buffer);
}

void benchmark_malloc(isize alloc_size, i32 count)
{
	uptr *ptrs = (uptr*)malloc(sizeof(uptr) * count);

	timespec start = get_time();

	for (i32 i = 0; i < count; i++) {
		void *tmp = malloc(alloc_size);
		ptrs[i] = (uptr)tmp;
	}

	for (i32 i = 0; i < count; i++) {
		free((void*)ptrs[i]);
	}

	timespec end = get_time();

	i64 duration = get_time_difference(start, end);
	print_result(duration, alloc_size, count);
}

int main(int, char**)
{
	print_header("system malloc");
	benchmark_malloc(16,                  NUM_16B);
	benchmark_malloc(32,                  NUM_32B);
	benchmark_malloc(64,                  NUM_64B);
	benchmark_malloc(128,                 NUM_128B);
	benchmark_malloc(256,                 NUM_256B);
	benchmark_malloc(512,                 NUM_512B);

	benchmark_malloc(1 * 1024,            NUM_1KB);
	benchmark_malloc(2 * 1024,            NUM_2KB);
	benchmark_malloc(4 * 1024,            NUM_4KB);
	benchmark_malloc(8 * 1024,            NUM_8KB);
	benchmark_malloc(16 * 1024,           NUM_16KB);
	benchmark_malloc(32 * 1024,           NUM_32KB);
	benchmark_malloc(64 * 1024,           NUM_64KB);
	benchmark_malloc(128 * 1024,          NUM_128KB);
	benchmark_malloc(256 * 1024,          NUM_256KB);
	benchmark_malloc(512 * 1024,          NUM_512KB);

	benchmark_malloc(1 * 1024 * 1024,     NUM_1MB);
	benchmark_malloc(2 * 1024 * 1024,     NUM_2MB);


	print_header("custom linear");
	benchmark_linear(16,                  NUM_16B);
	benchmark_linear(32,                  NUM_32B);
	benchmark_linear(64,                  NUM_64B);
	benchmark_linear(128,                 NUM_128B);
	benchmark_linear(256,                 NUM_256B);
	benchmark_linear(512,                 NUM_512B);

	benchmark_linear(1 * 1024,            NUM_1KB);
	benchmark_linear(2 * 1024,            NUM_2KB);
	benchmark_linear(4 * 1024,            NUM_4KB);
	benchmark_linear(8 * 1024,            NUM_8KB);
	benchmark_linear(16 * 1024,           NUM_16KB);
	benchmark_linear(32 * 1024,           NUM_32KB);
	benchmark_linear(64 * 1024,           NUM_64KB);
	benchmark_linear(128 * 1024,          NUM_128KB);
	benchmark_linear(256 * 1024,          NUM_256KB);
	benchmark_linear(512 * 1024,          NUM_512KB);

	benchmark_linear(1 * 1024 * 1024,     NUM_1MB);
	benchmark_linear(2 * 1024 * 1024,     NUM_2MB);


	print_header("custom stack");
	benchmark_stack(16,                  NUM_16B);
	benchmark_stack(32,                  NUM_32B);
	benchmark_stack(64,                  NUM_64B);
	benchmark_stack(128,                 NUM_128B);
	benchmark_stack(256,                 NUM_256B);
	benchmark_stack(512,                 NUM_512B);

	benchmark_stack(1 * 1024,            NUM_1KB);
	benchmark_stack(2 * 1024,            NUM_2KB);
	benchmark_stack(4 * 1024,            NUM_4KB);
	benchmark_stack(8 * 1024,            NUM_8KB);
	benchmark_stack(16 * 1024,           NUM_16KB);
	benchmark_stack(32 * 1024,           NUM_32KB);
	benchmark_stack(64 * 1024,           NUM_64KB);
	benchmark_stack(128 * 1024,          NUM_128KB);
	benchmark_stack(256 * 1024,          NUM_256KB);
	benchmark_stack(512 * 1024,          NUM_512KB);

	benchmark_stack(1 * 1024 * 1024,     NUM_1MB);
	benchmark_stack(2 * 1024 * 1024,     NUM_2MB);


	print_header("custom free_list");
	benchmark_free_list(16,                  NUM_16B);
	benchmark_free_list(32,                  NUM_32B);
	benchmark_free_list(64,                  NUM_64B);
	benchmark_free_list(128,                 NUM_128B);
	benchmark_free_list(256,                 NUM_256B);
	benchmark_free_list(512,                 NUM_512B);

	benchmark_free_list(1 * 1024,            NUM_1KB);
	benchmark_free_list(2 * 1024,            NUM_2KB);
	benchmark_free_list(4 * 1024,            NUM_4KB);
	benchmark_free_list(8 * 1024,            NUM_8KB);
	benchmark_free_list(16 * 1024,           NUM_16KB);
	benchmark_free_list(32 * 1024,           NUM_32KB);
	benchmark_free_list(64 * 1024,           NUM_64KB);
	benchmark_free_list(128 * 1024,          NUM_128KB);
	benchmark_free_list(256 * 1024,          NUM_256KB);
	benchmark_free_list(512 * 1024,          NUM_512KB);

	benchmark_free_list(1 * 1024 * 1024,     NUM_1MB);
	benchmark_free_list(2 * 1024 * 1024,     NUM_2MB);

	return 0;
}


