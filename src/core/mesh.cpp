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

	u32    *indices;
	i32    indices_count;
};

Mesh load_mesh_obj(const char *filename)
{
	Mesh mesh = {};

	char *path = platform_resolve_path(GamePath_models, filename);
	DEBUG_LOG("Loading mesh: %s", path);

	usize size;
	char *file = platform_file_read(path, &size);
	char *end  = file + size;

	DEBUG_LOG("-- file size: %llu bytes", size);

	DEBUG_ASSERT(file != nullptr);

	i32 num_indices  = 0;
	i32 num_vertices = 0;
	i32 num_normals  = 0;
	i32 num_uvs      = 0;

	char *ptr = file;
	while (ptr < end) {
		// texture coordinates
		if (ptr[0] == 'v' && ptr[1] == 't') {
			num_uvs++;
		// normals
		} else if (ptr[0] == 'v' && ptr[1] == 'n') {
			num_normals++;
		// vertices
		} else if (ptr[0] == 'v') {
			num_vertices++;
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

	DEBUG_ASSERT(num_vertices > 0);
	DEBUG_ASSERT(num_indices > 0);
	DEBUG_ASSERT(num_normals > 0);
	DEBUG_ASSERT(num_uvs > 0);

	DEBUG_LOG("-- num_vertices: %d", num_vertices);
	DEBUG_LOG("-- num_indices: %d", num_indices);
	DEBUG_LOG("-- num_normals: %d", num_normals);
	DEBUG_LOG("-- num_uvs: %d", num_uvs);

	mesh.vertices = new Vertex[num_vertices];
	mesh.vertices_count = num_vertices;

	mesh.indices  = new u32[num_indices];
	mesh.indices_count = num_indices;

	Vector3 *normals = new Vector3[num_normals];
	Vector2 *uvs     = new Vector2[num_uvs];

	i32 vertex_index = 0;
	i32 normal_index = 0;
	i32 uv_index     = 0;

	ptr = file;
	while (ptr < end) {
		// texture coordinates
		if (ptr[0] == 'v' && ptr[1] == 't') {
			do ptr++;
			while (ptr[0] == ' ');

			Vector2 uv;
			sscanf(ptr, "%f %f", &uv.x, &uv.y);
			uvs[uv_index++] = uv;
		// normals
		} else if (ptr[0] == 'v' && ptr[1] == 'n') {
			do ptr++;
			while (ptr[0] == ' ');

			Vector3 n;
			sscanf(ptr, "%f %f %f", &n.x, &n.y, &n.z);
			normals[normal_index++] = n;
		// vertices
		} else if (ptr[0] == 'v') {
			do ptr++;
			while (ptr[0] == ' ');

			Vector3 v;
			sscanf(ptr, "%f %f %f", &v.x, &v.y, &v.z);
			mesh.vertices[vertex_index++].vector = v;
		// faces
		}

		do ptr++;
		while (ptr < end && !is_newline(ptr[0]));

		do ptr++;
		while (ptr < end && is_newline(ptr[0]));
	}

	mesh.indices        = new u32[num_indices];
	mesh.indices_count  = num_indices;

	i32 indices_index = 0;

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


	return mesh;
}

