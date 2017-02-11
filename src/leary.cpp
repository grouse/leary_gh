/**
 * file:    leary.cpp
 * created: 2016-11-26
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2016 - all rights reserved
 */


#include "render/vulkan_device.cpp"

#include "core/settings.cpp"
#include "core/tokenizer.cpp"
#include "core/serialize.cpp"

#define STB_TRUETYPE_IMPLEMENTATION
#define STB_RECT_PACK_IMPLEMENTATION
#include "external/stb/stb_rect_pack.h"
#include "external/stb/stb_truetype.h"

enum InputAction {
	InputAction_move_vertical_start,
	InputAction_move_horizontal_start,
	InputAction_move_vertical_end,
	InputAction_move_horizontal_end,

	InputAction_move_player_vertical_start,
	InputAction_move_player_horizontal_start,
	InputAction_move_player_vertical_end,
	InputAction_move_player_horizontal_end
};

struct Camera {
	Matrix4f view;
	Matrix4f projection;
};

struct RenderedText {
	VulkanBuffer buffer;
	i32          vertex_count;
};

struct GameState {
	VulkanDevice        vulkan;
	VulkanPipeline      pipeline;

	VulkanTexture       texture;

	i32                 num_objects;
	VulkanBuffer        vertex_buffer;
	Matrix4f            *positions;

	VkCommandBuffer     *command_buffers;

	Camera              camera;
	VulkanUniformBuffer camera_ubo;

	Vector3f            velocity = {};
	Vector3f            player_velocity = {};

	VulkanPipeline      font_pipeline;
	VulkanTexture       font_texture;

	stbtt_bakedchar     baked_font[96];
	RenderedText        rendered_text;
	RenderedText        debug_text;
};

RenderedText render_font(GameState *game, float *x, float *y, const char *str)
{
	RenderedText text = {};

	i32 offset = 0;

	size_t text_length = strlen(str);
	size_t vertices_size = sizeof(f32)*30*text_length;
	f32 *vertices = (f32*)malloc(vertices_size);

	text.vertex_count = text_length * 6;

	while (*str) {
		char c = *str++;

		stbtt_aligned_quad q = {};
		stbtt_GetBakedQuad(game->baked_font, 512, 512, c-32, x, y, &q, 1);

		vertices[offset++] = q.x0;
		vertices[offset++] = q.y0;
		vertices[offset++] = 0.0f;
		vertices[offset++] = q.s0;
		vertices[offset++] = q.t0;

		vertices[offset++] = q.x1;
		vertices[offset++] = q.y0;
		vertices[offset++] = 0.0f;
		vertices[offset++] = q.s1;
		vertices[offset++] = q.t0;

		vertices[offset++] = q.x1;
		vertices[offset++] = q.y1;
		vertices[offset++] = 0.0f;
		vertices[offset++] = q.s1;
		vertices[offset++] = q.t1;

		vertices[offset++] = q.x1;
		vertices[offset++] = q.y1;
		vertices[offset++] = 0.0f;
		vertices[offset++] = q.s1;
		vertices[offset++] = q.t1;

		vertices[offset++] = q.x0;
		vertices[offset++] = q.y1;
		vertices[offset++] = 0.0f;
		vertices[offset++] = q.s0;
		vertices[offset++] = q.t1;

		vertices[offset++] = q.x0;
		vertices[offset++] = q.y0;
		vertices[offset++] = 0.0f;
		vertices[offset++] = q.s0;
		vertices[offset++] = q.t0;
	}

	text.buffer = create_vertex_buffer(&game->vulkan, vertices_size, vertices);
	return text;
}


void game_load_settings(Settings *settings)
{
	VAR_UNUSED(settings);
	char *settings_path = platform_resolve_path(GamePath_preferences,
	                                            "settings.conf");
	SERIALIZE_LOAD_CONF(settings_path, Settings, settings);
	free(settings_path);
}

void game_init(Settings *settings, PlatformState *platform, GameState *game)
{
	VAR_UNUSED(platform);
	game->vulkan = create_device(settings, platform);

	Vector4f pixels[32 * 32] = {};
	pixels[0]     = { 1.0f, 0.0f, 0.0f, 1.0f };
	pixels[31]    = { 0.0f, 1.0f, 0.0f, 1.0f };
	pixels[1023]     = { 0.0f, 0.0f, 1.0f, 1.0f };

	VkComponentMapping components = {};
	game->texture = create_texture(&game->vulkan,
	                               32, 32, VK_FORMAT_R32G32B32A32_SFLOAT,
	                               pixels, components);

	game->camera.view  = Matrix4f::identity();
	game->camera.view  = translate(game->camera.view, Vector3f{0.0f, 0.0f, 0.0f});

	f32 left   = - (f32)settings->video.resolution.width / 2.0f;
	f32 right  =   (f32)settings->video.resolution.width / 2.0f;
	f32 bottom = - (f32)settings->video.resolution.height / 2.0f;
	f32 top    =   (f32)settings->video.resolution.height / 2.0f;
	game->camera.projection = Matrix4f::orthographic(left, right, top, bottom, 0.0f, 1.0f);

	game->camera_ubo = create_uniform_buffer(&game->vulkan, sizeof(Camera));
	update_uniform_data(&game->vulkan, game->camera_ubo,
	                    &game->camera, 0, sizeof(Camera));

	f32 vertices[] = {
		-16.0f, -16.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,  0.0f,
		 16.0f, -16.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
		 16.0f,  16.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
		 16.0f,  16.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
		-16.0f,  16.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
		-16.0f, -16.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	};

	game->num_objects = 5;
	game->positions  = (Matrix4f*) malloc(5 * sizeof(Matrix4f));
	game->positions[0] = translate(Matrix4f::identity(), Vector3f{0.0f, 0.0f, 0.0f});
	game->positions[1] = translate(Matrix4f::identity(), Vector3f{40.0f, 0.0f, 0.0f});
	game->positions[2] = translate(Matrix4f::identity(), Vector3f{-40.0f, 0.0f, 0.0f});
	game->positions[3] = translate(Matrix4f::identity(), Vector3f{0.0f, 40.0f, 0.0f});
	game->positions[4] = translate(Matrix4f::identity(), Vector3f{0.0f, -40.0f, 0.0f});

	VkResult result;

	game->command_buffers = (VkCommandBuffer*) malloc(5 * sizeof(VkCommandBuffer));
	VkCommandBufferAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate_info.commandPool        = game->vulkan.command_pool;
	allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocate_info.commandBufferCount = game->num_objects;

	result = vkAllocateCommandBuffers(game->vulkan.handle,
	                                  &allocate_info,
	                                  game->command_buffers);
	DEBUG_ASSERT(result == VK_SUCCESS);

	game->vertex_buffer = create_vertex_buffer(&game->vulkan,
	                                           sizeof(vertices),
	                                           vertices);



	game->pipeline = create_pipeline(&game->vulkan);
	update_descriptor_sets(&game->vulkan,
	                       game->pipeline,
	                       game->texture,
	                       game->camera_ubo);


	{
		game->font_pipeline = create_font_pipeline(&game->vulkan);

		size_t font_size;
		char *font_path = platform_resolve_path(GamePath_data,
		                                        "fonts/Roboto-Regular.ttf");
		u8 *font_data = (u8*)platform_file_read(font_path, &font_size);

		u8 bitmap[512*512];
		stbtt_BakeFontBitmap(font_data, 0, 32.0, bitmap, 512, 512, 32,
		                     96, game->baked_font);

		components.a = VK_COMPONENT_SWIZZLE_R;
		game->font_texture = create_texture(&game->vulkan, 512, 512,
		                                    VK_FORMAT_R8_UNORM, bitmap,
		                                    components);

		update_descriptor_sets(&game->vulkan,
		                       game->font_pipeline,
		                       game->font_texture,
		                       game->camera_ubo);

		f32 x = 0.0f, y = 0.0f;
		game->rendered_text = render_font(game, &x, &y, "Hello, World!");
		game->debug_text = render_font(game, &x, &y, "frame time:");
	}
}

void game_input(GameState *game, InputAction action, float axis)
{
	switch (action) {
	case InputAction_move_vertical_start:
		game->velocity.y += axis * 100.0f;
		break;
	case InputAction_move_horizontal_start:
		game->velocity.x += axis * 100.0f;
		break;
	case InputAction_move_player_vertical_start:
		game->player_velocity.y += axis * 100.0f;
		break;
	case InputAction_move_player_horizontal_start:
		game->player_velocity.x += axis * 100.0f;
		break;
	default: break;
	}
}

void game_input(GameState *game, InputAction action)
{
	switch (action) {
	case InputAction_move_vertical_end:
		game->velocity.y = 0.0f;
		break;
	case InputAction_move_horizontal_end:
		game->velocity.x = 0.0f;
		break;
	case InputAction_move_player_vertical_end:
		game->player_velocity.y = 0.0f;
		break;
	case InputAction_move_player_horizontal_end:
		game->player_velocity.x = 0.0f;
		break;
	default: break;
	}
}

void game_quit(Settings *settings, GameState *game)
{
	VAR_UNUSED(settings);
	vkQueueWaitIdle(game->vulkan.queue);

	destroy(&game->vulkan, game->rendered_text.buffer);
	destroy(&game->vulkan, game->debug_text.buffer);
	destroy(&game->vulkan, game->font_texture);
	destroy(&game->vulkan, game->font_pipeline);

	destroy(&game->vulkan, game->vertex_buffer);
	destroy(&game->vulkan, game->camera_ubo);
	destroy(&game->vulkan, game->texture);
	destroy(&game->vulkan, game->pipeline);
	destroy(&game->vulkan);


	char *settings_path = platform_resolve_path(GamePath_preferences,
	                                            "settings.conf");
	SERIALIZE_SAVE_CONF(settings_path, Settings, settings);
	free(settings_path);

	platform_quit();
}

void game_update(GameState* game, f32 dt)
{
	destroy(&game->vulkan, game->debug_text.buffer);
	char buffer[1024];
	f32 dt_ms = dt * 1000.0f;
	f32 fps   = 1000.0f / dt_ms;
	sprintf(buffer, "frametime: %fms, %f fps", dt_ms, fps);
	f32 x = -640.0f, y = -340.0f;
	game->debug_text = render_font(game, &x, &y, buffer);


	game->camera.view = translate(game->camera.view, dt * game->velocity);
	game->positions[0] = translate(game->positions[0],
	                               dt * game->player_velocity);
	update_uniform_data(&game->vulkan, game->camera_ubo,
	                    &game->camera, 0, sizeof(Camera));
}

void game_render(GameState *game)
{
	u32 image_index = acquire_swapchain_image(&game->vulkan);

	std::array<VkClearValue, 1> clear_values = {};
	clear_values[0].color        = { {1.0f, 0.0f, 0.0f, 0.0f} };

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

	vkCmdBindPipeline(command,
	                  VK_PIPELINE_BIND_POINT_GRAPHICS,
	                  game->pipeline.handle);


	vkCmdBindDescriptorSets(command,
	                        VK_PIPELINE_BIND_POINT_GRAPHICS,
	                        game->pipeline.layout,
	                        0,
	                        1, &game->pipeline.descriptor_set,
	                        0, nullptr);


	vkCmdBindVertexBuffers(command, 0, 1, &game->vertex_buffer.handle, offsets);

	for (i32 i = 0; i < game->num_objects; i++) {
		vkCmdPushConstants(command,
		                   game->pipeline.layout,
		                   VK_SHADER_STAGE_VERTEX_BIT,
		                   0, sizeof(Matrix4f),
		                   &game->positions[i]);
		vkCmdDraw(command, 6, 1, 0, 0);
	}

	vkCmdBindPipeline(command,
	                  VK_PIPELINE_BIND_POINT_GRAPHICS,
	                  game->font_pipeline.handle);


	vkCmdBindDescriptorSets(command,
	                        VK_PIPELINE_BIND_POINT_GRAPHICS,
	                        game->font_pipeline.layout,
	                        0,
	                        1, &game->font_pipeline.descriptor_set,
	                        0, nullptr);


	vkCmdBindVertexBuffers(command, 0, 1, &game->rendered_text.buffer.handle,
	                       offsets);
	vkCmdDraw(command, game->rendered_text.vertex_count, 1, 0, 0);

	vkCmdBindVertexBuffers(command, 0, 1, &game->debug_text.buffer.handle,
	                       offsets);
	vkCmdDraw(command, game->debug_text.vertex_count, 1, 0, 0);


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
	present_info.pWaitSemaphores    = wait_semaphores;
	present_info.swapchainCount     = 1;
	present_info.pSwapchains        = swapchains;
	present_info.pImageIndices      = &image_index;

	result = vkQueuePresentKHR(game->vulkan.queue, &present_info);
	DEBUG_ASSERT(result == VK_SUCCESS);

	result = vkQueueWaitIdle(game->vulkan.queue);
	DEBUG_ASSERT(result == VK_SUCCESS);
}
