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

void aabb_penetration(Vector3 axis, Vector2 a, Vector2 b, f32 *mtv_lsq, Vector3 *mtv)
{
    f32 d0 = b.max - a.min;
    f32 d1 = a.max - b.min;

    if (d0 <= 0.0f || d1 <= 0.0f) {
        return;
    }

    f32 overlap = (d0 < d1) ? d0 : -d1;

    Vector3 v = axis * overlap;
    f32 vlsq = length_sq(v);

    if (vlsq < *mtv_lsq) {
        *mtv_lsq = vlsq;
        *mtv = v;
    }
}

void process_collision()
{
    for (auto &c : g_collision.aabbs) {
        c.colliding = false;
    }

    for (i32 i = 0; i < g_collision.aabbs.count - 1; i++) {
        auto &c1 = g_collision.aabbs[i];
        Vector3 hs1 = c1.scale / 2.0f;

        f32 left1  = c1.position.x - hs1.x;
        f32 right1 = c1.position.x + hs1.x;
        f32 top1   = c1.position.y - hs1.y;
        f32 bot1   = c1.position.y + hs1.y;
        f32 back1  = c1.position.z - hs1.z;
        f32 front1 = c1.position.z + hs1.z;

        for (i32 j = i+1; j < g_collision.aabbs.count; j++) {
            auto &c2 = g_collision.aabbs[j];
            Vector3 hs2 = c2.scale / 2.0f;

            f32 left2  = c2.position.x - hs2.x;
            f32 right2 = c2.position.x + hs2.x;
            f32 top2   = c2.position.y - hs2.y;
            f32 bot2   = c2.position.y + hs2.y;
            f32 back2  = c2.position.z - hs2.z;
            f32 front2 = c2.position.z + hs2.z;

            if (right1 > left2 && left1 < right2 &&
                bot1   > top2  && top1  < bot2   &&
                front1 > back2 && back1 < front2)
            {
                c1.colliding = true;
                c2.colliding = true;

                Vector3 mtv = {};
                f32 mtv_lsq = F32_MAX;

                aabb_penetration({ 1.0f, 0.0f, 0.0f },
                                 { left1, right1},
                                 { left2, right2 },
                                 &mtv_lsq, &mtv);

                aabb_penetration({ 0.0f, 1.0f, 0.0f },
                                 { top1, bot1},
                                 { top2, bot2 },
                                 &mtv_lsq, &mtv);

                aabb_penetration({ 0.0f, 0.0f, 1.0f },
                                 { back1, front1},
                                 { back2, front2 },
                                 &mtv_lsq, &mtv);

                c1.position += normalise(mtv) * lry::sqrt(mtv_lsq);
            }
        }
    }
}

