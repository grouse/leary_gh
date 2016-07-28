#include "opengl_device.h"

#include "glm/gtc/matrix_transform.hpp"

#include "render/game_window.h"
#include "core/settings.h"
#include "util/debug.h"

void OpenGLDevice::create(GameWindow& window) {
    m_window = &window;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    m_context = SDL_GL_CreateContext(window.getWindow());
    LEARY_ASSERT_PRINTF(m_context != nullptr,
                        "Failed to create OpenGL context: %s", SDL_GetError());

    glewExperimental = GL_TRUE;
    glewInit();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);


    Settings *settings = Settings::get();

    // initialise camera
    const float vertical_fov = glm::radians(45.0f);
    const float aspect_ratio = static_cast<float>(settings->video.resolution.width) /
                               static_cast<float>(settings->video.resolution.height);

    const float near_plane   = 0.1f;
    const float far_plane    = 100.0f;

    camera.projection = glm::perspective(vertical_fov, aspect_ratio,
                                         near_plane, far_plane);

    camera.position = glm::vec3(4.0f, 3.0f, 3.0f);
    camera.forward  = glm::vec3(0.0f, 0.0f, 0.0f);
    camera.up       = glm::vec3(0.0f, 1.0f, 0.0f);

    camera.view = glm::lookAt(camera.position, camera.forward, camera.up);

    // initialise vao
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // create models
    static const float triangle[] = {
        -1.0f, -1.0f,  0.0f,
         1.0f, -1.0f,  0.0f,
         1.0f,  1.0f,  0.0f,
    };

    test_mesh.num_vertices = 3;
    test_mesh.data         = malloc(3*3*sizeof(float));
    memcpy(test_mesh.data, triangle, 3*3*sizeof(float));

    glGenBuffers(1, &test_mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, test_mesh.vbo);

    const size_t data_size = 3 * test_mesh.num_vertices * sizeof(float);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(data_size),
                 test_mesh.data,
                 GL_STATIC_DRAW);

    // create shaders & program
    OpenGLShader shaders[2];
    shaders[0].create(GL_VERTEX_SHADER, "vertex.glsl");
    shaders[1].create(GL_FRAGMENT_SHADER, "fragment.glsl");

    m_program.create(shaders, 2);

    shaders[0].destroy();
    shaders[1].destroy();
}

void OpenGLDevice::destroy() {
   m_program.destroy();
   free(test_mesh.data);

    SDL_GL_DeleteContext(m_context);
}

void OpenGLDevice::present()
{
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(m_program.id);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, test_mesh.vbo);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 mvp   = camera.projection * camera.view * model;

    glUniformMatrix4fv(m_program.mvp_location, 1, GL_FALSE, &mvp[0][0]);

    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(test_mesh.num_vertices));

    glDisableVertexAttribArray(0);

    m_window->swap();
}
