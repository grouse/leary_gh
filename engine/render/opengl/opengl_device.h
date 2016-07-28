#ifndef LEARY_OPENGL_DEVICE_H
#define LEARY_OPENGL_DEVICE_H

#include <SDL.h>

#include <GL/glew.h>
#include <SDL_opengl.h>

#include "glm/glm.hpp"

#include "opengl_shader.h"
#include "core/ecs.h"

class GameWindow;

class OpenGLDevice {
public:
    void create(GameWindow& window);
    void destroy();

    void present();

private:
    GameWindow*   m_window;
    SDL_GLContext m_context;

    // @TODO: move to appropriate places

    GLuint vao;
    Mesh test_mesh;

    OpenGLProgram m_program;

    struct {
        glm::mat4 projection;
        glm::mat4 view;

        glm::vec3 position;
        glm::vec3 forward;
        glm::vec3 up;
    } camera;
};

#endif // LEARY_OPENGL_DEVICE_H
