/* $URL$
   $Rev$
   $Author$
   $Date$
   $Id$
 */

#include "vapp.h"
#include "vutils.h"

#include "vmath.h"

#include "vbm.h"
#include "LoadShaders.h"

#include <stdio.h>
#include <string>

#define MAX_FRAMEBUFFER_WIDTH 2048
#define MAX_FRAMEBUFFER_HEIGHT 2048

namespace vtarga
{
extern unsigned char * load_targa(const char * filename, GLenum &format, int &width, int &height);
};

using namespace vmath;

#define FRUSTUM_DEPTH 2000.0f

BEGIN_APP_DECLARATION(OITDemo)
    // Override functions from base class
    virtual void Initialize(const char * title);
    virtual void Display(bool auto_redraw);
    virtual void Finalize(void);
    virtual void Resize(int width, int height);
    virtual void OnKey(int key, int scancode, int action, int mods);

    // Member variables
    float aspect;

    // Program to construct the linked list (renders the transparent objects)
    GLuint  list_build_program;

    // Head pointer image and PBO for clearing it
    GLuint  head_pointer_texture;
    GLuint  head_pointer_clear_buffer;
    // Atomic counter buffer
    GLuint  atomic_counter_buffer;
    // Linked list buffer
    GLuint  linked_list_buffer;
    GLuint  linked_list_texture;

    // Program to render the scene
    GLuint render_scene_prog;
    struct
    {
        GLint aspect;
        GLint time;
        GLint model_matrix;
        GLint view_matrix;
        GLint projection_matrix;
    } render_scene_uniforms;

    // Program to resolve 
    GLuint resolve_program;

    // Full Screen Quad
    GLuint  quad_vbo;
    GLuint  quad_vao;

    GLint current_width;
    GLint current_height;

    VBObject object;

    void DrawScene(void);
    void InitPrograms(void);
END_APP_DECLARATION()

DEFINE_APP(OITDemo, "Order Independent Transparency")

void OITDemo::Initialize(const char * title)
{
    GLuint * data;
    render_scene_prog = -1;

    base::Initialize(title);

    InitPrograms();

    // Create head pointer texture
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &head_pointer_texture);
    glBindTexture(GL_TEXTURE_2D, head_pointer_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, MAX_FRAMEBUFFER_WIDTH, MAX_FRAMEBUFFER_HEIGHT, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindImageTexture(0, head_pointer_texture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);

    // Create buffer for clearing the head pointer texture
    glGenBuffers(1, &head_pointer_clear_buffer);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, head_pointer_clear_buffer);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, MAX_FRAMEBUFFER_WIDTH * MAX_FRAMEBUFFER_HEIGHT * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    data = (GLuint *)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
    memset(data, 0x00, MAX_FRAMEBUFFER_WIDTH * MAX_FRAMEBUFFER_HEIGHT * sizeof(GLuint));
    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

    // Create the atomic counter buffer
    glGenBuffers(1, &atomic_counter_buffer);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomic_counter_buffer);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_COPY);

    // Create the linked list storage buffer
    glGenBuffers(1, &linked_list_buffer);
    glBindBuffer(GL_TEXTURE_BUFFER, linked_list_buffer);
    glBufferData(GL_TEXTURE_BUFFER, MAX_FRAMEBUFFER_WIDTH * MAX_FRAMEBUFFER_HEIGHT * 3 * sizeof(vec4), NULL, GL_DYNAMIC_COPY);
    glBindBuffer(GL_TEXTURE_BUFFER, 0);

    // Bind it to a texture (for use as a TBO)
    glGenTextures(1, &linked_list_texture);
    glBindTexture(GL_TEXTURE_BUFFER, linked_list_texture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32UI, linked_list_buffer);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    glBindImageTexture(1, linked_list_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32UI);

    glGenVertexArrays(1, &quad_vao);
    glBindVertexArray(quad_vao);

    static const GLfloat quad_verts[] =
    {
        -0.0f, -1.0f,
         1.0f, -1.0f,
        -0.0f,  1.0f,
         1.0f,  1.0f,
    };

    glGenBuffers(1, &quad_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_verts), quad_verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);

    glClearDepth(1.0f);

    object.LoadFromVBM("media/unit_pipe.vbm", 0, 1, 2);
}

void OITDemo::InitPrograms()
{
    // Create the program for rendering the scene from the viewer's position
    ShaderInfo scene_shaders[] =
    {
        { GL_VERTEX_SHADER, "media/shaders/oit/build_lists.vs.glsl" },
        { GL_FRAGMENT_SHADER, "media/shaders/oit/build_lists.fs.glsl" },
        { GL_NONE }
    };

    if (render_scene_prog != -1)
        glDeleteProgram(render_scene_prog);

    render_scene_prog = LoadShaders(scene_shaders);

    render_scene_uniforms.model_matrix = glGetUniformLocation(render_scene_prog, "model_matrix");
    render_scene_uniforms.view_matrix = glGetUniformLocation(render_scene_prog, "view_matrix");
    render_scene_uniforms.projection_matrix = glGetUniformLocation(render_scene_prog, "projection_matrix");
    render_scene_uniforms.aspect = glGetUniformLocation(render_scene_prog, "aspect");
    render_scene_uniforms.time = glGetUniformLocation(render_scene_prog, "time");

    ShaderInfo resolve_shaders[] =
    {
        { GL_VERTEX_SHADER, "media/shaders/oit/resolve_lists.vs.glsl" },
        { GL_FRAGMENT_SHADER, "media/shaders/oit/resolve_lists.fs.glsl" },
        { GL_NONE }
    };

    resolve_program = LoadShaders(resolve_shaders);
}

void OITDemo::Display(bool auto_redraw)
{
    float t;

    unsigned int current_time = app_time();

    t = (float)(current_time & 0xFFFFF) / (float)0x3FFF;

    GLuint * data;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Reset atomic counter
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, atomic_counter_buffer);
    data = (GLuint *)glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_WRITE_ONLY);
    data[0] = 0;
    glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);

    // Clear head-pointer image
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, head_pointer_clear_buffer);
    glBindTexture(GL_TEXTURE_2D, head_pointer_texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, current_width, current_height, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Bind head-pointer image for read-write
    glBindImageTexture(0, head_pointer_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

    // Bind linked-list buffer for write
    glBindImageTexture(1, linked_list_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32UI);

    glUseProgram(render_scene_prog);

    vmath::mat4 model_matrix = vmath::translate(0.0f, 0.0f, -20.0f) *
                               vmath::rotate(t * 360.0f, 0.0f, 0.0f, 1.0f) *
                               vmath::rotate(t * 435.0f, 0.0f, 1.0f, 0.0f) *
                               vmath::rotate(t * 275.0f, 1.0f, 0.0f, 0.0f);
    vmath::mat4 view_matrix = vmath::mat4::identity();
    vmath::mat4 projection_matrix = vmath::frustum(-1.0f, 1.0f, aspect, -aspect, 1.0f, 40.f);

    glUniformMatrix4fv(render_scene_uniforms.model_matrix, 1, GL_FALSE, model_matrix);
    glUniformMatrix4fv(render_scene_uniforms.view_matrix, 1, GL_FALSE, view_matrix);
    glUniformMatrix4fv(render_scene_uniforms.projection_matrix, 1, GL_FALSE, projection_matrix);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    object.Render(0, 8 * 8 * 8);

    glDisable(GL_BLEND);

    glBindVertexArray(quad_vao);
    glUseProgram(resolve_program);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Done
    base::Display();
}

void OITDemo::DrawScene(void)
{
    // Draw the object
    glBindVertexArray(quad_vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void OITDemo::Finalize(void)
{
    glUseProgram(0);
    glDeleteProgram(render_scene_prog);
    glDeleteBuffers(1, &quad_vbo);
    glDeleteVertexArrays(1, &quad_vao);
}

void OITDemo::Resize(int width, int height)
{
    current_width = width;
    current_height = height;

    aspect = float(height) / float(width);
    glViewport(0, 0, current_width, current_height);
}

void OITDemo::OnKey(int key, int scancode, int action, int mods)
{
    switch (key)
    {
        case 'r': InitPrograms();
            break;
        default:
            break;
    }
}
