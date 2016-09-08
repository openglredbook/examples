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
    GLuint sort_prog;
    GLuint render_prog;
    GLuint vao[2];
    GLuint vbo[2];
    GLuint xfb;
    VBObject object;

    GLint model_matrix_pos;
    GLint projection_matrix_pos;
END_APP_DECLARATION()

DEFINE_APP(ViewportArrayApplication, "DrawTransformFeedback Example")

void ViewportArrayApplication::Initialize(const char * title)
{
    int i;

    base::Initialize(title);

    glGenTransformFeedbacks(1, &xfb);
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, xfb);

    sort_prog = glCreateProgram();

    static const char sort_vs_source[] =
            "#version 410\n"
            "\n"
            "uniform mat4 model_matrix;\n"
            "\n"
            "layout (location = 0) in vec4 position;\n"
            "layout (location = 1) in vec3 normal;\n"
            "\n"
            "out vec3 vs_normal;\n"
            "\n"
            "void main(void)\n"
            "{\n"
            "    vs_normal = (model_matrix * vec4(normal, 0.0)).xyz;\n"
            "    gl_Position = model_matrix * position;\n"
            "}\n";

    static const char sort_gs_source[] =
        "#version 410\n"
        "\n"
        "layout (triangles) in;\n"
        "layout (points, max_vertices = 3) out;\n"
        "\n"
        "uniform mat4 projection_matrix;\n"
        "\n"
        "in vec3 vs_normal[];\n"
        "\n"
        "layout (stream = 0) out vec4 rf_position;\n"
        "layout (stream = 0) out vec3 rf_normal;\n"
        "\n"
        "layout (stream = 1) out vec4 lf_position;\n"
        "layout (stream = 1) out vec3 lf_normal;\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    vec4 A = gl_in[0].gl_Position;\n"
        "    vec4 B = gl_in[1].gl_Position;\n"
        "    vec4 C = gl_in[2].gl_Position;\n"
        "    vec3 AB = (B - A).xyz;\n"
        "    vec3 AC = (C - A).xyz;\n"
        "    vec3 face_normal = cross(AB, AC);\n"
        "    int i;\n"
        "\n"
        "    if (face_normal.x < 0.0)\n"
        "    {\n"
        "        for (i = 0; i < gl_in.length(); i++)\n"
        "        {\n"
        "            rf_position = projection_matrix * (gl_in[i].gl_Position - vec4(30.0, 0.0, 0.0, 0.0));\n"
        "            rf_normal = vs_normal[i];\n"
        "            EmitStreamVertex(0);\n"
        "        }\n"
        "        EndStreamPrimitive(0);\n"
        "    }\n"
        "    else\n"
        "    {\n"
        "        for (i = 0; i < gl_in.length(); i++)\n"
        "        {\n"
        "            lf_position = projection_matrix * (gl_in[i].gl_Position + vec4(30.0, 0.0, 0.0, 0.0));\n"
        "            lf_normal = vs_normal[i];\n"
        "            EmitStreamVertex(1);\n"
        "        }\n"
        "        EndStreamPrimitive(1);\n"
        "    }\n"
        "}\n";

    vglAttachShaderSource(sort_prog, GL_VERTEX_SHADER, sort_vs_source);
    vglAttachShaderSource(sort_prog, GL_GEOMETRY_SHADER, sort_gs_source);

    static const char * varyings[] =
    {
        "rf_position", "rf_normal",
        "gl_NextBuffer",
        "lf_position", "lf_normal"
    };

    glTransformFeedbackVaryings(sort_prog, 5, varyings, GL_INTERLEAVED_ATTRIBS);

    glLinkProgram(sort_prog);
    glUseProgram(sort_prog);

    model_matrix_pos = glGetUniformLocation(sort_prog, "model_matrix");
    projection_matrix_pos = glGetUniformLocation(sort_prog, "projection_matrix");

    glGenVertexArrays(2, vao);
    glGenBuffers(2, vbo);

    for (i = 0; i < 2; i++)
    {
        glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, vbo[i]);
        glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, 1024 * 1024 * sizeof(GLfloat), NULL, GL_DYNAMIC_COPY);
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, i, vbo[i]);

        glBindVertexArray(vao[i]);
        glBindBuffer(GL_ARRAY_BUFFER, vbo[i]);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(vmath::vec4) + sizeof(vmath::vec3), NULL);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vmath::vec4) + sizeof(vmath::vec3), (GLvoid *)(sizeof(vmath::vec4)));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
    }

    render_prog = glCreateProgram();

    static const char render_vs_source[] =
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

    static const char render_fs_source[] =
        "#version 410\n"
        "\n"
        "layout (location = 0) out vec4 color;\n"
        "\n"
        "uniform vec4 pass_color;\n"
        "\n"
        "in vec3 vs_normal;\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    color = pass_color * (0.2 + pow(abs(vs_normal.z), 4.0)) + vec4(1.0, 1.0, 1.0, 0.0) * pow(abs(vs_normal.z), 37.0);\n"
        "}\n";

    vglAttachShaderSource(render_prog, GL_VERTEX_SHADER, render_vs_source);
    vglAttachShaderSource(render_prog, GL_FRAGMENT_SHADER, render_fs_source);

    glLinkProgram(render_prog);

    object.LoadFromVBM("media/ninja.vbm", 0, 1, 2);
}

void ViewportArrayApplication::Display(bool auto_redraw)
{
    float t = float(app_time() & 0x3FFF) / float(0x3FFF);
    static const vmath::vec3 X(1.0f, 0.0f, 0.0f);
    static const vmath::vec3 Y(0.0f, 1.0f, 0.0f);
    static const vmath::vec3 Z(0.0f, 0.0f, 1.0f);

    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(sort_prog);

    vmath::mat4 p(vmath::frustum(-1.0f, 1.0f, aspect, -aspect, 1.0f, 5000.0f));
    vmath::mat4 m;

    m = vmath::mat4(vmath::translate(0.0f,
                                       0.0f,
                                       100.0f * sinf(6.28318531f * t) - 230.0f) *
                    vmath::rotate(360.0f * t, X) *
                    vmath::rotate(360.0f * t * 2.0f, Y) *
                    vmath::rotate(360.0f * t * 5.0f, Z) *
                    vmath::translate(0.0f, -80.0f, 0.0f));

    glUniformMatrix4fv(model_matrix_pos, 1, GL_FALSE, m[0]);
    glUniformMatrix4fv(projection_matrix_pos, 1, GL_FALSE, p);

    glEnable(GL_RASTERIZER_DISCARD);

    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, xfb);
    glBeginTransformFeedback(GL_POINTS);

    object.Render();

    glEndTransformFeedback();
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);

    glDisable(GL_RASTERIZER_DISCARD);

    static const vmath::vec4 colors[2] =
    {
        vmath::vec4(0.8f, 0.8f, 0.9f, 0.5f),
        vmath::vec4(0.3f, 1.0f, 0.3f, 0.8f)
    };

    glUseProgram(render_prog);

    glUniform4fv(0, 1, colors[0]);
    glBindVertexArray(vao[0]);
    glDrawTransformFeedbackStream(GL_TRIANGLES, xfb, 0);

    glUniform4fv(0, 1, colors[1]);
    glBindVertexArray(vao[1]);
    glDrawTransformFeedbackStream(GL_TRIANGLES, xfb, 1);

    base::Display();
}

void ViewportArrayApplication::Finalize(void)
{
    glUseProgram(0);
    glDeleteProgram(sort_prog);
    glDeleteVertexArrays(2, vao);
    glDeleteBuffers(2, vbo);
}

void ViewportArrayApplication::Resize(int width, int height)
{
    glViewport(0, 0 , width, height);

    aspect = float(height) / float(width);
}
