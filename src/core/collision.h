#ifndef LEARY_COLLISION_H
#define LEARY_COLLISION_H

struct CollidableAABB {
    Vector3 position;
    Vector3 scale;
    bool colliding = false;
};

struct CollidableSphere {
    Vector3 position;
    f32 radius;
    f32 radius_sq;
    bool colliding = false;
};

struct Collision {
    Array<CollidableAABB>   aabbs;
    Array<CollidableSphere> spheres;
};

struct DebugCollision {
    bool render_collidables = true;

    PushConstants pc_normal;
    PushConstants pc_collide;

    struct {
        VulkanBuffer vbo;
        i32 vertex_count = 0;
    } cube;

    struct {
        VulkanBuffer vbo;
        VulkanBuffer ibo;
        i32 index_count = 0;
    } sphere;
};

#endif // LEARY_COLLISION_H
