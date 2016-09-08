/* $URL$
   $Rev$
   $Author$
   $Date$
   $Id$
 */

#include "vapp.h"
#include "vutils.h"

#include "vmath.h"

#include "LoadShaders.h"

#include <stdio.h>

using namespace vmath;

// Define USE_PRIMITIVE_RESTART to 0 to use two separate draw commands
#define USE_PRIMITIVE_RESTART 1

BEGIN_APP_DECLARATION(PrimitiveRestartExample)
    // Override functions from base class
    virtual void Initialize(const char * title);
    virtual void Display(bool auto_redraw);
    virtual void Finalize(void);
    virtual void Resize(int width, int height);

    // Member variables
    float aspect;
    GLuint render_prog;
    GLuint vao[1];
    GLuint vbo[1];
    GLuint ebo[1];

    GLint render_model_matrix_loc;
    GLint render_projection_matrix_loc;
END_APP_DECLARATION()

DEFINE_APP(PrimitiveRestartExample, "Primitive Restart Example")

void PrimitiveRestartExample::Initialize(const char * title)
{
    base::Initialize(title);

    static ShaderInfo shader_info[] =
    {
        { GL_VERTEX_SHADER, "media/shaders/primitive_restart/primitive_restart.vs.glsl" },
        { GL_FRAGMENT_SHADER, "media/shaders/primitive_restart/primitive_restart.fs.glsl" },
        { GL_NONE, NULL }
    };

    render_prog = LoadShaders(shader_info);

    glUseProgram(render_prog);

    // "model_matrix" is actually an array of 4 matrices
    render_model_matrix_loc = glGetUniformLocation(render_prog, "model_matrix");
    render_projection_matrix_loc = glGetUniformLocation(render_prog, "projection_matrix");

    // 8 corners of a cube, side length 2, centered on the origin
    static const GLfloat cube_positions[] =
    {
        -1.0f, -1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f,  1.0f, 1.0f,
        -1.0f,  1.0f, -1.0f, 1.0f,
        -1.0f,  1.0f,  1.0f, 1.0f,
         1.0f, -1.0f, -1.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 1.0f,
         1.0f,  1.0f, -1.0f, 1.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    // Color for each vertex
    static const GLfloat cube_colors[] =
    {
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 1.0f,
        0.5f, 0.5f, 0.5f, 1.0f
    };

    // Indices for the triangle strips
    static const GLushort cube_indices[] =
    {
        0, 1, 2, 3, 6, 7, 4, 5,         // First strip
        0xFFFF,                         // <<-- This is the restart index
        2, 6, 0, 4, 1, 5, 3, 7          // Second strip
    };

    // Set up the element array buffer
    glGenBuffers(1, ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indices), cube_indices, GL_STATIC_DRAW);

    // Set up the vertex attributes
    glGenVertexArrays(1, vao);
    glBindVertexArray(vao[0]);

    glGenBuffers(1, vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_positions) + sizeof(cube_colors), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(cube_positions), cube_positions);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(cube_positions), sizeof(cube_colors), cube_colors);

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (const GLvoid *)sizeof(cube_positions));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

void PrimitiveRestartExample::Display(bool auto_redraw)
{
    float t = float(app_time() & 0x1FFF) / float(0x1FFF);
    static float q = 0.0f;
    static const vmath::vec3 X(1.0f, 0.0f, 0.0f);
    static const vmath::vec3 Y(0.0f, 1.0f, 0.0f);
    static const vmath::vec3 Z(0.0f, 0.0f, 1.0f);

    // Setup
    glEnable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Activate simple shading program
    glUseProgram(render_prog);

    // Set up the model and projection matrix
    mat4 model_matrix(translate(0.0f, 0.0f, -5.0f) * rotate(t * 360.0f, Y) * rotate(t * 720.0f, Z));
    vmath::mat4 projection_matrix(vmath::frustum(-1.0f, 1.0f, -aspect, aspect, 1.0f, 500.0f));

    glUniformMatrix4fv(render_model_matrix_loc, 1, GL_FALSE, model_matrix);
    glUniformMatrix4fv(render_projection_matrix_loc, 1, GL_FALSE, projection_matrix);

    // Set up for a glDrawElements call
    glBindVertexArray(vao[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo[0]);

#if USE_PRIMITIVE_RESTART
    // When primitive restart is on, we can call one draw command
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(0xFFFF);
    glDrawElements(GL_TRIANGLE_STRIP, 17, GL_UNSIGNED_SHORT, NULL);
#else
    // Without primitive restart, we need to call two draw commands
    glDrawElements(GL_TRIANGLE_STRIP, 8, GL_UNSIGNED_SHORT, NULL);
    glDrawElements(GL_TRIANGLE_STRIP, 8, GL_UNSIGNED_SHORT, (const GLvoid *)(9 * sizeof(GLushort)));
#endif

    base::Display();
}

void PrimitiveRestartExample::Finalize(void)
{
    glUseProgram(0);
    glDeleteProgram(render_prog);
    glDeleteVertexArrays(1, vao);
    glDeleteBuffers(1, vbo);
}

void PrimitiveRestartExample::Resize(int width, int height)
{
    glViewport(0, 0 , width, height);

    aspect = float(height) / float(width);
}
