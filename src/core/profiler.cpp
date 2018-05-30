/**
 * file:    profiler.cpp
 * created: 2017-02-27
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include "profiler.h"
#include "gfx_vulkan.h"
#include "random.h"

#define PROFILER_MAX_STACK_DEPTH (256)

extern VulkanDevice* g_vulkan;

enum ProfileEventType {
    ProfileEvent_start,
    ProfileEvent_end
};

struct ProfileEvent {
    ProfileEventType type;
    const char *name;
    u64 timestamp;
};

struct ProfileTimer {
    const char *name;
    u64 duration;
    u64 calls;
    i32 parent;
};

Array<ProfileEvent> g_profile_events;
Array<ProfileEvent> g_profile_events_prev;
Array<ProfileTimer> g_profile_timers;

GfxTexture g_profiler_graph;
constexpr i32 kProfilerGraphWidth  = 128;
constexpr i32 kProfilerGraphHeight = 512;

void profiler_start(const char *name)
{
    ProfileEvent event;
    event.type = ProfileEvent_start;
    event.name = name;
    event.timestamp = cpu_ticks();

    array_add(&g_profile_events, event);
}

void profiler_end(const char *name)
{
    ProfileEvent event;
    event.type = ProfileEvent_end;
    event.name = name;
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
    // TODO(jesper): we should double buffer this texture in some way so we can
    // transition the next frame's image for CPU access, avoiding
    // synchronisation stalls
    g_profiler_graph = gfx_create_texture(
        VK_FORMAT_B8G8R8A8_UNORM,
        kProfilerGraphWidth,
        kProfilerGraphHeight,
        VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

    VulkanPipeline &pipeline = g_vulkan->pipelines[Pipeline_basic2d];

    GfxDescriptorSet ds = gfx_create_descriptor(
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        pipeline.descriptor_layout_material);
    ASSERT(ds.id != -1);

    gfx_set_texture(ds, g_profiler_graph, ResourceSlot_diffuse, Pipeline_basic2d);

    Vector2 dim = Vector2{ (f32)kProfilerGraphHeight, (f32)kProfilerGraphWidth };
    dim.x = dim.x / g_settings.video.resolution.width;
    dim.y = dim.y / g_settings.video.resolution.height;

    f32 vertices[] = {
        0.0f, 0.0f,  1.0f, 0.0f,
        dim.x, 0.0f,  1.0f, 1.0f,
        dim.x, dim.y, 0.0f, 1.0f,

        dim.x, dim.y, 0.0f, 1.0f,
        0.0f,  dim.y, 0.0f, 0.0f,
        0.0f,  0.0f,  1.0f, 0.0f,
    };

    VulkanBuffer vbo = create_vbo(vertices, sizeof(vertices));
    debug_add_texture("timers", vbo, ds, Pipeline_basic2d, &g_game->overlay);
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

    f32 df = (f32)duration / (f32)max_duration;
    i32 w = (i32)(df * kProfilerGraphWidth);

    static u32 current_frame = 0;
    current_frame = (current_frame + 1) % kProfilerGraphHeight;

    struct BGRA8 {
        u8 b, g, r, a;
    };

    Random r = create_random(0xdeadbeef);

    gfx_transition_immediate(
        &g_profiler_graph,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_PIPELINE_STAGE_HOST_BIT);

    BGRA8 *pixels = nullptr;
    vkMapMemory(
        g_vulkan->handle,
        g_profiler_graph.vk_memory,
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

#if 1
    for (i32 i = 0; i < w; i++) {
        pixels[i].r = 0;
        pixels[i].g = 0;
        pixels[i].b = 255;
        pixels[i].a = 255;
    }
#endif

    vkUnmapMemory(g_vulkan->handle, g_profiler_graph.vk_memory);

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
            timer.name = event.name;
            timer.duration = event.timestamp - parent.timestamp;
            timer.calls = 1;

            i32 j = 0;
            for (j = 0; j < g_profile_timers.count; j++) {
                ProfileTimer exist = g_profile_timers[j];

                if (exist.name == timer.name ||
                    strcmp(exist.name, timer.name) == 0)
                {
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

    {
        PROFILE_SCOPE(profiler_timer_sort);
        array_ins_sort(
            &g_profile_timers,
            [](ProfileTimer *lhs, ProfileTimer *rhs) {
                return lhs->duration < rhs->duration;
            });
    }
}

void profiler_end_frame()
{
}
