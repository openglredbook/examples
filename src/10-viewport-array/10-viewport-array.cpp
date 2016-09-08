/* $URL$
   $Rev$
   $Author$
   $Date$
   $Id$
 */

#include "vapp.h"
#include "vutils.h"
#include "vbm.h"

#include "vmath.h"

#include <stdio.h>

BEGIN_APP_DECLARATION(ViewportArrayApplication)
    // Override functions from base class
    virtual void Initialize(const char * title);
    virtual void Display(bool auto_redraw);
    virtual void Finalize(void);
    virtual void Resize(int width, int height);

    // Member variables
    float aspect;
    GLuint prog;
    GLuint vao;
    GLuint vbo;
    VBObject object;

    GLint model_matrix_pos;
    GLint projection_matrix_pos;
END_APP_DECLARATION()

DEFINE_APP(ViewportArrayApplication, "Viewport Array")

void ViewportArrayApplication::Initialize(const char * title)
{
    base::Initialize(title);

    prog = glCreateProgram();

    static const char vertex_shader_source[] =
        "#version 410\n"
        "\n"
        "layout (location = 0) in vec4 position;\n"
        "layout (location = 1) in vec3 normal;\n"
        "\n"
        "out vec3 vs_normal;\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    vs_normal = normal;\n"
        "    gl_Position = position;\n"
        "}\n";

    static const char geometry_shader_source[] =
        "#version 410\n"
        "\n"
        "layout (triangles, invocations = 4) in;\n"
        "layout (triangle_strip, max_vertices = 3) out;\n"
        "\n"
        "uniform mat4 model_matrix[4];\n"
        "uniform mat4 projection_matrix;\n"
        "\n"
        "in vec3 vs_normal[];\n"
        "\n"
        "out vec4 gs_color;\n"
        "out vec3 gs_normal;\n"
        "\n"
        "const vec4 colors[4] = vec4[4]\n"
        "(\n"
        "    vec4(1.0, 0.7, 0.3, 1.0),\n"
        "    vec4(1.0, 0.2, 0.3, 1.0),\n"
        "    vec4(0.1, 0.6, 1.0, 1.0),\n"
        "    vec4(0.3, 0.7, 0.5, 1.0)\n"
        ");\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    for (int i = 0; i < gl_in.length(); i++)\n"
        "    {\n"
        "        gl_ViewportIndex = gl_InvocationID;\n"
        "        gs_color = colors[gl_InvocationID];\n"
        "        gs_normal = (model_matrix[gl_InvocationID] * vec4(vs_normal[i], 0.0)).xyz;\n"
        "        gl_Position = projection_matrix *\n"
        "                      (model_matrix[gl_InvocationID] * gl_in[i].gl_Position);\n"
        "        EmitVertex();\n"
        "    }\n"
        "}\n";

    static const char fragment_shader_source[] =
        "#version 410\n"
        "\n"
        "layout (location = 0) out vec4 color;\n"
        "\n"
        "in vec4 gs_color;\n"
        "in vec3 gs_normal;\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    color = gs_color * (0.2 + pow(abs(gs_normal.z), 4.0)) + vec4(1.0, 1.0, 1.0, 0.0) * pow(abs(gs_normal.z), 37.0);\n"
        "}\n";

    vglAttachShaderSource(prog, GL_VERTEX_SHADER, vertex_shader_source);
    vglAttachShaderSource(prog, GL_GEOMETRY_SHADER, geometry_shader_source);
    vglAttachShaderSource(prog, GL_FRAGMENT_SHADER, fragment_shader_source);

    glLinkProgram(prog);
    glUseProgram(prog);

    model_matrix_pos = glGetUniformLocation(prog, "model_matrix");
    projection_matrix_pos = glGetUniformLocation(prog, "projection_matrix");

    object.LoadFromVBM("media/ninja.vbm", 0, 1, 2);
}

void ViewportArrayApplication::Display(bool auto_redraw)
{
    float t = float(app_time() & 0x3FFF) / float(0x3FFF);
    static const vmath::vec3 X(1.0f, 0.0f, 0.0f);
    static const vmath::vec3 Y(0.0f, 1.0f, 0.0f);
    static const vmath::vec3 Z(0.0f, 0.0f, 1.0f);

    glClearColor( 0.3f, 0.1f, 0.2f, 1.0f );
    glClearDepth( 1.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glUseProgram(prog);

    vmath::mat4 p(vmath::frustum(-1.0f, 1.0f, aspect, -aspect, 1.0f, 5000.0f));
    vmath::mat4 m[4];

    for (int i = 0; i < 4; i++)
    {
        m[i] = vmath::mat4(vmath::translate(0.0f, 0.0f, 100.0f * sinf(6.28318531f * t + i) - 230.0f) *
                           vmath::rotate(360.0f * t * float(i + 1), X) *
                           vmath::rotate(360.0f * t * float(i + 2), Y) *
                           vmath::rotate(360.0f * t * float(5 - i), Z) *
                           vmath::translate(0.0f, -80.0f, 0.0f));
    }

    glUniformMatrix4fv(model_matrix_pos, 4, GL_FALSE, m[0]);
    glUniformMatrix4fv(projection_matrix_pos, 1, GL_FALSE, p);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    object.Render();

    base::Display();
}

void ViewportArrayApplication::Finalize(void)
{
    glUseProgram(0);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(prog);
    glDeleteVertexArrays(1, &vao);
}

void ViewportArrayApplication::Resize(int width, int height)
{
    const float wot = float(width) * 0.5f;
    const float hot = float(height) * 0.5f;

    glViewportIndexedf(0, 0.0f, 0.0f, wot, hot);
    glViewportIndexedf(1, wot, 0.0f, wot, hot);
    glViewportIndexedf(2, 0.0f, hot, wot, hot);
    glViewportIndexedf(3, wot, hot, wot, hot);

    aspect = hot / wot;
}
