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

struct Mesh {
	Vertex *vertices;
	i32    vertices_count;

	i32    *indices;
	i32    indices_count;
};

Mesh load_mesh_obj(const char *filename)
{
	Mesh mesh = {};

	char *path = platform_resolve_path(GamePath_models, filename);

	usize size;
	char *file = platform_file_read(path, &size);
	char *end  = file + size;

	DEBUG_ASSERT(file != nullptr);

	std::vector<Vector3> vertices;
	std::vector<Vector3> normals;
	std::vector<Vector2> uvs;

	i32 num_indices = 0;

	bool has_normals = false;
	bool has_uvs     = false;

	char *ptr = file;
	while (ptr < end) {
		// texture coordinates
		if (ptr[0] == 'v' && ptr[1] == 't') {
			has_uvs = true;

			do ptr++;
			while (ptr[0] == ' ');

			Vector2 uv;
			sscanf(ptr, "%f %f", &uv.x, &uv.y);
			uvs.push_back(uv);
		// normals
		} else if (ptr[0] == 'v' && ptr[1] == 'n') {
			has_normals = true;

			do ptr++;
			while (ptr[0] == ' ');

			Vector3 n;
			sscanf(ptr, "%f %f %f", &n.x, &n.y, &n.z);
			normals.push_back(n);
		// vertices
		} else if (ptr[0] == 'v') {
			do ptr++;
			while (ptr[0] == ' ');

			Vector3 v;
			sscanf(ptr, "%f %f %f", &v.x, &v.y, &v.z);
			vertices.push_back(v);
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
			num_indices += 3;
		}

		do ptr++;
		while (ptr < end && !is_newline(ptr[0]));

		do ptr++;
		while (ptr < end && is_newline(ptr[0]));
	}

	DEBUG_ASSERT(has_normals);
	DEBUG_ASSERT(has_uvs);

	mesh.vertices       = new Vertex[vertices.size()];
	mesh.vertices_count = vertices.size();

	for (i32 i = 0; i < (i32)vertices.size(); i++) {
		mesh.vertices[i].vector = vertices[i];
	}

	mesh.indices        = new i32[num_indices];
	mesh.indices_count  = num_indices;

	i32 indices_index = 0;

	ptr = file;
	while (ptr < end) {
		if (ptr[0] == 'f') {
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

			i32 iv0, iv1, iv2;
			i32 it0, it1, it2;
			i32 in0, in1, in2;

			sscanf(ptr, "%d/%d/%d %d/%d/%d %d/%d/%d",
			       &iv0, &iv1, &iv2, &it0, &it1, &it2, &in0, &in1, &in2);

			mesh.indices[indices_index++] = iv0;
			mesh.indices[indices_index++] = iv1;
			mesh.indices[indices_index++] = iv2;

			mesh.vertices[iv0].uv = uvs[it0];
			mesh.vertices[iv1].uv = uvs[it1];
			mesh.vertices[iv2].uv = uvs[it2];

			mesh.vertices[iv0].normal = normals[in0];
			mesh.vertices[iv1].normal = normals[in1];
			mesh.vertices[iv2].normal = normals[in2];
		}

		do ptr++;
		while (ptr < end && !is_newline(ptr[0]));

		do ptr++;
		while (ptr < end && is_newline(ptr[0]));
	}

	DEBUG_LOG("num vertices: %d", mesh.vertices_count);
	DEBUG_LOG("num indices: %d", mesh.indices_count);

	return mesh;
}

