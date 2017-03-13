/**
 * file:    leary.cpp
 * created: 2016-11-26
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2016 - all rights reserved
 */

#define STB_TRUETYPE_IMPLEMENTATION
#define STB_RECT_PACK_IMPLEMENTATION
#include "external/stb/stb_rect_pack.h"
#include "external/stb/stb_truetype.h"

#include "core/allocator.cpp"

struct GameMemory {
	FrameAllocator      frame;
	PersistentAllocator persistent;
};

#include "core/array.cpp"

#include "core/settings.cpp"
#include "core/tokenizer.cpp"
#include "core/profiling.cpp"
#include "core/math.cpp"
#include "core/mesh.cpp"
#include "core/random.cpp"

#include "render/vulkan_device.h"

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
	VulkanBuffer vertices;
	VulkanBuffer indices;
	i32          vertex_count;
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

	GameMemory *memory;

	Camera fp_camera;
	Camera ui_camera;

	Array<RenderObject, PersistentAllocator> render_objects;

	VkCommandBuffer     *command_buffers;

	Vector3 velocity = {};

	stbtt_bakedchar     baked_font[256];

	char                *text_buffer;
	RenderedText        text_vertices;

	i32 *key_state;
};


#include "render/vulkan_device.cpp"

#include "core/serialize.cpp"



void render_font(GameState *game, RenderedText *text,
                 const char *str, float x, float y)
{
	i32 offset = 0;

	usize text_length = strlen(str);
	if (text_length == 0) return;

	usize vertices_size = sizeof(f32)*30*text_length;
	auto vertices = allocate<f32>(&game->memory->frame, vertices_size);

	text->vertex_count = (i32)(text_length * 6);

	Matrix4 camera = translate(game->ui_camera.view, {x, y, 0.0f});

	float tmp_x = 0.0f, tmp_y = 0.0f;

	while (*str) {
		char c = *str++;
		if (c == '\n') {
			tmp_y += 20.0f;
			tmp_x  = 0.0f;
			continue;
		}

		stbtt_aligned_quad q = {};
		stbtt_GetBakedQuad(game->baked_font, 1024, 1024, c, &tmp_x, &tmp_y, &q, 1);

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


void game_load_settings(Settings *settings)
{
	char *settings_path = platform_resolve_path(GamePath_preferences, "settings.conf");
	SERIALIZE_LOAD_CONF(settings_path, Settings, settings);
}

GameState game_init(Settings *settings, PlatformState *platform, GameMemory *memory)
{
	GameState game = {};
	game.memory = memory;

	game.text_buffer = allocate<char>(&game.memory->persistent, 1024 * 1024);

	f32 width = (f32)settings->video.resolution.width;
	f32 height = (f32)settings->video.resolution.height;
	f32 aspect = width / height;
	f32 vfov   = radians(45.0f);

	game.fp_camera.view = Matrix4::identity();
	game.fp_camera.position = Vector3{0.0f, 5.0f, 0.0f};
	game.fp_camera.yaw = -0.5f * PI;
	game.fp_camera.projection = Matrix4::perspective(vfov, aspect, 0.1f, 100.0f);

	Matrix4 view = Matrix4::identity();
	view[0].x = 2.0f / width;
	view[1].y = 2.0f / height;
	view[2].z = 1.0f;
	game.ui_camera.view = view;

	game.render_objects = make_array<RenderObject>(&game.memory->persistent, 20);

	VkResult result;
	game.vulkan = create_device(game.memory, platform, settings);

	game.command_buffers = allocate<VkCommandBuffer>(&game.memory->persistent, 5);
	VkCommandBufferAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate_info.commandPool        = game.vulkan.command_pool;
	allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocate_info.commandBufferCount = 1;

	result = vkAllocateCommandBuffers(game.vulkan.handle,
	                                  &allocate_info,
	                                  game.command_buffers);
	DEBUG_ASSERT(result == VK_SUCCESS);

	// create font atlas
	{
		usize font_size;
		char *font_path = platform_resolve_path(GamePath_data,
		                                        "fonts/Roboto-Regular.ttf");
		u8 *font_data = (u8*)platform_file_read(font_path, &font_size);

		u8 *bitmap = allocate<u8>(&game.memory->frame, 1024*1024);
		stbtt_BakeFontBitmap(font_data, 0, 20.0, bitmap, 1024, 1024, 0,
		                     256, game.baked_font);

		VkComponentMapping components = {};
		components.a = VK_COMPONENT_SWIZZLE_R;
		game.textures.font = create_texture(&game.vulkan, 1024, 1024,
		                                    VK_FORMAT_R8_UNORM, bitmap,
		                                    components);

		game.text_vertices.vertex_count = 0;
		game.text_vertices.buffer = create_vertex_buffer(&game.vulkan, 1024*1024);
	}

	// create pipelines
	{
		game.pipelines.mesh = create_mesh_pipeline(game.memory, &game.vulkan);
		game.pipelines.font = create_font_pipeline(game.memory, &game.vulkan);
		game.pipelines.terrain = create_terrain_pipeline(game.memory, &game.vulkan);
	}

	// create ubos
	{
		game.fp_camera.ubo = create_uniform_buffer(&game.vulkan, sizeof(Matrix4));

		Matrix4 view_projection = game.fp_camera.projection * game.fp_camera.view;
		update_uniform_data(&game.vulkan, game.fp_camera.ubo,
		                    &view_projection, 0, sizeof(view_projection));
	}

	// update descriptor sets
	{
		update_descriptor_sets(&game.vulkan,
		                       game.pipelines.mesh,
		                       game.fp_camera.ubo);

		update_descriptor_sets(&game.vulkan,
		                       game.pipelines.terrain,
		                       game.fp_camera.ubo);

		update_descriptor_sets(&game.vulkan,
		                       game.pipelines.font,
		                       game.textures.font);
	}

	Mesh cube = load_mesh_obj(game.memory, "cube.obj");

	Random r = make_random(3);
	for (i32 i = 0; i < 10; i++) {
		RenderObject obj = {};

		usize size = cube.vertices_count * sizeof(cube.vertices[0]);

		obj.pipeline = game.pipelines.mesh;
		obj.vertex_count = cube.vertices_count;
		obj.vertices = create_vertex_buffer(&game.vulkan, size, cube.vertices);

		f32 x = next_f32(&r) * 20.0f;
		f32 y = -1.0f;
		f32 z = next_f32(&r) * 20.0f;

		obj.transform = translate(Matrix4::identity(), {x, y, z});

		array_add(&game.render_objects, obj);
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
		terrain.pipeline = game.pipelines.terrain;
		terrain.vertices = create_vertex_buffer(&game.vulkan, sizeof(vertices), vertices);
		terrain.vertex_count = sizeof(vertices) / (sizeof(vertices[0]) * 3);

		array_add(&game.render_objects, terrain);
	}

	game.key_state = allocate<i32>(&game.memory->persistent, 0xFF);
	for (i32 i = 0; i < 0xFF; i++) {
		game.key_state[i] = InputType_key_release;
	}

	return game;
}

void game_quit(GameState *game, PlatformState *platform, Settings *settings)
{
	// NOTE(jesper): disable raw mouse as soon as possible to ungrab the cursor
	// on Linux
	platform_set_raw_mouse(platform, false);

	VAR_UNUSED(settings);
	vkQueueWaitIdle(game->vulkan.queue);

	destroy(&game->vulkan, game->text_vertices.buffer);

	destroy(&game->vulkan, game->textures.font);

	destroy(&game->vulkan, game->pipelines.font);
	destroy(&game->vulkan, game->pipelines.mesh);
	destroy(&game->vulkan, game->pipelines.terrain);

	for (i32 i = 0; i < game->render_objects.count; i++) {
		destroy(&game->vulkan, game->render_objects[i].vertices);
	}

	destroy(&game->vulkan, game->fp_camera.ubo);
	destroy(&game->vulkan);

	char *settings_path = platform_resolve_path(GamePath_preferences, "settings.conf");
	SERIALIZE_SAVE_CONF(settings_path, Settings, settings);
	platform_quit(platform);
}

void game_input(GameState *game, PlatformState *platform, Settings *settings,
                InputEvent event)
{
	switch (event.type) {
	case InputType_key_press: {
		if (event.key.repeated) {
			break;
		}

		game->key_state[event.key.vkey] = InputType_key_press;

		switch (event.key.vkey) {
		case VirtualKey_escape:
			game_quit(game, platform, settings);
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
		case VirtualKey_C:
			platform_toggle_raw_mouse(platform);
			break;
		default:
			DEBUG_LOG("unhandled key press: %d", event.key.vkey);
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

				game_input(game, platform, settings, e);
			}
			break;
		case VirtualKey_S:
			game->velocity.z = 0.0f;
			if (game->key_state[VirtualKey_W] == InputType_key_press) {
				InputEvent e;
				e.type         = InputType_key_press;
				e.key.vkey     = VirtualKey_W;
				e.key.repeated = false;

				game_input(game, platform, settings, e);
			}
			break;
		case VirtualKey_A:
			game->velocity.x = 0.0f;
			if (game->key_state[VirtualKey_D] == InputType_key_press) {
				InputEvent e;
				e.type         = InputType_key_press;
				e.key.vkey     = VirtualKey_D;
				e.key.repeated = false;

				game_input(game, platform, settings, e);
			}
			break;
		case VirtualKey_D:
			game->velocity.x = 0.0f;
			if (game->key_state[VirtualKey_A] == InputType_key_press) {
				InputEvent e;
				e.type         = InputType_key_press;
				e.key.vkey     = VirtualKey_A;
				e.key.repeated = false;

				game_input(game, platform, settings, e);
			}
			break;
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
		DEBUG_LOG("unhandled input type: %d", event.type);
		break;
	}
}

void game_update(GameState* game, f32 dt)
{
	PROFILE_FUNCTION();

	isize buffer_size = 1024*1024;
	char *buffer = game->text_buffer;
	buffer[0] = '\0';

	f32 dt_ms = dt * 1000.0f;
	i32 bytes = snprintf(buffer, buffer_size, "frametime: %f ms, %f fps\n",
	                     dt_ms, 1000.0f / dt_ms);
	buffer += bytes;
	buffer_size -= bytes;


	for (i32 i = 0; i < g_profile_timers_prev.names.count; i++) {
		bytes = snprintf(buffer, buffer_size, "%s: %" PRIu64 " cy (%" PRIu64 " cy)\n",
		                 g_profile_timers_prev.names[i],
		                 g_profile_timers_prev.cycles[i],
		                 g_profile_timers_prev.cycles_last[i]);
		buffer += bytes;
		buffer_size -= bytes;
	}

	render_font(game, &game->text_vertices, game->text_buffer, -1.0f, -1.0f);

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
	update_uniform_data(&game->vulkan, game->fp_camera.ubo,
	                    &view_projection, 0, sizeof(view_projection));
}

void game_render(GameState *game)
{
	PROFILE_FUNCTION();

	u32 image_index = acquire_swapchain_image(&game->vulkan);

	std::array<VkClearValue, 2> clear_values = {};
	clear_values[0].color        = { {1.0f, 0.0f, 0.0f, 0.0f} };
	clear_values[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo render_info = {};
	render_info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_info.renderPass        = game->vulkan.renderpass;
	render_info.framebuffer       = game->vulkan.framebuffers[image_index];
	render_info.renderArea.offset = { 0, 0 };
	render_info.renderArea.extent = game->vulkan.swapchain.extent;
	render_info.clearValueCount   = (u32)clear_values.size();
	render_info.pClearValues      = clear_values.data();

	VkDeviceSize offsets[] = { 0 };
	VkResult result;

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	VkCommandBuffer command = game->command_buffers[0];
	result = vkBeginCommandBuffer(command, &begin_info);
	DEBUG_ASSERT(result == VK_SUCCESS);

	vkCmdBeginRenderPass(command, &render_info, VK_SUBPASS_CONTENTS_INLINE);

#if 0
	vkCmdBindPipeline(command,
	                  VK_PIPELINE_BIND_POINT_GRAPHICS,
	                  game->pipelines.generic.handle);


	vkCmdBindDescriptorSets(command,
	                        VK_PIPELINE_BIND_POINT_GRAPHICS,
	                        game->pipelines.generic.layout,
	                        0,
	                        1, &game->pipelines.generic.descriptor_set,
	                        0, nullptr);

	vkCmdBindVertexBuffers(command, 0, 1, &game->object.vertices.handle, offsets);
	vkCmdBindIndexBuffer(command, game->object.indices.handle,
	                     0, VK_INDEX_TYPE_UINT32);

#if 0
	for (i32 i = 0; i < game->num_objects; i++) {
		vkCmdPushConstants(command,
		                   game->pipelines.generic.layout,
		                   VK_SHADER_STAGE_VERTEX_BIT,
		                   0, sizeof(Matrix4),
		                   &game->positions[i]);
		vkCmdDrawIndexed(command, game->object.vertex_count, 1, 0, 0, 0);
	}
#else
	vkCmdPushConstants(command, game->pipelines.generic.layout, VK_SHADER_STAGE_VERTEX_BIT,
	                   0, sizeof(Matrix4), &game->positions[0]);
	vkCmdDrawIndexed(command, game->object.vertex_count, 1, 0, 0, 0);
#endif
#endif

	for (i32 i = 0; i < game->render_objects.count; i++) {
		RenderObject &object = game->render_objects[i];

		vkCmdBindPipeline(command,
		                  VK_PIPELINE_BIND_POINT_GRAPHICS,
		                  object.pipeline.handle);

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


	vkCmdBindPipeline(command,
	                  VK_PIPELINE_BIND_POINT_GRAPHICS,
	                  game->pipelines.font.handle);


	vkCmdBindDescriptorSets(command,
	                        VK_PIPELINE_BIND_POINT_GRAPHICS,
	                        game->pipelines.font.layout,
	                        0,
	                        1, &game->pipelines.font.descriptor_set,
	                        0, nullptr);


	if (game->text_vertices.vertex_count > 0) {
		vkCmdBindVertexBuffers(command, 0, 1,
		                       &game->text_vertices.buffer.handle, offsets);
		vkCmdDraw(command, game->text_vertices.vertex_count, 1, 0, 0);
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

void game_update_and_render(GameState *game, f32 dt)
{
	game_update(game, dt);
	game_render(game);

	reset(&game->memory->frame);
}
