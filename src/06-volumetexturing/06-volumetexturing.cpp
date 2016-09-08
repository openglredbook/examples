/* $URL$
   $Rev$
   $Author$
   $Date$
   $Id$
 */

#define USE_GL3W
#include <vermilion.h>

#include "vapp.h"
#include "vutils.h"
#include "vbm.h"

#include "vmath.h"

#include <stdio.h>

BEGIN_APP_DECLARATION(VolumeTextureExample)
    // Override functions from base class
    virtual void Initialize(const char * title);
    virtual void Display(bool auto_redraw);
    virtual void Finalize(void);
    virtual void Resize(int width, int height);

    // Member variables
    float aspect;
    GLuint base_prog;
    GLuint vao;

    GLuint quad_vbo;

    GLuint tex;
    GLint tc_rotate_loc;
END_APP_DECLARATION()

DEFINE_APP(VolumeTextureExample, "Volume Texture Example")

void VolumeTextureExample::Initialize(const char * title)
{
    base::Initialize(title);

    base_prog = glCreateProgram();

    static const char quad_shader_vs[] =
        "#version 330 core\n"
        "\n"
        "layout (location = 0) in vec2 in_position;\n"
        "layout (location = 1) in vec2 in_tex_coord;\n"
        "\n"
        "out vec3 tex_coord;\n"
        "\n"
        "uniform mat4 tc_rotate;\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    gl_Position = vec4(in_position, 0.5, 1.0);\n"
        "    tex_coord = (vec4(in_tex_coord, 0.0, 1.0) * tc_rotate).stp;\n"
        "}\n"
    ;

    static const char quad_shader_fs[] =
        "#version 330 core\n"
        "\n"
        "in vec3 tex_coord;\n"
        "\n"
        "layout (location = 0) out vec4 color;\n"
        "\n"
        "uniform sampler3D tex;\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    color = texture(tex, tex_coord).rrrr;\n"
        "}\n"
    ;

    vglAttachShaderSource(base_prog, GL_VERTEX_SHADER, quad_shader_vs);
    vglAttachShaderSource(base_prog, GL_FRAGMENT_SHADER, quad_shader_fs);

    glGenBuffers(1, &quad_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);

    static const GLfloat quad_data[] =
    {
         1.0f, -1.0f,
        -1.0f, -1.0f,
        -1.0f, 1.0f,
         1.0f, 1.0f,

         0.0f, 0.0f,
         1.0f, 0.0f,
         1.0f, 1.0f,
         0.0f, 1.0f
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_data), quad_data, GL_STATIC_DRAW);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(8 * sizeof(float)));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glLinkProgram(base_prog);

    char buf[1024];
    glGetProgramInfoLog(base_prog, 1024, NULL, buf);

    tc_rotate_loc = glGetUniformLocation(base_prog, "tc_rotate");

    vglImageData image;

    tex = vglLoadTexture("media/cloud.dds", 0, &image);

    glTexParameteri(image.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    vglUnloadImage(&image);
}

void VolumeTextureExample::Display(bool auto_redraw)
{
    float t = float(app_time()) / float(0x3FFF);
    static const vmath::vec3 X(1.0f, 0.0f, 0.0f);
    static const vmath::vec3 Y(0.0f, 1.0f, 0.0f);
    static const vmath::vec3 Z(0.0f, 0.0f, 1.0f);

    vmath::mat4 tc_matrix(vmath::mat4::identity());

    glClearColor(0.6f, 0.7f, 0.9f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_CULL_FACE);
    glUseProgram(base_prog);

    tc_matrix = vmath::rotate(t * 170.0f, X) *
                vmath::rotate(t * 137.0f, Y) *
                vmath::rotate(t * 93.0f, Z);

    glUniformMatrix4fv(tc_rotate_loc, 1, GL_FALSE, tc_matrix);
    glBindVertexArray(vao);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDisable(GL_BLEND);

    base::Display();
}

void VolumeTextureExample::Finalize(void)
{
    glUseProgram(0);
    glDeleteProgram(base_prog);
    glDeleteTextures(1, &tex);
    glDeleteVertexArrays(1, &tex);
}

void VolumeTextureExample::Resize(int width, int height)
{
    glViewport(0, 0 , width, height);

    aspect = float(height) / float(width);
}
