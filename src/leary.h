/**
 * file:    leary.h
 * created: 2017-03-18
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#ifndef LEARY_H
#define LEARY_H

#ifndef INTROSPECT
#define INTROSPECT
#endif

#include <math.h>

#include "external/stb/stb_truetype.h"

#include "core/types.h"

#define PI 3.1415942f

#define MIN(a, b) (a) < (b) ? (a) : (b)
#define MAX(a, b) (a) > (b) ? (a) : (b)

struct LinearAllocator {
	void *start;
	void *current;
	void *last;
	isize size;
};

struct StackAllocator {
	void *start;
	void *current;
	isize size;
};

struct GameMemory {
	void *game;
	LinearAllocator frame;
	LinearAllocator persistent;
};

template<typename T, typename A>
struct Array {
	T* data;
	isize count;
	isize capacity;

	A* allocator;

	T& operator[] (isize i)
	{
		return data[i];
	}
};

template<typename T>
struct StaticArray {
	T* data;
	isize count;
	isize capacity;

	T& operator[] (isize i)
	{
		return data[i];
	}
};

#include "render/vulkan_device.h"

INTROSPECT struct Resolution
{
	i32 width  = 1280;
	i32 height = 720;
};

INTROSPECT struct VideoSettings
{
	Resolution resolution;

	// NOTE: these are integers to later support different fullscreen and vsync techniques
	i16 fullscreen = 0;
	i16 vsync      = 1;
};

INTROSPECT struct Settings
{
	VideoSettings video;
};



INTROSPECT struct Vector2 {
	f32 x, y;
};

INTROSPECT struct Vector3 {
	f32 x, y, z;
};

INTROSPECT struct Vector4 {
	f32 x, y, z, w;
};


struct Matrix4 {
	Vector4 columns[4];

	inline Vector4& operator[] (i32 i)
	{
		return columns[i];
	}

	static inline Matrix4 identity()
	{
		Matrix4 identity = {};
		identity[0].x = 1.0f;
		identity[1].y = 1.0f;
		identity[2].z = 1.0f;
		identity[3].w = 1.0f;
		return identity;
	}

	static inline Matrix4 orthographic(f32 left, f32 right,
	                                    f32 top, f32 bottom,
	                                    f32 near, f32 far)
	{
		Matrix4 result = Matrix4::identity();
		result[0].x = 2.0f / (right - left);
		result[1].y = 2.0f / (top - bottom);
		result[2].z = - 2.0f / (far - near);
		result[3].x = - (right + left) / (right - left);
		result[3].y = - (top + bottom ) / (top - bottom);
		result[3].z = (far + near) / (far - near);
		return result;
	}

	static inline Matrix4 perspective(f32 vfov, f32 aspect, f32 near, f32 far)
	{
		Matrix4 result = {};

		f32 tan_hvfov = tanf(vfov / 2.0f);
		result[0].x = 1.0f / (aspect * tan_hvfov);
		result[1].y = 1.0f / (tan_hvfov);
		result[2].w = -1.0f;
		result[2].z = far / (near - far);
		result[3].z = -(far * near) / (far - near);
		return result;
	}
};

struct Camera {
	Matrix4             view;
	Matrix4             projection;
	VulkanUniformBuffer ubo;

	Vector3             position;
	f32                 yaw, pitch, roll;
};

struct RenderedText {
	VulkanBuffer buffer;
	i32          vertex_count;
};

struct IndexRenderObject {
	VulkanPipeline pipeline;
	VulkanBuffer   vertices;
	VulkanBuffer   indices;
	i32            index_count;
	Matrix4        transform;
};

// TODO(jesper): stick these in the pipeline so that we reduce the number of
// pipeline binds
struct RenderObject {
	VulkanPipeline pipeline;
	VulkanBuffer   vertices;
	i32            vertex_count;
	Matrix4        transform;
};

struct GameState {
	VulkanDevice        vulkan;

	struct {
		VulkanPipeline font;
		VulkanPipeline mesh;
		VulkanPipeline terrain;
	} pipelines;

	struct {
		VulkanTexture font;
	} textures;

	Camera fp_camera;
	Camera ui_camera;

	Array<RenderObject, LinearAllocator> render_objects;
	Array<IndexRenderObject, LinearAllocator> index_render_objects;

	VkCommandBuffer     *command_buffers;

	Vector3 velocity = {};

	stbtt_bakedchar     baked_font[256];

	char                *text_buffer;
	RenderedText        text_vertices;

	i32 *key_state;
};

#define GAME_FUNCS(M)\
	M(void, init, Settings*, PlatformState*, GameMemory*);\
	M(void, load_platform_code, PlatformCode*);\
	M(void, load_settings, Settings*);\
	M(void, quit, GameMemory*, PlatformState*, Settings*);\
\
	M(void, input, GameMemory*, PlatformState*, Settings*, InputEvent);\
	M(void, update_and_render, GameMemory*, f32)

#define GAME_TYPEDEF_FUNC(ret, name, ...) typedef ret game_##name##_t (__VA_ARGS__)
#define GAME_DCL_FPTR(ret, name, ...) game_##name##_t *name
#define GAME_DCL_STATIC_FPTR(ret, name, ...) static game_##name##_t *game_##name

#define MISC_FUNCS(M)\
	M(LinearAllocator, make_linear_allocator, void*, isize);\
	M(StackAllocator, make_stack_allocator, void*, isize);\
\
	M(void, profile_init, GameMemory*);\
	M(void, profile_start_frame, void);\
	M(void, profile_end_frame, void);\
	M(i32, profile_start_timer, const char*);\
	M(void, profile_end_timer, i32, u64)

#define MISC_TYPEDEF_FUNC(ret, name, ...) typedef ret name##_t (__VA_ARGS__)
#define MISC_DCL_FPTR(ret, name, ...) name##_t *name
#define MISC_DCL_STATIC_FPTR(ret, name, ...) static name##_t *name

#endif /* LEARY_H */

