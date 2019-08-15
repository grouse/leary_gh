/**
 * file:    profiler.cpp
 * created: 2017-02-27
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017-2018 - all rights reserved
 */

#define PROFILER_MAX_STACK_DEPTH (256)

extern VulkanDevice* g_vulkan;

Array<ProfileEvent> g_profile_events;
Array<ProfileEvent> g_profile_events_prev;
Array<ProfileTimer> g_profile_timers;

GfxTexture g_profiler_cpu_graph;
GfxTexture g_profiler_gpu_graph;
constexpr i32 kProfilerGraphWidth  = 128;
constexpr i32 kProfilerGraphHeight = 512;

void profiler_start(const char *name, const char *file, i32 line)
{
    ProfileEvent event;
    event.type = ProfileEvent_start;
    event.name = name;
    event.file = file;
    event.line = line;
    event.timestamp = cpu_ticks();

    array_add(&g_profile_events, event);
}

void profiler_end(const char *name, const char *file, i32 line)
{
    ProfileEvent event;
    event.type = ProfileEvent_end;
    event.name = name;
    event.file = file;
    event.line = line;
    event.timestamp = cpu_ticks();

    array_add(&g_profile_events, event);
}

void init_profiler()
{
    init_array(&g_profile_events, g_heap);
    init_array(&g_profile_events_prev, g_heap);
    init_array(&g_profile_timers, g_heap);
}

void init_profiler_gui()
{
    VulkanPipeline &pipeline = g_vulkan->pipelines[Pipeline_basic2d];

    Vector2 dim = Vector2{ (f32)kProfilerGraphHeight, (f32)kProfilerGraphWidth };
    dim.x = dim.x / g_settings.video.resolution.width;
    dim.y = dim.y / g_settings.video.resolution.height;

    f32 vertices[] = {
        0.0f, 0.0f,  1.0f, 0.0f,
        0.0f, dim.y,  0.0f, 0.0f,
        dim.x, dim.y, 0.0f, 1.0f,

        dim.x, dim.y, 0.0f, 1.0f,
        dim.x,  0.0f, 1.0f, 1.0f,
        0.0f,  0.0f,  1.0f, 0.0f,
    };

    // TODO(jesper): these are separate VBOs right now because of how we're
    // handling the VBO dstruction of overlay render items.
    VulkanBuffer cpu_vbo = create_vbo(vertices, sizeof(vertices));
    VulkanBuffer gpu_vbo = create_vbo(vertices, sizeof(vertices));

    // TODO(jesper): we should double buffer this texture in some way so we can
    // transition the next frame's image for CPU access, avoiding
    // synchronisation stalls
    {
        g_profiler_cpu_graph = gfx_create_texture(
            kProfilerGraphWidth,
            kProfilerGraphHeight,
            1,
            VK_FORMAT_B8G8R8A8_UNORM,
            VK_IMAGE_TILING_LINEAR,
            VkComponentMapping{},
            VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

        GfxDescriptorSet ds = gfx_create_descriptor(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            pipeline.set_layouts[1]);
        ASSERT(ds.id != -1);

        gfx_transition_immediate(
            &g_profiler_cpu_graph,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_PIPELINE_STAGE_HOST_BIT);

        gfx_set_texture(Pipeline_basic2d, ds, 0, g_profiler_cpu_graph);

        debug_add_texture(
            "CPU",
            { (f32)kProfilerGraphHeight, (f32)kProfilerGraphWidth },
            cpu_vbo,
            ds,
            Pipeline_basic2d,
            &g_game->overlay);
    }

    {
        g_profiler_gpu_graph = gfx_create_texture(
            kProfilerGraphWidth,
            kProfilerGraphHeight,
            1,
            VK_FORMAT_B8G8R8A8_UNORM,
            VK_IMAGE_TILING_LINEAR,
            VkComponentMapping{},
            VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

        gfx_transition_immediate(
            &g_profiler_gpu_graph,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_PIPELINE_STAGE_HOST_BIT);

        GfxDescriptorSet ds = gfx_create_descriptor(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            pipeline.set_layouts[1]);
        ASSERT(ds.id != -1);

        gfx_set_texture(Pipeline_basic2d, ds, 0, g_profiler_gpu_graph);

        debug_add_texture(
            "GPU",
            { (f32)kProfilerGraphHeight, (f32)kProfilerGraphWidth },
            gpu_vbo,
            ds,
            Pipeline_basic2d,
            &g_game->overlay);
    }
}

void profiler_begin_frame()
{
    // TODO(jesper): THREAD_SAFETY
    std::swap(g_profile_events, g_profile_events_prev);
    g_profile_events.count = 0;
    g_profile_timers.count = 0;

    PROFILE_FUNCTION();

    static u64 last_ticks = cpu_ticks();
    static u32 max_duration = 0;

    u64 ticks = cpu_ticks();
    u32 duration = (u32)(ticks - last_ticks);

    last_ticks = ticks;
    max_duration = MAX(max_duration, duration);

    static u32 current_frame = 0;
    current_frame = (current_frame + 1) % kProfilerGraphHeight;

    struct BGRA8 {
        u8 b, g, r, a;
    };

    {
        PROFILE_SCOPE(profile_timer_cpu_graph);

        f32 df = (f32)duration / (f32)max_duration;
        i32 w = (i32)(df * kProfilerGraphWidth);

        BGRA8 *pixels = nullptr;
        vkMapMemory(
            g_vulkan->handle,
            g_profiler_cpu_graph.vk_memory,
            current_frame * kProfilerGraphWidth * sizeof(BGRA8),
            kProfilerGraphWidth * sizeof(BGRA8),
            0,
            (void**)&pixels);
        ASSERT(pixels != nullptr);

        for (i32 j = 0; j < kProfilerGraphWidth; j++) {
            pixels[j].r = 255;
            pixels[j].g = 255;
            pixels[j].b = 255;
            pixels[j].a = 255;
        }

        for (i32 i = 0; i < w; i++) {
            pixels[i].r = 0;
            pixels[i].g = 0;
            pixels[i].b = 255;
            pixels[i].a = 255;
        }

        vkUnmapMemory(g_vulkan->handle, g_profiler_cpu_graph.vk_memory);
    }

    {
        PROFILE_SCOPE(profile_timer_gpu_graph);

        static f32 max_gpu_time = 0.0f;
        max_gpu_time = MAX(max_gpu_time, g_vulkan->gpu_time);

        f32 df = g_vulkan->gpu_time/ max_gpu_time;
        i32 w = (i32)(df * kProfilerGraphWidth);

        BGRA8 *pixels = nullptr;
        vkMapMemory(
            g_vulkan->handle,
            g_profiler_gpu_graph.vk_memory,
            current_frame * kProfilerGraphWidth * sizeof(BGRA8),
            kProfilerGraphWidth * sizeof(BGRA8),
            0,
            (void**)&pixels);
        ASSERT(pixels != nullptr);

        for (i32 j = 0; j < kProfilerGraphWidth; j++) {
            pixels[j].r = 255;
            pixels[j].g = 255;
            pixels[j].b = 255;
            pixels[j].a = 255;
        }

        for (i32 i = 0; i < w; i++) {
            pixels[i].r = 0;
            pixels[i].g = 0;
            pixels[i].b = 255;
            pixels[i].a = 255;
        }

        vkUnmapMemory(g_vulkan->handle, g_profiler_gpu_graph.vk_memory);
    }

    {
        PROFILE_SCOPE(profile_timer_gather);

        i32 s = 0;
        i32 stack[PROFILER_MAX_STACK_DEPTH];

        for (i32 i = 0; i < PROFILER_MAX_STACK_DEPTH; i++) {
            stack[i] = -1;
        }

        // TODO(jesper): thread local g_profile_events
        for (i32 i = 0; i < g_profile_events_prev.count; i++) {
            ProfileEvent event = g_profile_events_prev[i];

            if (event.type == ProfileEvent_start) {
                stack[s++] = i;
                ASSERT(s < PROFILER_MAX_STACK_DEPTH);
            } else if (event.type == ProfileEvent_end) {
                i32 pid = stack[--s];
                ASSERT(pid != -1);

                ProfileEvent parent = g_profile_events_prev[pid];
                ASSERT(event.name == parent.name || strcmp(event.name, parent.name) == 0);

                ProfileTimer timer;
                timer.name = parent.name;
                timer.file = parent.file;
                timer.line = parent.line;
                timer.duration = event.timestamp - parent.timestamp;
                timer.calls = 1;

                i32 j = 0;
                for (j = 0; j < g_profile_timers.count; j++) {
                    ProfileTimer exist = g_profile_timers[j];

                    if (timer.file == exist.file && timer.line == exist.line) {
                        goto merge_existing;
                    }
                }

                array_add(&g_profile_timers, timer);
                continue;

    merge_existing:
                ProfileTimer &existing = g_profile_timers[j];
                existing.calls++;
                existing.duration += timer.duration;
            }
        }
    }

    {
        PROFILE_SCOPE(profiler_timer_sort);
        array_insertion_sort(
            &g_profile_timers,
            [](ProfileTimer *lhs, ProfileTimer *rhs) {
                return lhs->duration < rhs->duration;
            });
    }
}

void profiler_end_frame()
{
}
