/**
 * file:    mesh.cpp
 * created: 2017-03-01
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include <vector>

struct Vertex {
	Vector3 vector;
	Vector3 normal;
	Vector2 uv;
};

bool operator == (Vertex &lhs, Vertex &rhs)
{
	return memcmp(&lhs, &rhs, sizeof(Vertex)) == 0;
}

struct Mesh {
	Array<Vertex, LinearAllocator> vertices;
	Array<u32, LinearAllocator>    indices;
};

Mesh load_mesh_obj(GameMemory *memory, const char *filename)
{
	Mesh mesh = {};

	char *path = platform_resolve_path(GamePath_models, filename);
	DEBUG_LOG("Loading mesh: %s", path);

	usize size;
	char *file = platform_file_read(path, &size);
	DEBUG_ASSERT(file != nullptr);

	char *end  = file + size;

	DEBUG_LOG("-- file size: %llu bytes", size);

	i32 num_faces   = 0;

	auto vectors = make_array<Vector3>(&memory->free_list);
	auto normals = make_array<Vector3>(&memory->frame);
	auto uvs     = make_array<Vector2>(&memory->frame);

	char *ptr = file;
	while (ptr < end) {
		// texture coordinates
		if (ptr[0] == 'v' && ptr[1] == 't') {
			ptr += 3;

			Vector2 uv;
			sscanf(ptr, "%f %f", &uv.x, &uv.y);
			array_add(&uvs, uv);
		// normals
		} else if (ptr[0] == 'v' && ptr[1] == 'n') {
			ptr += 3;

			Vector3 n;
			sscanf(ptr, "%f %f %f", &n.x, &n.y, &n.z);
			array_add(&normals, n);
		// vertices
		} else if (ptr[0] == 'v') {
			ptr += 2;

			Vector3 v;
			sscanf(ptr, "%f %f %f", &v.x, &v.y, &v.z);
			array_add(&vectors, v);
		// faces
		} else if (ptr[0] == 'f') {
			do ptr++;
			while (ptr[0] == ' ');

			i32 num_dividers = 0;
			char *face = ptr;
			do {
				if (*face++ == '/') {
					num_dividers++;
				}
			} while (face < end && face[0] != '\n' && face[0] != '\r');

			// NOTE(jesper): only supporting triangulated meshes
			DEBUG_ASSERT(num_dividers == 6);
			num_faces++;
		}

		do ptr++;
		while (ptr < end && !is_newline(ptr[0]));

		do ptr++;
		while (ptr < end && is_newline(ptr[0]));
	}

	DEBUG_ASSERT(vectors.count > 0);
	DEBUG_ASSERT(normals.count > 0);
	DEBUG_ASSERT(uvs.count> 0);
	DEBUG_ASSERT(num_faces> 0);

	DEBUG_LOG("-- vectors : %d", vectors.count);
	DEBUG_LOG("-- normals : %d", normals.count);
	DEBUG_LOG("-- uvs     : %d", uvs.count);
	DEBUG_LOG("-- faces   : %d", num_faces);

	auto vertices = make_array<Vertex>(&memory->frame);

	ptr = file;
	while (ptr < end) {
		if (ptr[0] == 'f') {
			do ptr++;
			while (ptr[0] == ' ');

			u32 iv0, iv1, iv2;
			u32 it0, it1, it2;
			u32 in0, in1, in2;

			sscanf(ptr, "%u/%u/%u %u/%u/%u %u/%u/%u",
			       &iv0, &it0, &in0, &iv1, &it1, &in1, &iv2, &it2, &in2);

			// NOTE(jesper): objs are 1 indexed
			iv0--; iv1--; iv2--;
			it0--; it1--; it2--;
			in0--; in1--; in2--;

			Vertex v0;
			v0.vector = vectors[iv0];
			v0.normal = normals[in0];
			v0.uv     = uvs[it0];

			Vertex v1;
			v1.vector = vectors[iv1];
			v1.normal = normals[in1];
			v1.uv     = uvs[it1];

			Vertex v2;
			v2.vector = vectors[iv2];
			v2.normal = normals[in2];
			v2.uv     = uvs[it2];

			array_add(&vertices, v0);
			array_add(&vertices, v1);
			array_add(&vertices, v2);
		}

		do ptr++;
		while (ptr < end && !is_newline(ptr[0]));

		do ptr++;
		while (ptr < end && is_newline(ptr[0]));
	}

	mesh.vertices = make_array<Vertex>(&memory->persistent);
	mesh.indices  = make_array<u32>(&memory->persistent);

	i32 j = 0;
	for (i32 i = 0; i < vertices.count; i++) {
		bool unique = true;
		for (j = 0; j < mesh.vertices.count; j++) {
			if (vertices[i] == mesh.vertices[j]) {
				unique = false;
				break;
			}
		}

		if (unique) {
			u32 index = (u32)array_add(&mesh.vertices, vertices[i]);
			array_add(&mesh.indices, index);
		} else {
			u32 index = (u32)j;
			array_add(&mesh.indices, index);
		}
	}

#if 0
	for (i32 i = 0; i < mesh.vertices.count; i++) {
		DEBUG_LOG("vertex[%d]", i);
		DEBUG_LOG("vector = { %f, %f, %f }",
		          mesh.vertices[i].vector.x,
		          mesh.vertices[i].vector.y,
		          mesh.vertices[i].vector.z);
		DEBUG_LOG("normal = { %f, %f, %f }",
		          mesh.vertices[i].normal.x,
		          mesh.vertices[i].normal.y,
		          mesh.vertices[i].normal.z);
		DEBUG_LOG("uv = { %f, %f }",
		          mesh.vertices[i].uv.x,
		          mesh.vertices[i].uv.y);
	}

	for (i32 i = 0; i < mesh.indices.count; i += 3) {
		DEBUG_LOG("triangle: %u, %u, %u",
		          mesh.indices[i+0],
		          mesh.indices[i+1],
		          mesh.indices[i+2]);
	}
#endif

	return mesh;
}

