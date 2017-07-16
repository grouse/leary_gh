/**
 * file:    leary.cpp
 * created: 2016-11-26
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2016 - all rights reserved
 */

#include "leary.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "external/stb/stb_truetype.h"

#include "platform/platform.h"
#include "platform/platform_debug.h"
#include "platform/platform_file.h"
#include "platform/platform_input.h"

#include "core/allocator.cpp"
#include "core/array.cpp"
#include "core/tokenizer.cpp"
#include "core/profiling.cpp"
#include "core/math.cpp"
#include "core/mesh.cpp"
#include "core/random.cpp"
#include "core/assets.cpp"

#include "render/vulkan_device.cpp"

#include "core/serialize.cpp"

struct Entity {
	i32 id;
	i32 index;
};

struct RenderedText {
	VulkanBuffer buffer;
	i32          vertex_count;
};

struct IndexRenderObject {
	i32            entity_id;
	VulkanPipeline pipeline;
	VulkanBuffer   vertices;
	VulkanBuffer   indices;
	i32            index_count;
	//Matrix4        transform;
	Material       *material;
};

// TODO(jesper): stick these in the pipeline so that we reduce the number of
// pipeline binds
struct RenderObject {
	VulkanPipeline pipeline;
	VulkanBuffer   vertices;
	i32            vertex_count;
	Matrix4        transform;
	Material       *material;
};

struct Camera {
	Matrix4             view;
	Matrix4             projection;
	VulkanUniformBuffer ubo;

	Vector3             position;
	f32                 yaw, pitch, roll;
};

struct Physics {
	ARRAY(Vector3) positions;
	ARRAY(Vector3) velocities;

	ARRAY(i32) entities;
};

struct DebugOverlay {
	char            *buffer;
	RenderedText    text;
	stbtt_bakedchar font[256];

	bool profile_timers = true;
	bool allocators = true;
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
		VulkanTexture cube;
		VulkanTexture player;
	} textures;

	struct {
		Material font;
		Material phong;
		Material player;
	} materials;

	DebugOverlay overlay;

	ARRAY(Entity) entities;
	Physics physics;

	Camera fp_camera;
	Camera ui_camera;

	ARRAY(RenderObject) render_objects;
	ARRAY(IndexRenderObject) index_render_objects;

	VkCommandBuffer     *command_buffers;

	Vector3 velocity = {};

	i32 *key_state;
};

Entity entities_add(ARRAY(Entity) *entities)
{
	Entity e = {};
	e.id = (i32)entities->count;

	i32 i = (i32)array_add(entities, e);
	(*entities)[i].index = i;

	return e;
}

ARRAY(Entity) entities_create(GameMemory *memory)
{
	return ARRAY_CREATE(Entity, &memory->free_list);
}

Physics physics_create(GameMemory *memory)
{
	Physics p = {};

	p.positions     = ARRAY_CREATE(Vector3, &memory->free_list);
	p.velocities    = ARRAY_CREATE(Vector3, &memory->free_list);
	p.entities      = ARRAY_CREATE(i32, &memory->free_list);

	return p;
}

void physics_process(Physics *physics, f32 dt)
{
	for (i32 i = 0; i < physics->positions.count; i++) {
		physics->positions[i]  += physics->velocities[i] * dt;
	}
}

i32 physics_add(Physics *physics, Entity entity)
{
	array_add(&physics->positions,     {});
	array_add(&physics->velocities,    {});

	i32 id = (i32)array_add(&physics->entities, entity.id);
	return id;
}

void physics_remove(Physics *physics, i32 id)
{
	array_remove(&physics->velocities,    id);
	array_remove(&physics->positions,     id);
	array_remove(&physics->entities,      id);
}

i32 physics_id(Physics *physics, i32 entity_id)
{
	// TODO(jesper): make a more performant look-up
	i32 id = -1;
	for (i32 i = 0; i < physics->entities.count; i++) {
		if (physics->entities[i] == entity_id) {
			id = i;
			break;
		}
	}

	DEBUG_ASSERT(id != -1);
	return id;
}

Vector3 physics_position(Physics *physics, i32 i)
{
	DEBUG_ASSERT(i < physics->positions.count);
	return physics->positions[i];
}

void render_font(GameMemory *memory,
                 stbtt_bakedchar *font,
                 const char *str,
                 float x, float y,
                 RenderedText *text)
{
	GameState *game = (GameState*)memory->game;
	i32 offset = 0;

	usize text_length = strlen(str);
	if (text_length == 0) return;

	usize vertices_size = sizeof(f32)*30*text_length;
	auto vertices = (f32*)alloc(&memory->frame, vertices_size);

	text->vertex_count = 0;

	Matrix4 camera = translate(game->ui_camera.view, {x, y, 0.0f});

	float tmp_x = 0.0f, tmp_y = 0.0f;

	while (*str) {
		char c = *str++;
		if (c == '\n') {
			tmp_y += 20.0f;
			tmp_x  = 0.0f;
			continue;
		}

		text->vertex_count += 6;

		stbtt_aligned_quad q = {};
		stbtt_GetBakedQuad(font, 1024, 1024, c, &tmp_x, &tmp_y, &q, 1);

		Vector3 tl = camera * Vector3{q.x0, q.y0 + 15.0f, 0.0f};
		Vector3 tr = camera * Vector3{q.x1, q.y0 + 15.0f, 0.0f};
		Vector3 br = camera * Vector3{q.x1, q.y1 + 15.0f, 0.0f};
		Vector3 bl = camera * Vector3{q.x0, q.y1 + 15.0f, 0.0f};

		vertices[offset++] = tl.x;
		vertices[offset++] = tl.y;
		vertices[offset++] = 0.2f;
		vertices[offset++] = q.s0;
		vertices[offset++] = q.t0;

		vertices[offset++] = tr.x;
		vertices[offset++] = tr.y;
		vertices[offset++] = 0.2f;
		vertices[offset++] = q.s1;
		vertices[offset++] = q.t0;

		vertices[offset++] = br.x;
		vertices[offset++] = br.y;
		vertices[offset++] = 0.2f;
		vertices[offset++] = q.s1;
		vertices[offset++] = q.t1;

		vertices[offset++] = br.x;
		vertices[offset++] = br.y;
		vertices[offset++] = 0.2f;
		vertices[offset++] = q.s1;
		vertices[offset++] = q.t1;

		vertices[offset++] = bl.x;
		vertices[offset++] = bl.y;
		vertices[offset++] = 0.2f;
		vertices[offset++] = q.s0;
		vertices[offset++] = q.t1;

		vertices[offset++] = tl.x;
		vertices[offset++] = tl.y;
		vertices[offset++] = 0.2f;
		vertices[offset++] = q.s0;
		vertices[offset++] = q.t0;
	}

	void *mapped;
	vkMapMemory(game->vulkan.handle, text->buffer.memory,
	            0, VK_WHOLE_SIZE,
	            0, &mapped);

	memcpy(mapped, vertices, vertices_size);
	vkUnmapMemory(game->vulkan.handle, text->buffer.memory);
}

void game_init(GameMemory *memory, PlatformState *platform)
{
	GameState *game = ialloc<GameState>(&memory->persistent);
	memory->game = game;

	game->overlay.buffer = (char*)alloc(&memory->persistent, 1024 * 1024);

	f32 width = (f32)platform->settings.video.resolution.width;
	f32 height = (f32)platform->settings.video.resolution.height;
	f32 aspect = width / height;
	f32 vfov   = radians(45.0f);

	game->fp_camera.view = Matrix4::identity();
	game->fp_camera.position = Vector3{0.0f, 5.0f, 0.0f};
	game->fp_camera.yaw = -0.5f * PI;
	game->fp_camera.projection = Matrix4::perspective(vfov, aspect, 0.1f, 100.0f);

	Matrix4 view = Matrix4::identity();
	view[0].x = 2.0f / width;
	view[1].y = 2.0f / height;
	view[2].z = 1.0f;
	game->ui_camera.view = view;

	game->render_objects = ARRAY_CREATE(RenderObject, &memory->persistent, 20);
	game->index_render_objects = ARRAY_CREATE(IndexRenderObject, &memory->persistent, 20);

	VkResult result;
	game->vulkan = device_create(memory, platform, &platform->settings);

	game->command_buffers = alloc_array<VkCommandBuffer>(&memory->persistent, 5);
	VkCommandBufferAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate_info.commandPool        = game->vulkan.command_pool;
	allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocate_info.commandBufferCount = 1;

	result = vkAllocateCommandBuffers(game->vulkan.handle,
	                                  &allocate_info,
	                                  game->command_buffers);
	DEBUG_ASSERT(result == VK_SUCCESS);

	// create font atlas
	{
		usize font_size;
		char *font_path = platform_resolve_path(GamePath_data,
		                                        "fonts/Roboto-Regular.ttf");
		u8 *font_data = (u8*)platform_file_read(font_path, &font_size);

		u8 *bitmap = alloc_array<u8>(&memory->frame, 1024*1024);
		stbtt_BakeFontBitmap(font_data, 0, 20.0, bitmap, 1024, 1024, 0,
		                     256, game->overlay.font);

		VkComponentMapping components = {};
		components.a = VK_COMPONENT_SWIZZLE_R;
		game->textures.font = texture_create(&game->vulkan, 1024, 1024,
		                                    VK_FORMAT_R8_UNORM, bitmap,
		                                    components);

		game->overlay.text.vertex_count = 0;
		game->overlay.text.buffer = buffer_create_vbo(&game->vulkan, 1024*1024);
	}

	{
		Texture dummy = texture_load_bmp("dummy.bmp");

		// TODO(jesper): texture file loading
		game->textures.cube = texture_create(&game->vulkan,
		                                     dummy.width,
		                                     dummy.height,
		                                     dummy.format,
		                                     dummy.data,
		                                     VkComponentMapping{});
	}

	{
		Texture player = texture_load_bmp("player.bmp");

		// TODO(jesper): texture file loading
		game->textures.player = texture_create(&game->vulkan,
		                                       player.width, player.height,
		                                       player.format, player.data,
		                                       VkComponentMapping{});
	}


	// create pipelines
	{
		game->pipelines.mesh    = pipeline_create_mesh(&game->vulkan, memory);
		game->pipelines.font    = pipeline_create_font(&game->vulkan, memory);
		game->pipelines.terrain = pipeline_create_terrain(&game->vulkan, memory);
	}

	// create ubos
	{
		game->fp_camera.ubo = buffer_create_ubo(&game->vulkan, sizeof(Matrix4));

		Matrix4 view_projection = game->fp_camera.projection * game->fp_camera.view;
		buffer_data_ubo(&game->vulkan, game->fp_camera.ubo,
		                &view_projection, 0, sizeof(view_projection));
	}

	// create materials
	{
		game->materials.font = material_create(&game->vulkan, memory,
		                                       &game->pipelines.font,
		                                       Material_font);

		game->materials.phong = material_create(&game->vulkan, memory,
		                                        &game->pipelines.mesh,
		                                        Material_phong);

		game->materials.player = material_create(&game->vulkan, memory,
		                                         &game->pipelines.mesh,
		                                         Material_phong);
	}

	// update descriptor sets
	{
		pipeline_set_ubo(&game->vulkan, &game->pipelines.mesh,
		                 ResourceSlot_mvp, &game->fp_camera.ubo);

		pipeline_set_ubo(&game->vulkan, &game->pipelines.terrain,
		                 ResourceSlot_mvp, &game->fp_camera.ubo);


		material_set_texture(&game->vulkan, &game->materials.font,
		                     ResourceSlot_font_atlas, &game->textures.font);

		material_set_texture(&game->vulkan, &game->materials.phong,
		                     ResourceSlot_texture, &game->textures.cube);

		material_set_texture(&game->vulkan, &game->materials.player,
		                     ResourceSlot_texture, &game->textures.player);
	}

	{
		game->entities = entities_create(memory);
		game->physics  = physics_create(memory);
	}

	Random r = random_create(3);
	Mesh cube = load_mesh_obj(memory, "cube.obj");

	{
		Entity player = entities_add(&game->entities);
		i32 pid = physics_add(&game->physics, player);

		f32 x = next_f32(&r) * 20.0f;
		f32 y = -1.0f;
		f32 z = next_f32(&r) * 20.0f;

		game->physics.positions[pid] = { x, y, z };

		IndexRenderObject obj = {};
		obj.material = &game->materials.player;

		usize vertex_size = cube.vertices.count * sizeof(cube.vertices[0]);
		usize index_size  = cube.indices.count  * sizeof(cube.indices[0]);

		obj.entity_id   = player.id;
		obj.pipeline    = game->pipelines.mesh;
		obj.index_count = (i32)cube.indices.count;
		obj.vertices    = buffer_create_vbo(&game->vulkan, cube.vertices.data, vertex_size);
		obj.indices     = buffer_create_ibo(&game->vulkan, cube.indices.data, index_size);

		//obj.transform = translate(Matrix4::identity(), {x, y, z});
		array_add(&game->index_render_objects, obj);
	}

	for (i32 i = 0; i < 10; i++) {
		Entity e = entities_add(&game->entities);
		i32 pid  = physics_add(&game->physics, e);

		f32 x = next_f32(&r) * 20.0f;
		f32 y = -1.0f;
		f32 z = next_f32(&r) * 20.0f;

		game->physics.positions[pid] = { x, y, z };

		IndexRenderObject obj = {};
		obj.material = &game->materials.phong;

		usize vertex_size = cube.vertices.count * sizeof(cube.vertices[0]);
		usize index_size  = cube.indices.count  * sizeof(cube.indices[0]);

		obj.entity_id   = e.id;
		obj.pipeline    = game->pipelines.mesh;
		obj.index_count = (i32)cube.indices.count;
		obj.vertices    = buffer_create_vbo(&game->vulkan, cube.vertices.data, vertex_size);
		obj.indices     = buffer_create_ibo(&game->vulkan, cube.indices.data, index_size);

		//obj.transform = translate(Matrix4::identity(), {x, y, z});
		array_add(&game->index_render_objects, obj);
	}

	{
		f32 vertices[] = {
			-100.0f, 0.0f, -100.0f,
			100.0f,  0.0f, -100.0f,
			100.0f,  0.0f, 100.0f,

			100.0f,  0.0f, 100.0f,
			-100.0f, 0.0f, 100.0f,
			-100.0f, 0.0f, -100.0f
		};

		RenderObject terrain = {};
		terrain.pipeline = game->pipelines.terrain;
		terrain.vertices = buffer_create_vbo(&game->vulkan, vertices, sizeof(vertices));
		terrain.vertex_count = sizeof(vertices) / (sizeof(vertices[0]) * 3);

		array_add(&game->render_objects, terrain);
	}

	game->key_state = alloc_array<i32>(&memory->persistent, 0xFF);
	for (i32 i = 0; i < 0xFF; i++) {
		game->key_state[i] = InputType_key_release;
	}
}

void game_quit(GameMemory *memory, PlatformState *platform)
{
	GameState *game = (GameState*)memory->game;
	// NOTE(jesper): disable raw mouse as soon as possible to ungrab the cursor
	// on Linux
	platform_set_raw_mouse(platform, false);

	vkQueueWaitIdle(game->vulkan.queue);

	buffer_destroy(&game->vulkan, game->overlay.text.buffer);

	texture_destroy(&game->vulkan, game->textures.font);
	texture_destroy(&game->vulkan, game->textures.cube);
	texture_destroy(&game->vulkan, game->textures.player);

	material_destroy(&game->vulkan, &game->materials.font);
	material_destroy(&game->vulkan, &game->materials.phong);
	material_destroy(&game->vulkan, &game->materials.player);

	pipeline_destroy(&game->vulkan, game->pipelines.font);
	pipeline_destroy(&game->vulkan, game->pipelines.mesh);
	pipeline_destroy(&game->vulkan, game->pipelines.terrain);

	for (i32 i = 0; i < game->render_objects.count; i++) {
		buffer_destroy(&game->vulkan, game->render_objects[i].vertices);
	}

	for (i32 i = 0; i < game->index_render_objects.count; i++) {
		buffer_destroy(&game->vulkan, game->index_render_objects[i].vertices);
		buffer_destroy(&game->vulkan, game->index_render_objects[i].indices);
	}

	buffer_destroy_ubo(&game->vulkan, game->fp_camera.ubo);
	vulkan_destroy(&game->vulkan);

	platform_quit(platform);
}

void game_pre_reload(GameMemory *memory)
{
	GameState *game = (GameState*)memory->game;
	// NOTE(jesper): wait for the vulkan queues to be idle. Here for when I get
	// to shader and resource reloading - I don't even want to think about what
	// kind of fits graphics drivers will throw if we start recreating pipelines
	// in the middle of things
	vkQueueWaitIdle(game->vulkan.queue);
}

void game_reload(GameMemory *memory)
{
	GameState *game = (GameState*)memory->game;
	vulkan_set_code(&game->vulkan);
	// TODO(jesper): reload the shaders and resources if changed
}

void game_input(GameMemory *memory, PlatformState *platform, InputEvent event)
{
	GameState *game = (GameState*)memory->game;
	switch (event.type) {
	case InputType_key_press: {
		if (event.key.repeated) {
			break;
		}

		game->key_state[event.key.vkey] = InputType_key_press;

		switch (event.key.vkey) {
		case VirtualKey_escape:
			game_quit(memory, platform);
			break;
		case VirtualKey_W:
			// TODO(jesper): tweak movement speed when we have a sense of scale
			game->velocity.z = 3.0f;
			break;
		case VirtualKey_S:
			// TODO(jesper): tweak movement speed when we have a sense of scale
			game->velocity.z = -3.0f;
			break;
		case VirtualKey_A:
			// TODO(jesper): tweak movement speed when we have a sense of scale
			game->velocity.x = -3.0f;
			break;
		case VirtualKey_D:
			// TODO(jesper): tweak movement speed when we have a sense of scale
			game->velocity.x = 3.0f;
			break;
		case VirtualKey_left: {
			i32 pid = physics_id(&game->physics, 0);
			game->physics.velocities[pid].x = 5.0f;
		} break;
		case VirtualKey_right: {
			i32 pid = physics_id(&game->physics, 0);
			game->physics.velocities[pid].x = -5.0f;
		} break;
		case VirtualKey_up: {
			i32 pid = physics_id(&game->physics, 0);
			game->physics.velocities[pid].z = 5.0f;
		} break;
		case VirtualKey_down: {
			i32 pid = physics_id(&game->physics, 0);
			game->physics.velocities[pid].z = -5.0f;
		} break;
		case VirtualKey_C:
			platform_toggle_raw_mouse(platform);
			break;
		default:
			//DEBUG_LOG("unhandled key press: %d", event.key.vkey);
			break;
		}
	} break;
	case InputType_key_release: {
		if (event.key.repeated) {
			break;
		}

		game->key_state[event.key.vkey] = InputType_key_release;

		switch (event.key.vkey) {
		case VirtualKey_W:
			game->velocity.z = 0.0f;
			if (game->key_state[VirtualKey_S] == InputType_key_press) {
				InputEvent e;
				e.type         = InputType_key_press;
				e.key.vkey     = VirtualKey_S;
				e.key.repeated = false;

				game_input(memory, platform, e);
			}
			break;
		case VirtualKey_S:
			game->velocity.z = 0.0f;
			if (game->key_state[VirtualKey_W] == InputType_key_press) {
				InputEvent e;
				e.type         = InputType_key_press;
				e.key.vkey     = VirtualKey_W;
				e.key.repeated = false;

				game_input(memory, platform, e);
			}
			break;
		case VirtualKey_A:
			game->velocity.x = 0.0f;
			if (game->key_state[VirtualKey_D] == InputType_key_press) {
				InputEvent e;
				e.type         = InputType_key_press;
				e.key.vkey     = VirtualKey_D;
				e.key.repeated = false;

				game_input(memory, platform, e);
			}
			break;
		case VirtualKey_D:
			game->velocity.x = 0.0f;
			if (game->key_state[VirtualKey_A] == InputType_key_press) {
				InputEvent e;
				e.type         = InputType_key_press;
				e.key.vkey     = VirtualKey_A;
				e.key.repeated = false;

				game_input(memory, platform, e);
			}
			break;
		case VirtualKey_left: {
			i32 pid = physics_id(&game->physics, 0);
			game->physics.velocities[pid].x = 0.0f;

			if (game->key_state[VirtualKey_right] == InputType_key_press) {
				InputEvent e;
				e.type         = InputType_key_press;
				e.key.vkey     = VirtualKey_right;
				e.key.repeated = false;

				game_input(memory, platform, e);
			}
		} break;
		case VirtualKey_right: {
			i32 pid = physics_id(&game->physics, 0);
			game->physics.velocities[pid].x = 0.0f;

			if (game->key_state[VirtualKey_left] == InputType_key_press) {
				InputEvent e;
				e.type         = InputType_key_press;
				e.key.vkey     = VirtualKey_left;
				e.key.repeated = false;

				game_input(memory, platform, e);
			}
		} break;
		case VirtualKey_up: {
			i32 pid = physics_id(&game->physics, 0);
			game->physics.velocities[pid].z = 0.0f;

			if (game->key_state[VirtualKey_down] == InputType_key_press) {
				InputEvent e;
				e.type         = InputType_key_press;
				e.key.vkey     = VirtualKey_down;
				e.key.repeated = false;

				game_input(memory, platform, e);
			}
		} break;
		case VirtualKey_down: {
			i32 pid = physics_id(&game->physics, 0);
			game->physics.velocities[pid].z = 0.0f;

			if (game->key_state[VirtualKey_up] == InputType_key_press) {
				InputEvent e;
				e.type         = InputType_key_press;
				e.key.vkey     = VirtualKey_up;
				e.key.repeated = false;

				game_input(memory, platform, e);
			}
		} break;
		default:
			//DEBUG_LOG("unhandled key release: %d", event.key.vkey);
			break;
		}
	} break;
	case InputType_mouse_move: {
		// TODO(jesper): move mouse sensitivity into settings
		game->fp_camera.yaw   += -0.001f * event.mouse.dx;
		game->fp_camera.pitch += -0.001f * event.mouse.dy;
	} break;
	default:
		//DEBUG_LOG("unhandled input type: %d", event.type);
		break;
	}
}

void debug_overlay_update(GameMemory *memory, DebugOverlay *overlay, f32 dt)
{
	PROFILE_FUNCTION();

	isize buffer_size = 1024*1024;
	char *buffer = overlay->buffer;
	buffer[0] = '\0';

	f32 dt_ms = dt * 1000.0f;
	i32 bytes = snprintf(buffer, buffer_size, "frametime: %f ms, %f fps\n",
	                     dt_ms, 1000.0f / dt_ms);
	buffer += bytes;
	buffer_size -= bytes;

	if (overlay->profile_timers) {
		bytes        = snprintf(buffer, buffer_size, "Profile Timers\n");
		buffer      += bytes;
		buffer_size -= bytes;

		for (i32 i = 0; i < g_profile_timers_prev->names.count; i++) {
			bytes = snprintf(buffer, buffer_size,
			                 "  %s: %" PRIu64 " cy (%" PRIu64 " cy)\n",
			                 g_profile_timers_prev->names[i],
			                 g_profile_timers_prev->cycles[i],
			                 g_profile_timers_prev->cycles_last[i]);
			buffer += bytes;
			buffer_size -= bytes;
		}
	} else {
		bytes        = snprintf(buffer, buffer_size, "Profile Timers...\n");
		buffer      += bytes;
		buffer_size -= bytes;
	}

	if (overlay->allocators) {
		bytes        = snprintf(buffer, buffer_size, "Allocators\n");
		buffer      += bytes;
		buffer_size -= bytes;

		bytes = snprintf(buffer, buffer_size,
		                 "  stack: { sp: %p, size: %ld, remaining: %ld }\n",
		                 memory->stack.stack.sp, memory->stack.size,
		                 memory->stack.remaining);
		buffer += bytes;
		buffer_size -= bytes;

		bytes = snprintf(buffer, buffer_size,
		                 "  frame: { current: %p, size: %ld, remaining: %ld }\n",
		                 memory->frame.linear.current, memory->frame.size,
		                 memory->frame.remaining);
		buffer += bytes;
		buffer_size -= bytes;

		bytes = snprintf(buffer, buffer_size,
		                 "  persistent: { current: %p, size: %ld, remaining: %ld }\n",
		                 memory->persistent.linear.current,
		                 memory->persistent.size,
		                 memory->persistent.remaining);
		buffer += bytes;
		buffer_size -= bytes;

		bytes = snprintf(buffer, buffer_size,
		                 "  free list: { size: %ld, remaining: %ld }\n",
		                 memory->free_list.size,
		                 memory->free_list.remaining);
		buffer += bytes;
		buffer_size -= bytes;
	} else {
		bytes        = snprintf(buffer, buffer_size, "Allocators...\n");
		buffer      += bytes;
		buffer_size -= bytes;
	}
}

void debug_overlay_render(DebugOverlay *overlay, GameMemory *memory)
{
	PROFILE_FUNCTION();

	render_font(memory,
	            overlay->font,
	            overlay->buffer, -1.0f, -1.0,
	            &overlay->text);
}

void game_update(GameMemory *memory, f32 dt)
{
	PROFILE_FUNCTION();
	GameState *game = (GameState*)memory->game;

	debug_overlay_update(memory, &game->overlay, dt);

	physics_process(&game->physics, dt);

	Matrix4 pitch = rotate_x(Matrix4::identity(), game->fp_camera.pitch);
	Matrix4 yaw   = rotate_y(Matrix4::identity(), game->fp_camera.yaw);
	Matrix4 roll  = rotate_z(Matrix4::identity(), game->fp_camera.roll);

	Matrix4 rotation = roll * pitch * yaw;

	Matrix4 view = game->fp_camera.view;

	Vector3 forward = { view[0].z, view[1].z, view[2].z };
	Vector3 strafe  = -Vector3{ view[0].x, view[1].x, view[2].x };

	game->fp_camera.position += dt * (game->velocity.z * forward +
	                                  game->velocity.x * strafe);
	Matrix4 translation = translate(Matrix4::identity(), game->fp_camera.position);

	game->fp_camera.view = rotation * translation;

	Matrix4 view_projection = game->fp_camera.projection * game->fp_camera.view;
	buffer_data_ubo(&game->vulkan, game->fp_camera.ubo,
	                &view_projection, 0, sizeof(view_projection));
}

void game_render(GameMemory *memory)
{
	PROFILE_FUNCTION();
	void *sp = memory->stack.stack.sp;
	defer { alloc_reset(&memory->stack, sp); };

	GameState *game = (GameState*)memory->game;

	debug_overlay_render(&game->overlay, memory);

	u32 image_index = swapchain_acquire(&game->vulkan);

	auto clear_values = alloc_array<VkClearValue>(&memory->stack, 2);
	clear_values[0].color        = { {1.0f, 0.0f, 0.0f, 0.0f} };
	clear_values[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo render_info = {};
	render_info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_info.renderPass        = game->vulkan.renderpass;
	render_info.framebuffer       = game->vulkan.framebuffers[image_index];
	render_info.renderArea.offset = { 0, 0 };
	render_info.renderArea.extent = game->vulkan.swapchain.extent;
	render_info.clearValueCount   = 2;
	render_info.pClearValues      = clear_values;

	VkDeviceSize offsets[] = { 0 };
	VkResult result;

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	VkCommandBuffer command = game->command_buffers[0];
	result = vkBeginCommandBuffer(command, &begin_info);
	DEBUG_ASSERT(result == VK_SUCCESS);

	vkCmdBeginRenderPass(command, &render_info, VK_SUBPASS_CONTENTS_INLINE);

	for (i32 i = 0; i < game->render_objects.count; i++) {
		RenderObject &object = game->render_objects[i];

		vkCmdBindPipeline(command,
		                  VK_PIPELINE_BIND_POINT_GRAPHICS,
		                  object.pipeline.handle);

		// TODO(jesper): bind material descriptor set if bound
		// TODO(jesper): only bind pipeline descriptor set if one exists, might
		// be such a special case that we should hardcode it?
		vkCmdBindDescriptorSets(command,
		                        VK_PIPELINE_BIND_POINT_GRAPHICS,
		                        object.pipeline.layout,
		                        0,
		                        1, &object.pipeline.descriptor_set,
		                        0, nullptr);

		vkCmdBindVertexBuffers(command, 0, 1, &object.vertices.handle, offsets);

		vkCmdPushConstants(command, object.pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT,
		                   0, sizeof(object.transform), &object.transform);

		vkCmdDraw(command, object.vertex_count, 1, 0, 0);
	}

	for (i32 i = 0; i < game->index_render_objects.count; i++) {
		IndexRenderObject &object = game->index_render_objects[i];

		vkCmdBindPipeline(command,
		                  VK_PIPELINE_BIND_POINT_GRAPHICS,
		                  object.pipeline.handle);

		// TODO(jesper): bind material descriptor set if bound
		// TODO(jesper): only bind pipeline descriptor set if one exists, might
		// be such a special case that we should hardcode it?
		auto descriptors = ARRAY_CREATE(VkDescriptorSet, &memory->stack);
		defer { array_destroy(&descriptors); };

		array_add(&descriptors, object.pipeline.descriptor_set);


		if (object.material) {
			array_add(&descriptors, object.material->descriptor_set);
		}

		vkCmdBindDescriptorSets(command,
		                        VK_PIPELINE_BIND_POINT_GRAPHICS,
		                        object.pipeline.layout,
		                        0,
		                        (i32)descriptors.count, descriptors.data,
		                        0, nullptr);

		vkCmdBindVertexBuffers(command, 0, 1, &object.vertices.handle, offsets);
		vkCmdBindIndexBuffer(command, object.indices.handle,
		                     0, VK_INDEX_TYPE_UINT32);

		i32 pid = physics_id(&game->physics, object.entity_id);
		Matrix4 transform = translate(Matrix4::identity(),
		                              game->physics.positions[pid]);


		vkCmdPushConstants(command, object.pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT,
		                   0, sizeof(transform), &transform);

		vkCmdDrawIndexed(command, object.index_count, 1, 0, 0, 0);
	}


	vkCmdBindPipeline(command,
	                  VK_PIPELINE_BIND_POINT_GRAPHICS,
	                  game->pipelines.font.handle);


	auto descriptors = ARRAY_CREATE(VkDescriptorSet, &memory->stack);
	array_add(&descriptors, game->materials.font.descriptor_set);

	// TODO(jesper): bind material descriptor set if bound
	// TODO(jesper): only bind pipeline descriptor set if one exists, might
	// be such a special case that we should hardcode it?
	vkCmdBindDescriptorSets(command,
	                        VK_PIPELINE_BIND_POINT_GRAPHICS,
	                        game->pipelines.font.layout,
	                        0,
	                        (i32)descriptors.count, descriptors.data,
	                        0, nullptr);


	if (game->overlay.text.vertex_count > 0) {
		vkCmdBindVertexBuffers(command, 0, 1,
		                       &game->overlay.text.buffer.handle, offsets);
		vkCmdDraw(command, game->overlay.text.vertex_count, 1, 0, 0);
	}


	vkCmdEndRenderPass(command);

	result = vkEndCommandBuffer(command);
	DEBUG_ASSERT(result == VK_SUCCESS);

	VkPipelineStageFlags wait_stages[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};

	VkSemaphore signal_semaphores[] = {
		game->vulkan.render_completed
	};

	VkSubmitInfo present_submit_info = {};
	present_submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	present_submit_info.waitSemaphoreCount   = 1;
	present_submit_info.pWaitSemaphores      = &game->vulkan.swapchain.available;
	present_submit_info.pWaitDstStageMask    = wait_stages;
	present_submit_info.commandBufferCount   = 1;
	present_submit_info.pCommandBuffers      = game->command_buffers;
	present_submit_info.signalSemaphoreCount = 1;
	present_submit_info.pSignalSemaphores    = signal_semaphores;

	result = vkQueueSubmit(game->vulkan.queue, 1, &present_submit_info, VK_NULL_HANDLE);
	DEBUG_ASSERT(result == VK_SUCCESS);


	VkSemaphore wait_semaphores[] = {
		game->vulkan.render_completed
	};

	VkSwapchainKHR swapchains[] = {
		game->vulkan.swapchain.handle
	};
	VkPresentInfoKHR present_info = {};
	present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores    = wait_semaphores; present_info.swapchainCount     = 1;
	present_info.pSwapchains        = swapchains;
	present_info.pImageIndices      = &image_index;

	result = vkQueuePresentKHR(game->vulkan.queue, &present_info);
	DEBUG_ASSERT(result == VK_SUCCESS);

	PROFILE_START(vulkan_swap);
	result = vkQueueWaitIdle(game->vulkan.queue);
	DEBUG_ASSERT(result == VK_SUCCESS);
	PROFILE_END(vulkan_swap);
}

void game_update_and_render(GameMemory *memory, f32 dt)
{
	game_update(memory, dt);
	game_render(memory);

	alloc_reset(&memory->frame);
}
