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

BEGIN_APP_DECLARATION(MultiTextureExample)
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

    GLuint tex1;
    GLuint tex2;

    GLint  time_loc;
END_APP_DECLARATION()

DEFINE_APP(MultiTextureExample, "Simple Multi-Texture Example")

void MultiTextureExample::Initialize(const char * title)
{
    base::Initialize(title);

    base_prog = glCreateProgram();

    static const char quad_shader_vs[] =
        "#version 330 core\n"
        "\n"
        "layout (location = 0) in vec2 in_position;\n"
        "layout (location = 1) in vec2 in_tex_coord;\n"
        "\n"
        "out vec2 tex_coord0;\n"
        "out vec2 tex_coord1;\n"
        "\n"
        "uniform float time;\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    mat2 m = mat2( vec2(cos(time), sin(time)),\n"
        "                   vec2(-sin(time), cos(time)) );\n"
        "    gl_Position = vec4(in_position, 0.5, 1.0);\n"
        "    tex_coord0 = in_tex_coord * m;\n"
        "    tex_coord1 = in_tex_coord * transpose(m);\n"
        "}\n"
    ;

    static const char quad_shader_fs[] =
        "#version 330 core\n"
        "\n"
        "in vec2 tex_coord0;\n"
        "in vec2 tex_coord1;\n"
        "\n"
        "layout (location = 0) out vec4 color;\n"
        "\n"
        "uniform sampler2D tex1;\n"
        "uniform sampler2D tex2;\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    color = texture(tex1, tex_coord0) + texture(tex2, tex_coord1);\n"
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

    glUseProgram(base_prog);

    time_loc = glGetUniformLocation(base_prog, "time");
    glUniform1i(glGetUniformLocation(base_prog, "tex1"), 0);
    glUniform1i(glGetUniformLocation(base_prog, "tex2"), 1);

    vglImageData image;

    tex1 = vglLoadTexture("media/test.dds", 0, &image);

    glBindTexture(image.target, tex1);
    glTexParameteri(image.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    vglUnloadImage(&image);

    tex2 = vglLoadTexture("media/test3.dds", 0, &image);

    vglUnloadImage(&image);
}

void MultiTextureExample::Display(bool auto_redraw)
{
    float t = float(app_time() & 0x3FFF) / float(0x3FFF);
    static const vmath::vec3 X(1.0f, 0.0f, 0.0f);
    static const vmath::vec3 Y(0.0f, 1.0f, 0.0f);
    static const vmath::vec3 Z(0.0f, 0.0f, 1.0f);

    glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_CULL_FACE);
    glUseProgram(base_prog);

    glUniform1f(time_loc, t);

    glBindVertexArray(vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tex2);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    base::Display();
}

void MultiTextureExample::Finalize(void)
{
    glUseProgram(0);
    glDeleteProgram(base_prog);
    glDeleteTextures(1, &tex1);
    glDeleteTextures(2, &tex2);
    glDeleteVertexArrays(1, &vao);
}

void MultiTextureExample::Resize(int width, int height)
{
    glViewport(0, 0 , width, height);

    aspect = float(height) / float(width);
}
