/**
 * file:    collision.cpp
 * created: 2017-11-27
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

Collision g_collision = {};
DebugCollision g_debug_collision = {};

void init_collision()
{
    init_array(&g_collision.aabbs, g_heap);
    init_array(&g_collision.spheres, g_heap);

    // debug cube wireframe
    {
        f32 vertices[] = {
            // front-face
            -0.5f, -0.5f, -0.5f,
            0.5f, -0.5f, -0.5f,

            0.5f, -0.5f, -0.5f,
            0.5f, 0.5f, -0.5f,

            0.5f, 0.5f, -0.5f,
            -0.5f, 0.5f, -0.5f,

            -0.5f, 0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,

            // back-face
            -0.5f, -0.5f, 0.5f,
            0.5f, -0.5f, 0.5f,

            0.5f, -0.5f, 0.5f,
            0.5f, 0.5f, 0.5f,

            0.5f, 0.5f, 0.5f,
            -0.5f, 0.5f, 0.5f,

            -0.5f, 0.5f, 0.5f,
            -0.5f, -0.5f, 0.5f,

            // top-face
            -0.5f, -0.5f, -0.5f,
            -0.5f, -0.5f, 0.5f,

            0.5f, -0.5f, -0.5f,
            0.5f, -0.5f, 0.5f,

            // bottom-face
            -0.5f, 0.5f, -0.5f,
            -0.5f, 0.5f, 0.5f,

            0.5f, 0.5f, -0.5f,
            0.5f, 0.5f, 0.5f,
        };

        g_debug_collision.cube.vbo = create_vbo(vertices, sizeof(vertices));
        g_debug_collision.cube.vertex_count = 24;
    }

    // debug sphere wireframe
    {
        Mesh *mesh = find_mesh("unit_sphere.obj");
        ASSERT(mesh != nullptr);

        g_debug_collision.sphere.vbo = mesh->vbo.points;
        g_debug_collision.sphere.ibo = mesh->ibo;
        g_debug_collision.sphere.vertex_count = mesh->element_count;
    }

    CollidableAABB test = {};
    test.position  = { 3.0f, 0.0f, 0.0f };
    test.scale     = { 1.0f, 1.0f, 1.0f };
    array_add(&g_collision.aabbs, test);

    test.position  = { 1.0f, 3.0f, 0.0f };
    test.scale     = { 1.0f, 1.0f, 1.0f };
    array_add(&g_collision.aabbs, test);

    CollidableSphere stest = {};
    stest.position = { 0.0f, 0.0f, 0.0f };
    stest.radius    = 1.0f;
    stest.radius_sq = stest.radius * stest.radius;
    array_add(&g_collision.spheres, stest);

    stest = {};
    stest.position = { -3.0f, 0.0f, 0.0f };
    stest.radius    = 1.0f;
    stest.radius_sq = stest.radius * stest.radius;
    array_add(&g_collision.spheres, stest);
}

void process_collision()
{
    for (auto &c : g_collision.aabbs) {
        c.colliding = false;
    }

    for (auto &c : g_collision.spheres) {
        c.colliding = false;
    }


    // NOTE: aabb - aabb
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

                if (left1 < right2 && left2 < right1) {
                    f32 d0 = right2 - left1;
                    f32 d1 = right1 - left2;

                    f32 overlap = (d0 < d1) ? d0 : -d1;
                    Vector3 v   = { overlap, 0.0f, 0.0f };
                    f32 vlsq    = length_sq(v);

                    if (vlsq < mtv_lsq) {
                        mtv_lsq = vlsq;
                        mtv     = v;
                    }
                }

                if (top1 < bot2 && top2 < bot1) {
                    f32 d0 = bot2 - top1;
                    f32 d1 = bot1 - top2;

                    f32 overlap = (d0 < d1) ? d0 : -d1;
                    Vector3 v   = { 0.0f, overlap, 0.0f };
                    f32 vlsq    = length_sq(v);

                    if (vlsq < mtv_lsq) {
                        mtv_lsq = vlsq;
                        mtv     = v;
                    }
                }

                if (back1 < front2 && back2 < front1) {
                    f32 d0 = front2 - back1;
                    f32 d1 = front1 - back2;

                    f32 overlap = (d0 < d1) ? d0 : -d1;
                    Vector3 v   = { 0.0f, 0.0f, overlap };
                    f32 vlsq    = length_sq(v);

                    if (vlsq < mtv_lsq) {
                        mtv_lsq = vlsq;
                        mtv     = v;
                    }
                }


                c1.position += mtv;
            }
        }
    }

    // NOTE: sphere - sphere
    for (i32 i = 0; i < g_collision.spheres.count - 1; i++) {
        auto &a = g_collision.spheres[i];

        for (i32 j = i+1; j < g_collision.spheres.count; j++) {
            auto &b = g_collision.spheres[j];

            Vector3 d = a.position - b.position;
            f32 dist = length_sq(d);

            f32 rsum = a.radius_sq + b.radius_sq + 2 * a.radius * b.radius;
            if ( dist <= rsum ) {
                a.colliding = true;
                b.colliding = true;

                a.position -= normalise(d) * (sqrt(dist) - (a.radius + b.radius));
            }
        }
    }

    // NOTE: aabb - sphere
    for (i32 i = 0; i < g_collision.aabbs.count; i++) {
        auto &aabb = g_collision.aabbs[i];
        Vector3 hs = aabb.scale / 2.0f;

        f32 left  = aabb.position.x - hs.x;
        f32 top   = aabb.position.y - hs.y;
        f32 back  = aabb.position.z - hs.z;

        f32 right = aabb.position.x + hs.x;
        f32 bot   = aabb.position.y + hs.y;
        f32 front = aabb.position.z + hs.z;

        for (i32 j = 0; j < g_collision.spheres.count; j++) {
            auto &sphere = g_collision.spheres[j];

            Vector3 c = clamp(sphere.position, { left, top, back }, { right, bot, front });
            Vector3 sc = c - sphere.position;
            f32 d = length_sq(sc) - sphere.radius_sq;

            if (d < 0.0f) {
                aabb.colliding = true;
                sphere.colliding = true;

                aabb.position -= sc * d;
            }
        }
    }

}

