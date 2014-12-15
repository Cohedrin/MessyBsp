/*
    MessyBsp. BSP collision and loading example code.
    Copyright (C) 2014 Richard Maxwell <jodi.the.tigger@gmail.com>
    This file is part of MessyBsp
    MessyBsp is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Affero General Public License for more details.
    You should have received a copy of the GNU Affero General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>
*/

#include "Bsp.hpp"
#include "TraceTest.hpp"
#include <cstdlib>

#include <cstdio>

// SDL + OpenGL
#include <SDL2/SDL.h>

// Need ifdef for different platforms.
#include <GL/glew.h>

#include "third-party/getopt/getopt.h"

void DoGraphics(const Bsp::CollisionBsp& bsp);

int main(int argc, char** argv)
{
    bool benchmark = false;

    // Parse options
    while (auto ch = getopt(argc, argv, "hb"))
    {
        if (ch < 0)
        {
            break;
        }

        if (ch == 'h')
        {
            printf("MessyBsp - By Richard Maxwell\n\n");

            printf("  Renders 'final.bsp' or does a collsion detection bnechmark.\n\n");

            printf("  MessyBsp [--benchmark] [--help]\n\n");

            printf("  -b:  Benchmark 1,000,000 random collision tests\n");
            printf("       against 'final.bsp'. Prints the cost in Microseconds.\n\n");

            printf("  -h:  This help text.\n");
            printf("\n");

            return 0;
        }

        if (ch == 'b')
        {
            benchmark = true;
            break;
        }
    }

    Bsp::CollisionBsp bsp;

    Bsp::GetCollisionBsp("final.bsp", bsp);

    if (benchmark)
    {
        auto result = TimeBspCollision(bsp, 1000);//1000000);

        printf("Trace Took %ld microseconds\n", result.count());

        return 0;
    }

    // RAM: sdl testing.
    DoGraphics(bsp);

    return 0;
}

std::vector<float> MakeTrianglesAndNormals()
{
    return std::vector<float>
    {
        // x,y,z,nx,ny,nz (RH coords, z comes out of monitor)
        0.0f,
        0.0f,
        0.0f,

        0.0f,
        0.0f,
        1.0f,


        5.0f,
        10.0f,
        0.0f,

        0.0f,
        0.0f,
        1.0f,


        10.0f,
        0.0f,
        0.0f,

        0.0f,
        0.0f,
        1.0f,
    };
}

// RAM: Lets try loaind sdl and get a glcontext.
void DoGraphics(const Bsp::CollisionBsp &)
{
    SDL_Init(SDL_INIT_VIDEO);

    // Window mode MUST include SDL_WINDOW_OPENGL for use with OpenGL.
    // FFS. I wan't scoped_exit!
    SDL_Window *window = SDL_CreateWindow(
        "SDL2/OpenGL Demo", 0, 0, 640, 480,
        SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);

    // Create an OpenGL context associated with the window.
    SDL_GLContext glcontext = SDL_GL_CreateContext(window);

    SDL_GL_MakeCurrent(window, glcontext);

    // Now I can init glew.
    {
        glewExperimental = GL_TRUE;

        GLenum err = glewInit();
        if (GLEW_OK != err)
        {
            /* Problem: glewInit failed, something is seriously wrong. */
            fprintf(stderr, "Error: %s\n", glewGetErrorString(err));

        }

        fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
    }

    // TODO: Load vertex data, gen normal data
    // TODO: player controller
    // TODO: use opengl debug function binding
    // TODO: breakpoint at debug error

    // Loading vertex data (1 == number of buffers)
    // http://en.wikipedia.org/wiki/Vertex_Buffer_Object
    GLuint triangleVboHandle;
    glGenBuffers(1, &triangleVboHandle);
    glBindBuffer(GL_ARRAY_BUFFER, triangleVboHandle);

    auto triangles = MakeTrianglesAndNormals();
    glBufferData(
        GL_ARRAY_BUFFER,
        triangles.size(),
        triangles.data(),
        GL_STATIC_DRAW);

    // Since I'm not changing state much, just setup here.
    glVertexPointer(3, GL_FLOAT, 0, nullptr);

    // TODO: vertex, fragment, program, bind, load
    // try https://www.opengl.org/sdk/docs/tutorials/ClockworkCoders/loading.php
    // http://classes.soe.ucsc.edu/cmps162/Spring12/s12/labnotes/shaders.html
    // FFS I cannot find a simple tutorial!
    // try https://www.khronos.org/webgl/wiki/Tutorial
    // normals are transformed differently, ugh.
    // http://www.songho.ca/opengl/gl_normaltransform.html
//    Glchar* vs = "
//            uniform mat4 mvp;
//            attribute vec3 position;
//            attribute vec3 normal;

//            varying vec4 colour;
//            varying vec4 normal;

//            void main() {
//                gl_Position = mvp * vec4(position, 1.0);

//                Vec4 color = Vec4(pos, 1.0)
//            }
    const GLchar* vs = "\
    uniform mat4 u_modelViewProjMatrix;\
    uniform mat4 u_normalMatrix;\
    uniform vec3 lightDir;\
 \
    attribute vec3 vNormal;\
    attribute vec4 vPosition;\
 \
    varying float v_Dot;\
 \
    void main()\
    {\
        gl_Position = u_modelViewProjMatrix * vPosition;\
        vec4 transNormal = u_normalMatrix * vec4(vNormal, 1);\
        v_Dot = max(dot(transNormal.xyz, lightDir), 0.0);\
    }";

    const GLchar* ps = "\
    varying float v_Dot; \
    void main()\
    {\
        vec4 c = (0.1, 0.1, 1.0, 1.0);\
        \
        vec2 texCoord = vec2(v_texCoord.s, 1.0 - v_texCoord.t);\
        vec4 color = texture2D(sampler2d, texCoord);\
        color += vec4(0.1, 0.1, 0.1, 1);\
        gl_FragColor = c * v_Dot;\
    }";

    auto vsO = glCreateShader(GL_VERTEX_SHADER);
    auto psO = glCreateShader(GL_FRAGMENT_SHADER);
    auto pO = glCreateProgram();
    glShaderSource(vsO, 1, vs, nullptr);
    glShaderSource(psO, 1, ps, nullptr);
    glCompileShader(vsO);
    glCompileShader(psO);
    glAttachShader(pO, vsO);
    glAttachShader(pO, psO);
    glLinkProgram(pO);

    // MAIN SDL LOOP
    bool running = true;
    while (running)
    {
        SDL_Event e;

        if (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                running = false;
            }

            if (e.type == SDL_KEYUP)
            {
                if (e.key.keysym.sym == SDLK_ESCAPE)
                {
                    running = false;
                }
            }
        }

        // now you can make GL calls.
        glClearColor(0,1,0,1);
        glClear(GL_COLOR_BUFFER_BIT);

        // glEnableVertexAttribArray
        // glVertexAttribPointer
        // glEnableClientState ??
        // glDrawArrays

        SDL_GL_SwapWindow(window);
    }


    // Once finished with OpenGL functions, the SDL_GLContext can be deleted.
    SDL_GL_DeleteContext(glcontext);

    // Close and destroy the window
    SDL_DestroyWindow(window);

    // Clean up
    SDL_Quit();
}
