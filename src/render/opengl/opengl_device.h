/**
 * @file:   opengl_device.h
 * @author: Jesper Stefansson (grouse)
 * @email:  jesper.stefansson@gmail.com
 *
 * Copyright (c) 2015-2016 Jesper Stefansson
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgement in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

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
