/* $URL$
   $Rev$
   $Author$
   $Date$
   $Id$
 */

// #define USE_GL3W
#include <vermilion.h>

#include "vapp.h"
#include "vutils.h"
#include "vbm.h"

#include "vmath.h"

#include <stdio.h>

BEGIN_APP_DECLARATION(LightingExample)
    // Override functions from base class
    virtual void Initialize(const char * title);
    virtual void Display(bool auto_redraw);
    virtual void Finalize(void);
    virtual void Resize(int width, int height);

    // Texture for compute shader to write into
    GLuint  output_image;

    // Program, vao and vbo to render a full screen quad
    GLuint  render_prog;

    // Uniform locations
    GLint   mv_mat_loc;
    GLint   prj_mat_loc;
    GLint   col_amb_loc;
    GLint   col_diff_loc;
    GLint   col_spec_loc;

    // Object to render
    VBObject    object;
END_APP_DECLARATION()

DEFINE_APP(LightingExample, "Lighting Example")

void LightingExample::Initialize(const char * title)
{
    base::Initialize(title);

    // Now create a simple program to visualize the result
    render_prog = glCreateProgram();

    static const char render_vs[] =
        "#version 430 core\n"
        "\n"
        "uniform mat4 model_matrix;\n"
        "uniform mat4 proj_matrix;\n"
        "\n"
        "layout (location = 0) in vec4 position;\n"
        "layout (location = 1) in vec3 normal;\n"
        "\n"
        "out vec3 vs_worldpos;\n"
        "out vec3 vs_normal;\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    vec4 position = proj_matrix * model_matrix * position;\n"
        "    gl_Position = position;\n"
        "    vs_worldpos = position.xyz;\n"
        "    vs_normal = mat3(model_matrix) * normal;\n"
        "}\n"
        ;

    static const char render_fs[] =
        "#version 430 core\n"
        "\n"
        "layout (location = 0) out vec4 color;\n"
        "\n"
        "in vec3 vs_worldpos;\n"
        "in vec3 vs_normal;\n"
        "\n"
        "uniform vec4 color_ambient = vec4(0.1, 0.2, 0.5, 1.0);\n"
        "uniform vec4 color_diffuse = vec4(0.2, 0.3, 0.6, 1.0);\n"
        "uniform vec4 color_specular = vec4(0.0); // vec4(1.0, 1.0, 1.0, 1.0);\n"
        "uniform float shininess = 77.0f;\n"
        "\n"
        "uniform vec3 light_position = vec3(12.0f, 32.0f, 560.0f);\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    vec3 light_direction = normalize(light_position - vs_worldpos);\n"
        "    vec3 normal = normalize(vs_normal);\n"
        "    vec3 half_vector = normalize(light_direction + normalize(vs_worldpos));\n"
        "    float diffuse = max(0.0, dot(normal, light_direction));\n"
        "    float specular = pow(max(0.0, dot(vs_normal, half_vector)), shininess);\n"
        "    color = color_ambient + diffuse * color_diffuse + specular * color_specular;\n"
        "}\n";

    vglAttachShaderSource(render_prog, GL_VERTEX_SHADER, render_vs);
    vglAttachShaderSource(render_prog, GL_FRAGMENT_SHADER, render_fs);

    glLinkProgram(render_prog);

    mv_mat_loc = glGetUniformLocation(render_prog, "model_matrix");
    prj_mat_loc = glGetUniformLocation(render_prog, "proj_matrix");
    col_amb_loc = glGetUniformLocation(render_prog, "color_ambient");
    col_diff_loc = glGetUniformLocation(render_prog, "color_diffuse");
    col_spec_loc = glGetUniformLocation(render_prog, "color_specular");

    object.LoadFromVBM("media/torus.vbm", 0, 1, 2);
}

void LightingExample::Display(bool auto_redraw)
{
    float time = float(app_time() & 0xFFFF) / float(0xFFFF);

    vmath::mat4 mv_matrix = vmath::translate(0.0f, 0.0f, -60.0f) *
                            vmath::rotate(987.0f * time * 3.14159f, vmath::vec3(0.0f, 0.0f, 1.0f)) *
                            vmath::rotate(1234.0f * time * 3.14159f, vmath::vec3(1.0f, 0.0f, 0.0f));
    vmath::mat4 prj_matrix = vmath::perspective(60.0f, 1.333f, 0.1f, 1000.0f);

    glUseProgram(render_prog);

    glUniformMatrix4fv(mv_mat_loc, 1, GL_FALSE, mv_matrix);
    glUniformMatrix4fv(prj_mat_loc, 1, GL_FALSE, prj_matrix);

    // Clear, select the rendering program and draw a full screen quad
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    object.Render();

    base::Display();
}

void LightingExample::Finalize(void)
{
    glUseProgram(0);
    glDeleteProgram(render_prog);
    glDeleteTextures(1, &output_image);
}

void LightingExample::Resize(int width, int height)
{
    glViewport(0, 0, width, height);
}
