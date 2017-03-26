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

#define NUM_16B  (50000)
#define NUM_256B (5000)
#define NUM_8K   (500)
#define NUM_2MB  (50)

void print_result(i64 duration, isize size, i32 count)
{
	i32 psize       = 0;
	const char *fmt = nullptr;

	if (size > 1024 * 1024) {
		psize = size / (1024 * 1024);
		fmt   = "- %d MB [%d]: %f us (%f us per alloc)";
	} else if (size > 1024) {
		psize = size / (1024);
		fmt   = "- %d KB [%d]: %f us (%f us per alloc)";
	} else {
		psize = size;
		fmt   = "- %d B [%d]: %f us (%f us per alloc)";
	}

	DEBUG_LOG(fmt, psize, count, duration / 1000.0f, duration / 1000.0f / count);
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
	free(buffer);

	i64 duration = get_time_difference(start, end);
	print_result(duration, alloc_size, count);
}

void benchmark_linear(isize alloc_size, i32 count)
{
	isize size = alloc_size * (count + 1) * 2;
	void *buffer = malloc(size);

	LinearAllocator a = make_linear_allocator(buffer, size);

	timespec start = get_time();

	for (i32 i = 0; i < count; i++) {
		void *tmp = alloc(&a, alloc_size);
		(void)tmp;
	}

	reset(&a);

	timespec end = get_time();
	free(buffer);

	i64 duration = get_time_difference(start, end);
	print_result(duration, alloc_size, count);
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
	free(buffer);

	i64 duration = get_time_difference(start, end);
	print_result(duration, alloc_size, count);
}

void benchmark_malloc(isize alloc_size, i32 count)
{
	timespec start = get_time();

	for (i32 i = 0; i < count; i++) {
		void *tmp = malloc(alloc_size);
		free(tmp);
	}

	timespec end = get_time();

	i64 duration = get_time_difference(start, end);
	print_result(duration, alloc_size, count);
}

int main(int, char**)
{
	DEBUG_LOG("system malloc:");
	benchmark_malloc(16,                  NUM_16B);
	benchmark_malloc(256,                 NUM_256B);
	benchmark_malloc(8 * 1024,            NUM_8K);
	benchmark_malloc(2 * 1024  * 1024,    NUM_2MB);

	DEBUG_LOG("");

	DEBUG_LOG("custom linear:");
	benchmark_linear(16,                  NUM_16B);
	benchmark_linear(256,                 NUM_256B);
	benchmark_linear(8 * 1024,            NUM_8K);
	benchmark_linear(2 * 1024  * 1024,    NUM_2MB);

	DEBUG_LOG("");

	DEBUG_LOG("custom free_list:");
	benchmark_free_list(16,               NUM_16B);
	benchmark_free_list(256,              NUM_256B);
	benchmark_free_list(8 * 1024,         NUM_8K);
	benchmark_free_list(2 * 1024  * 1024, NUM_2MB);

	DEBUG_LOG("");

	DEBUG_LOG("custom stack:");
	benchmark_stack(16,                   NUM_16B);
	benchmark_stack(256,                  NUM_256B);
	benchmark_stack(8 * 1024,             NUM_8K);
	benchmark_stack(2 * 1024  * 1024,     NUM_2MB);

	return 0;
}


