/**
 * file:    collision.cpp
 * created: 2017-11-27
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

struct CollidableAABB {
    Vector3 position;
    Vector3 scale;
    bool colliding = false;
};

struct Collision {
    Array<CollidableAABB> aabbs;
};

struct DebugCollision {
    bool render_collidables = true;

    PushConstants pc_normal;
    PushConstants pc_collide;

    struct {
        VulkanBuffer vbo;
        i32 vertex_count = 0;
    } cube;
};

Collision g_collision = {};
DebugCollision g_debug_collision = {};

void init_collision()
{
    init_array(&g_collision.aabbs, g_heap);

    // debug cube wireframe
    {
        f32 vertices[] = {
            // front-face
            0.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 0.0f,

            1.0f, 0.0f, 0.0f,
            1.0f, 1.0f, 0.0f,

            1.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f,

            0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f,

            // back-face
            0.0f, 0.0f, 1.0f,
            1.0f, 0.0f, 1.0f,

            1.0f, 0.0f, 1.0f,
            1.0f, 1.0f, 1.0f,

            1.0f, 1.0f, 1.0f,
            0.0f, 1.0f, 1.0f,

            0.0f, 1.0f, 1.0f,
            0.0f, 0.0f, 1.0f,

            // top-face
            0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f,

            1.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 1.0f,

            // bottom-face
            0.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 1.0f,

            1.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 1.0f,
        };

        g_debug_collision.cube.vbo = create_vbo(vertices, sizeof(vertices) * sizeof(f32));
        g_debug_collision.cube.vertex_count = 24;
    }

    CollidableAABB test = {};
    test.position  = { 0.0f, 0.0f, 0.0f };
    test.scale     = { 1.0f, 1.0f, 1.0f };
    array_add(&g_collision.aabbs, test);

    test.position  = { 1.0f, 0.0f, 0.0f };
    test.scale     = { 1.0f, 1.0f, 1.0f };
    array_add(&g_collision.aabbs, test);
}

void process_collision()
{
    for (auto &c : g_collision.aabbs) {
        c.colliding = false;
    }

    for (i32 i = 0; i < g_collision.aabbs.count - 1; i++) {
        auto &c1 = g_collision.aabbs[i];
        Vector3 hs1 = c1.scale / 2.0f;

        for (i32 j = i+1; j < g_collision.aabbs.count; j++) {
            auto &c2 = g_collision.aabbs[j];
            Vector3 hs2 = c2.scale / 2.0f;

            if ((c1.position.x + hs1.x) > (c2.position.x - hs2.x) &&
                (c1.position.x - hs1.x) < (c2.position.x + hs2.x) &&
                (c1.position.y + hs1.y) > (c2.position.y - hs2.y) &&
                (c1.position.y - hs1.y) < (c2.position.y + hs2.y) &&
                (c1.position.z + hs1.z) > (c2.position.z - hs2.z) &&
                (c1.position.z - hs1.z) < (c2.position.z + hs2.z))
            {
                c1.colliding = true;
                c2.colliding = true;
            }
        }
    }
}
