/*
 * Copyright © 2012-2015 Graham Sellers
 *
 * This code is part of the OpenGL Programming Guide, 9th Edition.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include <vermilion.h>

#include <iostream>

#include "vapp.h"
#include "mat.h"
#include "LoadShaders.h"
#include "Shapes/Teapot.h"

using namespace std;

GLuint  PLoc;  // Projection matrix
GLuint  InnerLoc;  // Inner tessellation paramter
GLuint  OuterLoc;  // Outer tessellation paramter

GLfloat  Inner = 10.0f;
GLfloat  Outer = 10.0f;

//----------------------------------------------------------------------------

BEGIN_APP_DECLARATION(TeapotExample)
    virtual void Initialize(const char * title);
    virtual void Display(bool auto_redraw);
    virtual void Finalize(void);
    virtual void Resize(int width, int height);
    virtual void OnKey(int key, int scancode, int action, int mods);

    float aspect;
END_APP_DECLARATION()

DEFINE_APP(TeapotExample, "Teapot Rendering")

void TeapotExample::Initialize(const char * title)
{
    base::Initialize(title);

    // Create a vertex array object
    GLuint vao;
    glGenVertexArrays( 1, &vao );
    glBindVertexArray( vao );

    // Create and initialize a buffer object
    enum { ArrayBuffer, ElementBuffer, NumVertexBuffers };
    GLuint buffers[NumVertexBuffers];
    glGenBuffers( NumVertexBuffers, buffers );
    glBindBuffer( GL_ARRAY_BUFFER, buffers[ArrayBuffer] );
    glBufferData( GL_ARRAY_BUFFER, sizeof(TeapotVertices),
                  TeapotVertices, GL_STATIC_DRAW );

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, buffers[ElementBuffer] );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof(TeapotIndices),
                  TeapotIndices, GL_STATIC_DRAW );

    // Load shaders and use the resulting shader program
    ShaderInfo  shaders[] = {
        { GL_VERTEX_SHADER,          "media/shaders/teapot/teapot.vert" },
        { GL_TESS_CONTROL_SHADER,    "media/shaders/teapot/teapot.cont" },
        { GL_TESS_EVALUATION_SHADER, "media/shaders/teapot/teapot.eval" },
        { GL_FRAGMENT_SHADER,        "media/shaders/teapot/teapot.frag" },
        { GL_NONE, NULL }
    };

    GLuint program = LoadShaders( shaders );
    glUseProgram( program );

    // set up vertex arrays
    GLuint vPosition = glGetAttribLocation( program, "vPosition" );
    glEnableVertexAttribArray( vPosition );
    glVertexAttribPointer( vPosition, 3, GL_FLOAT, GL_FALSE, 0,
                           BUFFER_OFFSET(0) );

    PLoc = glGetUniformLocation( program, "P" );
    InnerLoc = glGetUniformLocation( program, "Inner" );
    OuterLoc = glGetUniformLocation( program, "Outer" );

    glUniform1f( InnerLoc, Inner );
    glUniform1f( OuterLoc, Outer );
    
        mat4  modelview = Translate( -0.2625f, -1.575f, -1.0f );
        modelview *= Translate( 0.0f, 0.0f, -7.5f );
    glUniformMatrix4fv( glGetUniformLocation( program, "MV" ),
                        1, GL_TRUE, modelview );

    glPatchParameteri( GL_PATCH_VERTICES, NumTeapotVerticesPerPatch );
    
    glClearColor( 0.0, 0.0, 0.0, 1.0 );
}

//----------------------------------------------------------------------------

void TeapotExample::Display(bool auto_redraw)
{
    mat4  projection = Perspective( 60.0, aspect, 5, 10 );
    glUniformMatrix4fv( PLoc, 1, GL_TRUE, projection );

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glDrawElements( GL_PATCHES, NumTeapotVertices,
                    GL_UNSIGNED_INT, BUFFER_OFFSET(0) );

    base::Display();
}

//----------------------------------------------------------------------------

void TeapotExample::Resize(int width, int height)
{    
    glViewport( 0, 0, width, height );

    aspect = GLfloat(width) / height;
}

//----------------------------------------------------------------------------

void TeapotExample::Finalize(void)
{
}

void TeapotExample::OnKey(int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        switch (key)
        {
            case GLFW_KEY_K: 
                Inner--;
                if ( Inner < 1.0 )  Inner = 1.0;
                glUniform1f( InnerLoc, Inner );
                break;

            case GLFW_KEY_I:
                Inner++;
                if ( Inner > 64 )  Inner = 64.0;
                glUniform1f( InnerLoc, Inner );
                break;

            case GLFW_KEY_L: 
                Outer--;
                if ( Outer < 1.0 )  Outer = 1.0;
                glUniform1f( OuterLoc, Outer );
                break;

            case GLFW_KEY_O:
                Outer++;
                if ( Outer > 64 )  Outer = 64.0;
                glUniform1f( OuterLoc, Outer );
                break;

            case GLFW_KEY_R:
                Inner = 1.0;
                Outer = 1.0;
                glUniform1f( InnerLoc, Inner );
                glUniform1f( OuterLoc, Outer );
                break;

            case GLFW_KEY_M: {
                static GLenum mode = GL_LINE;
                mode = ( mode == GL_FILL ? GL_LINE : GL_FILL );
                glPolygonMode( GL_FRONT_AND_BACK, mode );
            } break;
        }
    }
}
