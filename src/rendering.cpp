#include "rendering.h"

#include "gamestate.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include <fstream>
#include <string>
#include <vector>
#include <stdio.h>

#define FOURCC_DXT1 0x31545844 // Equivalent to "DXT1" in ASCII
#define FOURCC_DXT3 0x33545844 // Equivalent to "DXT3" in ASCII
#define FOURCC_DXT5 0x35545844 // Equivalent to "DXT5" in ASCII

GLuint loadDDS(const char * imagepath){

	unsigned char header[124];

	FILE *fp; 
 
	/* try to open the file */ 
	fp = fopen(imagepath, "rb"); 
	if (fp == NULL){
		printf("%s could not be opened. Are you in the right directory ? Don't forget to read the FAQ !\n", imagepath); getchar(); 
		return 0;
	}
   
	/* verify the type of file */ 
	char filecode[4]; 
	fread(filecode, 1, 4, fp); 
	if (strncmp(filecode, "DDS ", 4) != 0) { 
		fclose(fp); 
		return 0; 
	}
	
	/* get the surface desc */ 
	fread(&header, 124, 1, fp); 

	unsigned int height      = *(unsigned int*)&(header[8 ]);
	unsigned int width	     = *(unsigned int*)&(header[12]);
	unsigned int linearSize	 = *(unsigned int*)&(header[16]);
	unsigned int mipMapCount = *(unsigned int*)&(header[24]);
	unsigned int fourCC      = *(unsigned int*)&(header[80]);

 
	unsigned char * buffer;
	unsigned int bufsize;
	/* how big is it going to be including all mipmaps? */ 
	bufsize = mipMapCount > 1 ? linearSize * 2 : linearSize; 
	buffer = (unsigned char*)malloc(bufsize * sizeof(unsigned char)); 
	fread(buffer, 1, bufsize, fp); 
	/* close the file pointer */ 
	fclose(fp);

	unsigned int components  = (fourCC == FOURCC_DXT1) ? 3 : 4; 
	unsigned int format;
	switch(fourCC) 
	{ 
	case FOURCC_DXT1: 
		format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; 
		break; 
	case FOURCC_DXT3: 
		format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; 
		break; 
	case FOURCC_DXT5: 
		format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; 
		break; 
	default: 
		free(buffer); 
		return 0; 
	}

	// Create one OpenGL texture
	GLuint textureID;
	glGenTextures(1, &textureID);

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, textureID);
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);	
	
	unsigned int blockSize = (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16; 
	unsigned int offset = 0;

	/* load the mipmaps */ 
	for (unsigned int level = 0; level < mipMapCount && (width || height); ++level) 
	{ 
		unsigned int size = ((width+3)/4)*((height+3)/4)*blockSize; 
		glCompressedTexImage2D(GL_TEXTURE_2D, level, format, width, height,  
			0, size, buffer + offset); 
	 
		offset += size; 
		width  /= 2; 
		height /= 2; 

		// Deal with Non-Power-Of-Two textures. This code is not included in the webpage to reduce clutter.
		if(width < 1) width = 1;
		if(height < 1) height = 1;

	} 

	free(buffer); 

	return textureID;


}

void generateCube(GLuint* vertexArray, GLuint* vertexBuffer, GLuint* colourBuffer) {
	glGenVertexArrays(1, vertexArray);
	glBindVertexArray(*vertexArray);

	const GLfloat vertexBufferData[] = {
		-1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		 1.0f, 1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f,-1.0f,
		 1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f,-1.0f,
		 1.0f,-1.0f,-1.0f,
		 1.0f, 1.0f,-1.0f,
		 1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f,-1.0f,
		 1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f,-1.0f, 1.0f,
		 1.0f,-1.0f, 1.0f,
		 1.0f, 1.0f, 1.0f,
		 1.0f,-1.0f,-1.0f,
		 1.0f, 1.0f,-1.0f,
		 1.0f,-1.0f,-1.0f,
		 1.0f, 1.0f, 1.0f,
		 1.0f,-1.0f, 1.0f,
		 1.0f, 1.0f, 1.0f,
		 1.0f, 1.0f,-1.0f,
		-1.0f, 1.0f,-1.0f,
		 1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		 1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		 1.0f,-1.0f, 1.0f
	};

	glGenBuffers(1, vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, *vertexBuffer);

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexBufferData), vertexBufferData, GL_STATIC_DRAW);
	
	static const GLfloat colourBufferData[] = {
		0.583f,  0.771f,  0.014f,
		0.609f,  0.115f,  0.436f,
		0.327f,  0.483f,  0.844f,
		0.822f,  0.569f,  0.201f,
		0.435f,  0.602f,  0.223f,
		0.310f,  0.747f,  0.185f,
		0.597f,  0.770f,  0.761f,
		0.559f,  0.436f,  0.730f,
		0.359f,  0.583f,  0.152f,
		0.483f,  0.596f,  0.789f,
		0.559f,  0.861f,  0.639f,
		0.195f,  0.548f,  0.859f,
		0.014f,  0.184f,  0.576f,
		0.771f,  0.328f,  0.970f,
		0.406f,  0.615f,  0.116f,
		0.676f,  0.977f,  0.133f,
		0.971f,  0.572f,  0.833f,
		0.140f,  0.616f,  0.489f,
		0.997f,  0.513f,  0.064f,
		0.945f,  0.719f,  0.592f,
		0.543f,  0.021f,  0.978f,
		0.279f,  0.317f,  0.505f,
		0.167f,  0.620f,  0.077f,
		0.347f,  0.857f,  0.137f,
		0.055f,  0.953f,  0.042f,
		0.714f,  0.505f,  0.345f,
		0.783f,  0.290f,  0.734f,
		0.722f,  0.645f,  0.174f,
		0.302f,  0.455f,  0.848f,
		0.225f,  0.587f,  0.040f,
		0.517f,  0.713f,  0.338f,
		0.053f,  0.959f,  0.120f,
		0.393f,  0.621f,  0.362f,
		0.673f,  0.211f,  0.457f,
		0.820f,  0.883f,  0.371f,
		0.982f,  0.099f,  0.879f
	};

	glGenBuffers(1, colourBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, *colourBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colourBufferData), colourBufferData, GL_STATIC_DRAW);
}

void generateTexturedCube(GLuint texture, GLuint* vertexArray, GLuint* vertexBuffer, GLuint* uvBuffer) {
	glGenVertexArrays(1, vertexArray);
	glBindVertexArray(*vertexArray);

	const GLfloat vertexBufferData[] = {
		-1.0f,-1.0f,-1.0f, 
		-1.0f,-1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		 1.0f, 1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f,-1.0f,
		 1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f,-1.0f,
		 1.0f,-1.0f,-1.0f,
		 1.0f, 1.0f,-1.0f,
		 1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f,-1.0f,
		 1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f,-1.0f, 1.0f,
		 1.0f,-1.0f, 1.0f,
		 1.0f, 1.0f, 1.0f,
		 1.0f,-1.0f,-1.0f,
		 1.0f, 1.0f,-1.0f,
		 1.0f,-1.0f,-1.0f,
		 1.0f, 1.0f, 1.0f,
		 1.0f,-1.0f, 1.0f,
		 1.0f, 1.0f, 1.0f,
		 1.0f, 1.0f,-1.0f,
		-1.0f, 1.0f,-1.0f,
		 1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		 1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		 1.0f,-1.0f, 1.0f
	};

	glGenBuffers(1, vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, *vertexBuffer);

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexBufferData), vertexBufferData, GL_STATIC_DRAW);

	static const GLfloat uvBufferData[] = {
		0.000059f, 1.0f-0.000004f, 
		0.000103f, 1.0f-0.336048f, 
		0.335973f, 1.0f-0.335903f, 
		1.000023f, 1.0f-0.000013f, 
		0.667979f, 1.0f-0.335851f, 
		0.999958f, 1.0f-0.336064f, 
		0.667979f, 1.0f-0.335851f, 
		0.336024f, 1.0f-0.671877f, 
		0.667969f, 1.0f-0.671889f, 
		1.000023f, 1.0f-0.000013f, 
		0.668104f, 1.0f-0.000013f, 
		0.667979f, 1.0f-0.335851f, 
		0.000059f, 1.0f-0.000004f, 
		0.335973f, 1.0f-0.335903f, 
		0.336098f, 1.0f-0.000071f, 
		0.667979f, 1.0f-0.335851f, 
		0.335973f, 1.0f-0.335903f, 
		0.336024f, 1.0f-0.671877f, 
		1.000004f, 1.0f-0.671847f, 
		0.999958f, 1.0f-0.336064f, 
		0.667979f, 1.0f-0.335851f, 
		0.668104f, 1.0f-0.000013f, 
		0.335973f, 1.0f-0.335903f, 
		0.667979f, 1.0f-0.335851f, 
		0.335973f, 1.0f-0.335903f, 
		0.668104f, 1.0f-0.000013f, 
		0.336098f, 1.0f-0.000071f, 
		0.000103f, 1.0f-0.336048f, 
		0.000004f, 1.0f-0.671870f, 
		0.336024f, 1.0f-0.671877f, 
		0.000103f, 1.0f-0.336048f, 
		0.336024f, 1.0f-0.671877f, 
		0.335973f, 1.0f-0.335903f, 
		0.667969f, 1.0f-0.671889f, 
		1.000004f, 1.0f-0.671847f, 
		0.667979f, 1.0f-0.335851f
	};


	glGenBuffers(1, uvBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, *uvBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(uvBufferData), uvBufferData, GL_STATIC_DRAW);

}

void generateTriangle(GLuint* vertexArray, GLuint* vertexBuffer) {
	glGenVertexArrays(1, vertexArray);
	glBindVertexArray(*vertexArray);

	const GLfloat vertexBufferData[] = {
		-1.0f, -1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,
		 0.0f,  1.0f, 0.0f
	};

	glGenBuffers(1, vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, *vertexBuffer);

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexBufferData), vertexBufferData, GL_STATIC_DRAW);
}

bool load_obj_file(const char* file, 
		std::vector<glm::vec3>& out_vertices, 
		std::vector<glm::vec2>& out_uvs, 
		std::vector<glm::vec3>& out_normals
	) {
	
	std::vector<glm::vec3> tmp_vertices;
	std::vector<glm::vec2> tmp_uvs;
	std::vector<glm::vec3> tmp_normals;

	bool normals_enabled = true;

	std::vector<unsigned int> vertex_indices;
	std::vector<unsigned int>     uv_indices;
	std::vector<unsigned int> normal_indices;
	
	FILE* fileptr = fopen(file, "r");
	if (fileptr == NULL) {
		DebugPrintf(EDebugType::WARNING, EDebugCode::OGL_LOAD_OBJ_FILE, "Failed to load object from .obj file: %s", file);
		return false;
	}
	
	while (true) {
		char key[128];
		int result = fscanf(fileptr, "%s", key);

		if (result == EOF)
			break;

		if (strcmp(key, "v") == 0) { 

			glm::vec3 vertex;
			fscanf(fileptr, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			tmp_vertices.push_back(vertex);

		} else if (strcmp(key, "vt") == 0) {

			glm::vec2 uv;
			fscanf(fileptr, "%f %f\n", &uv.x, &uv.y);
			uv.y = 1.0 - uv.y;
			tmp_uvs.push_back(uv);

		} else if (strcmp(key, "vn") == 0) {

			glm::vec3 normal;
			fscanf(fileptr, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			tmp_normals.push_back(normal);

		} else if (strcmp(key, "f") == 0) {
		
			unsigned int vertex_index[3], uv_index[3], normal_index[3];

			if (normals_enabled) {
				int result = fscanf(fileptr, "%u/%u/%u %u/%u/%u %u/%u/%u\n", 
					&vertex_index[0], &uv_index[0], &normal_index[0],
					&vertex_index[1], &uv_index[1], &normal_index[1],
					&vertex_index[2], &uv_index[2], &normal_index[2]
				);

				if (result != 9)
					normals_enabled = false;
			} 
		   
			if (!normals_enabled) {
				result = fscanf(fileptr, "%u/%u %u/%u %u/%u\n", 
					&vertex_index[0], &uv_index[0],
					&vertex_index[1], &uv_index[1],
					&vertex_index[2], &uv_index[2]
				);
			}

			vertex_indices.push_back(vertex_index[0]);
			vertex_indices.push_back(vertex_index[1]);
			vertex_indices.push_back(vertex_index[2]);

			uv_indices.push_back(uv_index[0]);
			uv_indices.push_back(uv_index[1]);
			uv_indices.push_back(uv_index[2]);

			if (normals_enabled) {
				normal_indices.push_back(normal_index[0]);
				normal_indices.push_back(normal_index[1]);
				normal_indices.push_back(normal_index[2]);
			}
		} else {
			// comment or unsupported option
			char buffer[1000];
			fgets(buffer, 1000, fileptr);
		}

	}
	
	for (unsigned int i = 0; i < vertex_indices.size(); i++) {
		
		unsigned int vertex_index = vertex_indices[i];
		unsigned int     uv_index =     uv_indices[i];

		glm::vec3 vertex = tmp_vertices[vertex_index-1];
		glm::vec2 uv     =      tmp_uvs[uv_index-1];
		
		out_vertices.push_back(vertex);
		out_uvs     .push_back(uv);
		
		if (normals_enabled) {
			unsigned int normal_index = normal_indices[i];
			glm::vec3 normal =  tmp_normals[normal_index-1];
			out_normals .push_back(normal);
		}
	}

	return true;		
}

GLuint loadShader(GLenum type, const char* file) {
	GLuint shader = glCreateShader(type);

	std::string shaderSource;
	std::ifstream shaderStream(file, std::ios::in);
	if (shaderStream.is_open()) {
		std::string line = "";
		while (std::getline(shaderStream, line))
			shaderSource += "\n" + line;

		shaderStream.close();
	}

	char const* shaderSourcePtr = shaderSource.c_str();
	glShaderSource(shader, 1, &shaderSourcePtr, NULL);
	glCompileShader(shader);

	GLint result = GL_FALSE;
	int infoLogLength;
	
	glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

	char* infoLog = new char[infoLogLength];
	glGetShaderInfoLog(shader, infoLogLength, NULL, infoLog);
	
	if (result == GL_FALSE)
		DebugPrintf(EDebugType::FATAL, EDebugCode::OGL_SHADER_COMPILE, "Failed to compile shader %s: %s", file, infoLog); 
	else 
		DebugPrintf(EDebugType::MESSAGE, EDebugCode::OGL_SHADER_COMPILE, "Compiled shader %s: %s", file, infoLog);
	
	delete[] infoLog;

	return shader;
}

GLuint loadShaders(const char* vertexFile, const char* fragmentFile) {
	GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexFile); 
	GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentFile); 

	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	GLint result;
	int infoLogLength;
	
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &result);
	glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &infoLogLength);

	char* infoLog = new char[infoLogLength];
	glGetProgramInfoLog(shaderProgram, infoLogLength, NULL, infoLog);
	
	if (result == GL_FALSE) 
		DebugPrintf(EDebugType::FATAL, EDebugCode::OGL_SHADER_LINK, "Failed to link program: %s", infoLog);
	else
		DebugPrintf(EDebugType::MESSAGE, EDebugCode::OGL_SHADER_LINK, "Linked shader program program: %s", infoLog);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	delete[] infoLog;

	return shaderProgram;
}

GLuint loadTexture(const char* file) {
	GLuint texture = 0;

	int width, height, components;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* pixels = stbi_load(file, &width, &height, &components, 0);

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	if (components == 3)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
	else if (components == 4) 
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	else {
		DebugPrintf(EDebugType::ERROR, EDebugCode::OGL_TEXTURE_UNSUPPORTED_COMPONENTS, 
				"Trying to load texture with unsupported number of components: %s. Expected 3 or 4 components but got: %d", 
				file, components);

		return 0;
	}


	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	glGenerateMipmap(GL_TEXTURE_2D);

	stbi_image_free(pixels);

	return texture;
}

Rendering::Rendering() {}
Rendering::~Rendering() {
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &cbo);
	glDeleteProgram(shaderProgram);
	glDeleteVertexArrays(1, &vao);
	glDeleteTextures(1, &samplerLocation);

	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(window);
}


void Rendering::init(GameState* gs) {
	this->game = gs;

	// setup SDL with OpenGL context
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

	window = SDL_CreateWindow(
		game->settings.general.title.c_str(),
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		game->settings.video.resolution_x, game->settings.video.resolution_y,
		SDL_WINDOW_OPENGL
	);

	if (window == NULL)
		DebugPrintf(EDebugType::FATAL, EDebugCode::SDL_INIT_WINDOW, "Failed to initialise SDL Window: %s", SDL_GetError());

	SDL_SetWindowGrab(window, SDL_TRUE);

	
	// OpenGL setup
	camera.aspect_ratio = (float) game->settings.video.resolution_x / (float) game->settings.video.resolution_y;

	glcontext = SDL_GL_CreateContext(window);
	
	glewExperimental = GL_TRUE;
	glewInit();
	
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);


	// load data into GPU
	GLuint vertex_array_object;
	glGenVertexArrays(1, &vertex_array_object);
	glBindVertexArray(vertex_array_object);
	
	this->vao = vertex_array_object; // @TODO @cleanup: consider cleaning up

	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;

	// @TODO @cleanup: move into load_object function
	bool success = load_obj_file("data/objects/crate2.obj", vertices, uvs, normals);
	if (success) {
		texture = loadTexture("data/textures/crate.tga"); // uses stb_image
		
		
		vertices_size = vertices.size();
		
		GLuint vertex_buffer;
		glGenBuffers(1, &vertex_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

		// @TODO @cleanup: store struct of arrays of VBO	
		this->vbo = vertex_buffer;

		GLuint uv_buffer;
		glGenBuffers(1, &uv_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, uv_buffer);
		glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);

		// @TODO @cleanup: store struct of arrays of UVBO	
		this->uvbo = uv_buffer;
	}





	
	
	// load and compile shaders
	// @TODO @cleanup: move hardcoded shader loading into something cleaner and smarter
	shaderProgram = loadShaders("data/shaders/vertex.glsl", "data/shaders/fragment.glsl");
	MVPLocation = glGetUniformLocation(shaderProgram, "MVP");	// get location of the uniform "MVP" in the program
	samplerLocation = glGetUniformLocation(shaderProgram, "textureSampler");

}

void Rendering::render() {
	// Compute camera matrices
	camera.forward = glm::vec3(
		cos(camera.verticalAngle) * sin(camera.horizontalAngle),
		sin(camera.verticalAngle),
		cos(camera.verticalAngle) * cos(camera.horizontalAngle)
	);
	
	camera.right = glm::vec3(
		sin(camera.horizontalAngle - 3.14f/2.0f),
		0,
		cos(camera.horizontalAngle - 3.14f/2.0f)
	);
	
	camera.up = glm::cross(camera.right, camera.forward);

	camera.projection = glm::perspective(
		camera.FoV,
		camera.aspect_ratio, // aspect ratio
		0.1f,                // near clipping plane
		100.0f               // far clipping plane
	);
	
	camera.view = glm::lookAt(
		camera.position, 
		camera.position + camera.forward, 
		camera.up
	);

	MVP = camera.projection * camera.view * glm::mat4(1.0f);
	


	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(shaderProgram);

	// send transforms to shader
	glUniformMatrix4fv(MVPLocation, 1, GL_FALSE, &MVP[0][0]);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glUniform1i(samplerLocation, 0);


	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, uvbo);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glDrawArrays(GL_TRIANGLES, 0, vertices_size);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	SDL_GL_SwapWindow(window);

	glUseProgram(0);
}
