/**
 * file:    mesh.cpp
 * created: 2017-03-01
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

struct Vertex {
	Vector3 vector;
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

	i32 num_vertices = 0;
	i32 num_faces = 0;
	i32 num_uvs = 0;
	i32 line = 0;

	char *ptr = file;
	while (ptr < end) {
		if (ptr[0] == 'v' && ptr[1] == 't') {
			num_uvs++;
		} else if (ptr[0] == 'v') {
			num_vertices++;
		} else if (ptr[0] == 'f') {
			i32 num_points = 0;
			char *face = ptr;
			while (face < end && !is_newline(face[0])) {
				if (face[0] == '/') {
					num_points++;
				}

				face++;
			}

			// NOTE(jesper): wtf?
			DEBUG_ASSERT(num_points == 3 || num_points == 4);

			if (num_points == 3) {
				mesh.indices_count += 3;
			} else if (num_points == 4) {
				mesh.indices_count += 6;
			}

			num_faces++;
		}

		do ptr++;
		while (ptr < end && !is_newline(ptr[0]));

		do ptr++;
		while (ptr < end && is_newline(ptr[0]));

		line++;
	}

	mesh.vertices       = new Vertex[num_vertices];
	mesh.indices        = new i32[mesh.indices_count];

	mesh.vertices_count = num_vertices;

	Vector2 *uvs = new Vector2[num_uvs];
	i32 uv_index = 0;
	i32 vertex_index = 0;

	ptr = file;
	while (ptr < end) {
		if (ptr[0] == 'v' && ptr[1] == 't') {
			f32 u, v;
			i32 dummy;
			sscanf(ptr, "%f %f %d", &u, &v, &dummy);
			DEBUG_ASSERT(dummy == 0); // what's this?

			uvs[uv_index++] = {u, v};
		} else if (ptr[0] == 'v') {
			f32 x, y, z;
			sscanf(ptr, "%f %f %f", &x, &y, &z);

			Vertex vertex = {};
			vertex.vector = { x, y, z };

			mesh.vertices[vertex_index++] = vertex;
		}

		do ptr++;
		while (ptr < end && !is_newline(ptr[0]));

		do ptr++;
		while (ptr < end && is_newline(ptr[0]));
	}

	i32 index_index = 0;

	ptr = file;
	while (ptr < end) {
		if (ptr[0] == 'f') {
			i32 num_points = 0;
			char *face = ptr;
			while (face < end && !is_newline(face[0])) {
				if (face[0] == '/') {
					num_points++;
				}
				face++;
			}

			// NOTE(jesper): wtf?
			DEBUG_ASSERT(num_points == 3 || num_points == 4);

			if (num_points == 3) {
				i32 i0, t0;
				i32 i1, t1;
				i32 i2, t2;

				sscanf(ptr, "%d/%d %d/%d %d/%d", &i0, &t0, &i1, &t1, &i2, &t2);
				mesh.indices[index_index++] = i0;
				mesh.indices[index_index++] = i1;
				mesh.indices[index_index++] = i2;

				mesh.vertices[i0].uv = uvs[t0];
				mesh.vertices[i1].uv = uvs[t1];
				mesh.vertices[i2].uv = uvs[t2];
			} else if (num_points == 4) {
				i32 i0, t0;
				i32 i1, t1;
				i32 i2, t2;
				i32 i3, t3;

				sscanf(ptr, "%d/%d %d/%d %d/%d %d/%d",
				       &i0, &t0, &i1, &t1, &i2, &t2, &i3, &t3);

				mesh.indices[index_index++] = i0;
				mesh.indices[index_index++] = i1;
				mesh.indices[index_index++] = i2;

				mesh.indices[index_index++] = i2;
				mesh.indices[index_index++] = i3;
				mesh.indices[index_index++] = i0;

				mesh.vertices[i0].uv = uvs[t0];
				mesh.vertices[i1].uv = uvs[t1];
				mesh.vertices[i2].uv = uvs[t2];
				mesh.vertices[i3].uv = uvs[t3];
			}
		}

		do ptr++;
		while (ptr < end && !is_newline(ptr[0]));

		do ptr++;
		while (ptr < end && is_newline(ptr[0]));
	}

	DEBUG_LOG("num_vertices: %d", num_vertices);
	DEBUG_LOG("num_faces: %d", num_faces);
	DEBUG_LOG("num_uvs: %d", num_uvs);

	return mesh;
}

