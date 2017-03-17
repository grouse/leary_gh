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
	VAR_UNUSED(result);
	return ts;
}

i64 get_time_difference(timespec start, timespec end)
{
	i64 difference = (end.tv_sec - start.tv_sec) * 1000000000 +
	                 (end.tv_nsec - start.tv_nsec);
	return difference;
}

i64 benchmark_stack_allocator()
{
	isize size = 256 * 1024 * 1024;
	void *buffer = malloc(size);

	StackAllocator stack = make_stack_allocator(buffer, size);

	timespec start = get_time();

	for (i32 i = 0; i < 10000; i++) {
		void *tmp = alloc(&stack, 16);
		dealloc(&stack, tmp);
	}

	for (i32 i = 0; i < 1000; i++) {
		void *tmp = alloc(&stack, 256);
		dealloc(&stack, tmp);
	}

	for (i32 i = 0; i < 50; i++) {
		void *tmp = alloc(&stack, 2 * 1024 * 1024);
		dealloc(&stack, tmp);
	}

	timespec end = get_time();
	free(buffer);

	return get_time_difference(start, end);
}

i64 benchmark_linear_allocator()
{
	isize size = 256 * 1024 * 1024;
	void *buffer = malloc(size);

	LinearAllocator a = make_linear_allocator(buffer, size);

	timespec start = get_time();

	for (i32 i = 0; i < 10000; i++) {
		void *tmp = alloc(&a, 16);
		VAR_UNUSED(tmp);
	}

	reset(&a);

	for (i32 i = 0; i < 1000; i++) {
		void *tmp = alloc(&a, 256);
		VAR_UNUSED(tmp);
	}

	reset(&a);

	for (i32 i = 0; i < 50; i++) {
		void *tmp = alloc(&a, 2 * 1024 * 1024);
		VAR_UNUSED(tmp);
	}
	reset(&a);

	timespec end = get_time();
	free(buffer);

	return get_time_difference(start, end);
}

i64 benchmark_malloc()
{
	timespec start = get_time();

	for (i32 i = 0; i < 10000; i++) {
		void *tmp = malloc(16);
		free(tmp);
	}

	for (i32 i = 0; i < 1000; i++) {
		void *tmp = malloc(256);
		free(tmp);
	}

	for (i32 i = 0; i < 50; i++) {
		void *tmp = malloc(2 * 1024 * 1024);
		free(tmp);
	}

	timespec end = get_time();
	return get_time_difference(start, end);
}

#define MIN(a, b) (a) < (b) ? (a) : (b)
#define MAX(a, b) (a) > (b) ? (a) : (b)

int main(int, char**)
{
	i64 malloc_average = 0;
	i64 linear_average = 0;
	i64 stack_average  = 0;

	// NOTE(jesper): we run each benchmark a thousand times and use the average
	// for our final result to rule out outliers from the system scheduler and
	// caching
	for (i32 i = 0; i < 1000; i++) {
		i64 malloc_duration = benchmark_malloc();
		i64 linear_duration = benchmark_linear_allocator();
		i64 stack_duration  = benchmark_stack_allocator();

		malloc_average = (malloc_average + malloc_duration) / 2;
		linear_average = (linear_average + linear_duration) / 2;
		stack_average = (stack_average + stack_duration) / 2;
	}

	DEBUG_LOG("malloc finished in %f ms", (f32)malloc_average / 1000000.0f);
	DEBUG_LOG("linear finished in %f ms", (f32)linear_average / 1000000.0f);
	DEBUG_LOG("stack  finished in %f ms", (f32)stack_average / 1000000.0f);
	return 0;
}
