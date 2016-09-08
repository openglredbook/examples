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

BEGIN_APP_DECLARATION(OverdrawCountExample)
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

    // Color palette buffer texture
    GLuint  image_palette_buffer;
    GLuint  image_palette_texture;

    // Output image and PBO for clearing it
    GLuint  overdraw_count_buffer;
    GLuint  overdraw_count_clear_buffer;

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

DEFINE_APP(OverdrawCountExample, "Overdraw Count Example")

void OverdrawCountExample::Initialize(const char * title)
{
    render_scene_prog = -1;

    base::Initialize(title);

    const GLubyte * vendor = glGetString(GL_VENDOR);
    const GLubyte * renderer = glGetString(GL_RENDERER);
    const GLubyte * version = glGetString(GL_VERSION);

    // Assemble a message
    std::string message = std::string("Created debug context with ") +
                          std::string((const char*)vendor) + std::string(" ") +
                          std::string((const char*)renderer) +
                          std::string(". The OpenGL version is ") +
                          std::string((const char*)version) + std::string(".");

    // Send the message to the debug output log
    glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION,
                         GL_DEBUG_TYPE_MARKER,
                         0x4752415A,
                         GL_DEBUG_SEVERITY_NOTIFICATION,
                         0,
                         message.c_str());

    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0x4752415A, -1, "Initialization");

    InitPrograms();

    // glPopDebugGroup();

    // Create palette texture
    glGenBuffers(1, &image_palette_buffer);
    glBindBuffer(GL_TEXTURE_BUFFER, image_palette_buffer);
    glBufferData(GL_TEXTURE_BUFFER, 256 * 4 * sizeof(float), NULL, GL_STATIC_DRAW);
    glGenTextures(1, &image_palette_texture);
    glBindTexture(GL_TEXTURE_BUFFER, image_palette_texture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, image_palette_buffer);

    vmath::vec4 * data = (vmath::vec4 *)glMapBuffer(GL_TEXTURE_BUFFER, GL_WRITE_ONLY);
    for (int i = 0; i < 256; i++)
    {
        data[i] = vmath::vec4((float)i);
    }
    glUnmapBuffer(GL_TEXTURE_BUFFER);

    // Create overdraw counter texture
    glGenTextures(1, &overdraw_count_buffer);
    glBindTexture(GL_TEXTURE_2D, overdraw_count_buffer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, MAX_FRAMEBUFFER_WIDTH, MAX_FRAMEBUFFER_HEIGHT, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Create buffer for clearing the head pointer texture
    glGenBuffers(1, &overdraw_count_clear_buffer);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, overdraw_count_clear_buffer);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, MAX_FRAMEBUFFER_WIDTH * MAX_FRAMEBUFFER_HEIGHT * sizeof(GLuint), NULL, GL_STATIC_DRAW);

    data = (vmath::vec4 *)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
    memset(data, 0x00, MAX_FRAMEBUFFER_WIDTH * MAX_FRAMEBUFFER_HEIGHT * sizeof(GLuint));
    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

    // Create VAO containing quad for the final blit
    glGenVertexArrays(1, &quad_vao);
    glBindVertexArray(quad_vao);

    static const GLfloat quad_verts[] =
    {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
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

void OverdrawCountExample::InitPrograms()
{
    // Create the program for rendering the scene from the viewer's position
    ShaderInfo scene_shaders[] =
    {
        { GL_VERTEX_SHADER, "media/shaders/overdrawcount/overdraw_count.vs.glsl" },
        { GL_FRAGMENT_SHADER, "media/shaders/overdrawcount/overdraw_count.fs.glsl" },
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
        { GL_VERTEX_SHADER, "media/shaders/overdrawcount/blit.vs.glsl" },
        { GL_FRAGMENT_SHADER, "media/shaders/overdrawcount/blit.fs.glsl" },
        { GL_NONE }
    };

    resolve_program = LoadShaders(resolve_shaders);
}

void OverdrawCountExample::Display(bool auto_redraw)
{
    float t;

    unsigned int current_time = app_time();

    t = (float)(current_time & 0xFFFFF) / (float)0x3FFF;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Clear output image
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, overdraw_count_clear_buffer);
    glBindTexture(GL_TEXTURE_2D, overdraw_count_buffer);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, current_width, current_height, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Bind output image for read-write
    glBindImageTexture(0, overdraw_count_buffer, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

    // Render
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

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    object.Render(0, 8 * 8 * 8);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glBindVertexArray(quad_vao);
    glUseProgram(resolve_program);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Done
    base::Display();
}

void OverdrawCountExample::DrawScene(void)
{
    // Draw the object
    glBindVertexArray(quad_vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void OverdrawCountExample::Finalize(void)
{
    glUseProgram(0);
    glDeleteProgram(render_scene_prog);
    glDeleteBuffers(1, &quad_vbo);
    glDeleteVertexArrays(1, &quad_vao);
}

void OverdrawCountExample::Resize(int width, int height)
{
    current_width = width;
    current_height = height;

    aspect = float(height) / float(width);
    glViewport(0, 0, current_width, current_height);
}

void OverdrawCountExample::OnKey(int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        switch (key)
        {
            case GLFW_KEY_R:
                InitPrograms();
                return;
        }
    }

    base::OnKey(key, scancode, action, mods);
}
