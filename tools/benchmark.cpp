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

#define NUM_16B  (50000)
#define NUM_256B (5000)
#define NUM_8K   (500)
#define NUM_2MB  (50)

i64 benchmark_stack(isize alloc_size, i32 count)
{
	isize size = alloc_size * 2;
	void *buffer = malloc(size);

	StackAllocator stack = make_stack_allocator(buffer, size);

	timespec start = get_time();

	for (i32 i = 0; i < count; i++) {
		void *tmp = alloc(&stack, alloc_size);
		dealloc(&stack, tmp);
	}

	timespec end = get_time();
	free(buffer);

	return get_time_difference(start, end);
}

i64 benchmark_linear(isize alloc_size, i32 count)
{
	isize size = alloc_size * count * 2;
	void *buffer = malloc(size);

	LinearAllocator a = make_linear_allocator(buffer, size);

	timespec start = get_time();

	for (i32 i = 0; i < count; i++) {
		void *tmp = alloc(&a, alloc_size);
		VAR_UNUSED(tmp);
	}

	reset(&a);

	timespec end = get_time();
	free(buffer);

	return get_time_difference(start, end);
}

i64 benchmark_malloc(isize alloc_size, i32 count)
{
	timespec start = get_time();

	for (i32 i = 0; i < count; i++) {
		void *tmp = malloc(alloc_size);
		free(tmp);
	}

	timespec end = get_time();
	return get_time_difference(start, end);
}

int main(int, char**)
{
	i64 malloc_average_16B  = 0;
	i64 malloc_average_256B = 0;
	i64 malloc_average_8K   = 0;
	i64 malloc_average_2MB  = 0;

	i64 linear_average_16B  = 0;
	i64 linear_average_256B = 0;
	i64 linear_average_8K   = 0;
	i64 linear_average_2MB  = 0;

	i64 stack_average_16B  = 0;
	i64 stack_average_256B = 0;
	i64 stack_average_8K   = 0;
	i64 stack_average_2MB  = 0;

	// NOTE(jesper): we run each benchmark a thousand times and use the average
	// for our final result to rule out outliers from the system scheduler and
	// caching
	for (i32 i = 0; i < 1000; i++) {
		i64 malloc_duration_16B  = benchmark_malloc(16, NUM_16B);
		i64 malloc_duration_256B = benchmark_malloc(256, NUM_256B);
		i64 malloc_duration_8K   = benchmark_malloc(256, NUM_8K);
		i64 malloc_duration_2MB  = benchmark_malloc(2 * 1024 * 1024, NUM_2MB);

		i64 linear_duration_16B  = benchmark_linear(16, NUM_16B);
		i64 linear_duration_256B = benchmark_linear(256, NUM_256B);
		i64 linear_duration_8K   = benchmark_linear(256, NUM_8K);
		i64 linear_duration_2MB  = benchmark_linear(2 * 1024 * 1024, NUM_2MB);

		i64 stack_duration_16B  = benchmark_stack(16, NUM_16B);
		i64 stack_duration_256B = benchmark_stack(256, NUM_256B);
		i64 stack_duration_8K   = benchmark_stack(8 * 1024, NUM_8K);
		i64 stack_duration_2MB  = benchmark_stack(2 * 1024 * 1024, NUM_2MB);


		malloc_average_16B  = (malloc_average_16B + malloc_duration_16B) / 2;
		malloc_average_256B = (malloc_average_256B + malloc_duration_256B) / 2;
		malloc_average_8K   = (malloc_average_8K + malloc_duration_8K) / 2;
		malloc_average_2MB  = (malloc_average_2MB + malloc_duration_2MB) / 2;

		linear_average_16B  = (linear_average_16B + linear_duration_16B) / 2;
		linear_average_256B = (linear_average_256B + linear_duration_256B) / 2;
		linear_average_8K   = (linear_average_8K + linear_duration_8K) / 2;
		linear_average_2MB  = (linear_average_2MB + linear_duration_2MB) / 2;

		stack_average_16B   = (stack_average_16B + stack_duration_16B) / 2;
		stack_average_256B  = (stack_average_256B + stack_duration_256B) / 2;
		stack_average_8K    = (stack_average_8K + stack_duration_8K) / 2;
		stack_average_2MB   = (stack_average_2MB + stack_duration_2MB) / 2;
	}

	f32 malloc_average_16B_um  = (f32)malloc_average_16B / 1000.0f;
	f32 malloc_average_256B_um = (f32)malloc_average_256B / 1000.0f;
	f32 malloc_average_8K_um   = (f32)malloc_average_8K / 1000.0f;
	f32 malloc_average_2MB_um  = (f32)malloc_average_2MB / 1000.0f;

	f32 linear_average_16B_um  = (f32)linear_average_16B / 1000.0f;
	f32 linear_average_256B_um = (f32)linear_average_256B / 1000.0f;
	f32 linear_average_8K_um   = (f32)linear_average_8K / 1000.0f;
	f32 linear_average_2MB_um  = (f32)linear_average_2MB / 1000.0f;

	f32 stack_average_16B_um  = (f32)stack_average_16B / 1000.0f;
	f32 stack_average_256B_um = (f32)stack_average_256B / 1000.0f;
	f32 stack_average_8K_um   = (f32)stack_average_8K / 1000.0f;
	f32 stack_average_2MB_um  = (f32)stack_average_2MB / 1000.0f;

	DEBUG_LOG("system malloc:");
	DEBUG_LOG("- 16B  [%d]:\t%f um \t(%f per alloc)", NUM_16B,
	          malloc_average_16B_um, malloc_average_16B_um / NUM_16B);

	DEBUG_LOG("- 256B [%d]: \t%f um \t(%f per alloc)", NUM_256B,
	          malloc_average_256B_um, malloc_average_256B_um / NUM_256B);

	DEBUG_LOG("- 8K   [%d]: \t%f um \t(%f per alloc)", NUM_8K,
	          malloc_average_8K_um, malloc_average_8K_um / NUM_8K);

	DEBUG_LOG("- 2MB  [%d]: \t%f um \t(%f per alloc)", NUM_2MB,
	          malloc_average_2MB_um, malloc_average_2MB_um / NUM_2MB);

	DEBUG_LOG("");

	DEBUG_LOG("custom linear:");
	DEBUG_LOG("- 16B  [%d]:\t%f um \t(%f per alloc)", NUM_16B,
	          linear_average_16B_um, linear_average_16B_um / NUM_16B);

	DEBUG_LOG("- 256B [%d]: \t%f um \t(%f per alloc)", NUM_256B,
	          linear_average_256B_um, linear_average_256B_um / NUM_256B);

	DEBUG_LOG("- 8K   [%d]: \t%f um \t(%f per alloc)", NUM_8K,
	          linear_average_8K_um, linear_average_8K_um / NUM_8K);

	DEBUG_LOG("- 2MB  [%d]: \t%f um \t(%f per alloc)", NUM_2MB,
	          linear_average_2MB_um, linear_average_2MB_um / NUM_2MB);

	DEBUG_LOG("");

	DEBUG_LOG("custom stack:");
	DEBUG_LOG("- 16B  [%d]:\t%f um \t(%f per alloc)", NUM_16B,
	          stack_average_16B_um, stack_average_16B_um / NUM_16B);

	DEBUG_LOG("- 256B [%d]: \t%f um \t(%f per alloc)", NUM_256B,
	          stack_average_256B_um, stack_average_256B_um / NUM_256B);

	DEBUG_LOG("- 8K   [%d]: \t%f um \t(%f per alloc)", NUM_8K,
	          stack_average_8K_um, stack_average_8K_um / NUM_8K);

	DEBUG_LOG("- 2MB  [%d]: \t%f um \t(%f per alloc)", NUM_2MB,
	          stack_average_2MB_um, stack_average_2MB_um / NUM_2MB);
	return 0;
}


