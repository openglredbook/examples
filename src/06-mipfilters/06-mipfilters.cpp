/* $URL$
   $Rev$
   $Author$
   $Date$
   $Id$
 */

#include <vermilion.h>

#include "vapp.h"
#include "vutils.h"
#include "vbm.h"

#include "vmath.h"

#include <stdio.h>

BEGIN_APP_DECLARATION(MipmapExample)
    // Override functions from base class
    virtual void Initialize(const char * title);
    virtual void Display(bool auto_redraw);
    virtual void Finalize(void);
    virtual void Resize(int width, int height);

    // Member variables
    float aspect;
    GLuint mipmap_prog;
    GLuint vao;

    GLuint cube_vbo;
    GLuint cube_element_buffer;

    GLuint tex;
    GLint skybox_rotate_loc;

    GLint object_mat_mvp_loc;
    GLint object_mat_mv_loc;

    VBObject object;
END_APP_DECLARATION()

DEFINE_APP(MipmapExample, "Mipmap Example")

void MipmapExample::Initialize(const char * title)
{
    base::Initialize(title);

    mipmap_prog = glCreateProgram();

    static const char skybox_shader_vs[] =
        "#version 330 core\n"
        "\n"
        "layout (location = 0) in vec3 in_position;\n"
        "layout (location = 1) in vec2 in_texcoord;\n"
        "\n"
        "out vec2 tex_coord;\n"
        "\n"
        "uniform mat4 tc_rotate;\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    gl_Position = tc_rotate * vec4(in_position, 1.0);\n"
        "    tex_coord = in_texcoord;\n"
        "}\n"
    ;

    static const char skybox_shader_fs[] =
        "#version 330 core\n"
        "\n"
        "in vec2 tex_coord;\n"
        "\n"
        "layout (location = 0) out vec4 color;\n"
        "\n"
        "uniform sampler2D tex;\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    color = texture(tex, tex_coord);\n"
        "}\n"
    ;

    vglAttachShaderSource(mipmap_prog, GL_VERTEX_SHADER, skybox_shader_vs);
    vglAttachShaderSource(mipmap_prog, GL_FRAGMENT_SHADER, skybox_shader_fs);

    glLinkProgram(mipmap_prog);

    glGenBuffers(1, &cube_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);

    static const GLfloat plane_vertices[] =
    {
        -20.0f, 0.0f, -50.0f,
        -20.0f, 0.0f,  50.0f,
         20.0f, 0.0f, -50.0f,
         20.0f, 0.0f,  50.0f
    };

    static const GLfloat plane_texcoords[] =
    {
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f
    };

    static const GLushort plane_indices[] =
    {
        0, 1, 2, 3
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(plane_vertices) + sizeof(plane_texcoords), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(plane_vertices), plane_vertices);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(plane_vertices), sizeof(plane_texcoords), plane_texcoords);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(plane_vertices)));
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &cube_element_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_element_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(plane_indices), plane_indices, GL_STATIC_DRAW);

    skybox_rotate_loc = glGetUniformLocation(mipmap_prog, "tc_rotate");

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexStorage2D(GL_TEXTURE_2D, 7, GL_RGBA8, 64, 64);

    unsigned int * data = new unsigned int [64 * 64];

    unsigned int colors[] = { 0xFF0000FF, 0xFF00FF00, 0xFFFF0000, 0xFF00FFFF, 0xFFFF00FF, 0xFFFFFF00, 0xFFFFFFFF };

    int i, j, k;
    int n;

    for (i = 0; i < 7; i++)
    {
        n = 0;
        for (j = 0; j < (64 >> i); j++)
        {
            for (k = 0; k < (64 >> i); k++)
            {
                data[n] = (k ^ (64 - j)) * 0x04040404;
                n++;
            }
        }
        glTexSubImage2D(GL_TEXTURE_2D, i, 0, 0, 64 >> i, 64 >> i, GL_RGBA, GL_UNSIGNED_BYTE, data);
    }

    // glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 4.5f);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    delete [] data;
}

void MipmapExample::Display(bool auto_redraw)
{
    static const unsigned int start_time = app_time();
    float t = float((app_time() - start_time)) / float(0x3FFF);
    static const vmath::vec3 X(1.0f, 0.0f, 0.0f);
    static const vmath::vec3 Y(0.0f, 1.0f, 0.0f);
    static const vmath::vec3 Z(0.0f, 0.0f, 1.0f);

    vmath::mat4 tc_matrix(vmath::mat4::identity());

    glClearColor(0.0f, 0.25f, 0.3f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glDisable(GL_CULL_FACE);
    glUseProgram(mipmap_prog);

    tc_matrix = vmath::translate(vmath::vec3(0.0f, 0.0f, -60.0f)) * vmath::rotate(80.0f * 3.0f * 0.03f, X);
    tc_matrix = vmath::perspective(35.0f, 1.0f / aspect, 0.1f, 700.0f) * tc_matrix;

    glUniformMatrix4fv(skybox_rotate_loc, 1, GL_FALSE, tc_matrix);
    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_element_buffer);

    t = fmodf(t, 1.0f);

    if (t < 0.25f)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    }
    else if (t < 0.5f)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    }
    else if (t < 0.75f)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }

    glDrawElements(GL_TRIANGLE_STRIP, 8, GL_UNSIGNED_SHORT, NULL);
    glDrawElements(GL_TRIANGLE_STRIP, 8, GL_UNSIGNED_SHORT, BUFFER_OFFSET(8 * sizeof(GLushort)));

    base::Display();
}

void MipmapExample::Finalize(void)
{
    glUseProgram(0);
    glDeleteProgram(mipmap_prog);
    glDeleteTextures(1, &tex);
    glDeleteVertexArrays(1, &tex);
}

void MipmapExample::Resize(int width, int height)
{
    glViewport(0, 0 , width, height);

    aspect = float(height) / float(width);
}
