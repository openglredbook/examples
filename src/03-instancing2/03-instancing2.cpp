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

#include <stdio.h>

using namespace vmath;

BEGIN_APP_DECLARATION(InstancingExample)
    // Override functions from base class
    virtual void Initialize(const char * title);
    virtual void Display(bool auto_redraw);
    virtual void Finalize(void);
    virtual void Resize(int width, int height);

    // Member variables
    float aspect;

    GLuint color_buffer;
    GLuint model_matrix_buffer;
    GLuint render_prog;
    GLint model_matrix_loc;
    GLint view_matrix_loc;
    GLint projection_matrix_loc;

    VBObject object;
END_APP_DECLARATION()

DEFINE_APP(InstancingExample, "Instancing Example")

#define INSTANCE_COUNT 100

void InstancingExample::Initialize(const char * title)
{
    int n;

    base::Initialize(title);

    // Create the program for rendering the model
    render_prog = glCreateProgram();

    // This is the rendering vertex shader
    static const char render_vs[] =
        "#version 330\n"
        "\n"
        "// 'position' and 'normal' are regular vertex attributes\n"
        "layout (location = 0) in vec4 position;\n"
        "layout (location = 1) in vec3 normal;\n"
        "\n"
        "// Color is a per-instance attribute\n"
        "layout (location = 2) in vec4 color;\n"
        "\n"
        "// model_matrix will be used as a per-instance transformation\n"
        "// matrix. Note that a mat4 consumes 4 consecutive locations, so\n"
        "// this will actually sit in locations, 3, 4, 5, and 6.\n"
        "layout (location = 3) in mat4 model_matrix;\n"
        "\n"
        "// The view matrix and the projection matrix are constant across a draw\n"
        "uniform mat4 view_matrix;\n"
        "uniform mat4 projection_matrix;\n"
        "\n"
        "// The output of the vertex shader (matched to the fragment shader)\n"
        "out VERTEX\n"
        "{\n"
        "    vec3    normal;\n"
        "    vec4    color;\n"
        "} vertex;\n"
        "\n"
        "// Ok, go!\n"
        "void main(void)\n"
        "{\n"
        "    // Construct a model-view matrix from the uniform view matrix\n"
        "    // and the per-instance model matrix.\n"
        "    mat4 model_view_matrix = view_matrix * model_matrix;\n"
        "\n"
        "    // Transform position by the model-view matrix, then by the\n"
        "    // projection matrix.\n"
        "    gl_Position = projection_matrix * (model_view_matrix * position);\n"
        "    // Transform the normal by the upper-left-3x3-submatrix of the\n"
        "    // model-view matrix\n"
        "    vertex.normal = mat3(model_view_matrix) * normal;\n"
        "    // Pass the per-instance color through to the fragment shader.\n"
        "    vertex.color = color;\n"
        "}\n";

    // Simple fragment shader
    static const char render_fs[] =
        "#version 330\n"
        "\n"
        "layout (location = 0) out vec4 color;\n"
        "\n"
        "in VERTEX\n"
        "{\n"
        "    vec3    normal;\n"
        "    vec4    color;\n"
        "} vertex;\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    color = vertex.color * (0.1 + abs(vertex.normal.z)) + vec4(0.8, 0.9, 0.7, 1.0) * pow(abs(vertex.normal.z), 40.0);\n"
        "}\n";

    // Compile and link like normal
    vglAttachShaderSource(render_prog, GL_VERTEX_SHADER, render_vs);
    vglAttachShaderSource(render_prog, GL_FRAGMENT_SHADER, render_fs);

    glLinkProgram(render_prog);
    glUseProgram(render_prog);

    // Get the location of the projetion_matrix uniform
    view_matrix_loc = glGetUniformLocation(render_prog, "view_matrix");
    projection_matrix_loc = glGetUniformLocation(render_prog, "projection_matrix");

    // Load the object
    object.LoadFromVBM("media/armadillo_low.vbm", 0, 1, 2);

    // Bind its vertex array object so that we can append the instanced attributes
    object.BindVertexArray();

    // Get the locations of the vertex attributes in 'prog', which is the
    // (linked) program object that we're going to be rendering with. Note
    // that this isn't really necessary because we specified locations for
    // all the attributes in our vertex shader. This code could be made
    // more concise by assuming the vertex attributes are where we asked
    // the compiler to put them.
    int position_loc    = glGetAttribLocation(render_prog, "position");
    int normal_loc      = glGetAttribLocation(render_prog, "normal");
    int color_loc       = glGetAttribLocation(render_prog, "color");
    int matrix_loc      = glGetAttribLocation(render_prog, "model_matrix");

    // Configure the regular vertex attribute arrays - position and color.
    /*
    // This is commented out here because the VBM object takes care
    // of it for us.
    glBindBuffer(GL_ARRAY_BUFFER, position_buffer);
    glVertexAttribPointer(position_loc, 4, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(position_loc);
    glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
    glVertexAttribPointer(normal_loc, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(normal_loc);
    */

    // Generate the colors of the objects
    vec4 colors[INSTANCE_COUNT];

    for (n = 0; n < INSTANCE_COUNT; n++)
    {
        float a = float(n) / 4.0f;
        float b = float(n) / 5.0f;
        float c = float(n) / 6.0f;

        colors[n][0] = 0.5f + 0.25f * (sinf(a + 1.0f) + 1.0f);
        colors[n][1] = 0.5f + 0.25f * (sinf(b + 2.0f) + 1.0f);
        colors[n][2] = 0.5f + 0.25f * (sinf(c + 3.0f) + 1.0f);
        colors[n][3] = 1.0f;
    }

    glGenBuffers(1, &color_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, color_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_DYNAMIC_DRAW);

    // Now we set up the color array. We want each instance of our geometry
    // to assume a different color, so we'll just pack colors into a buffer
    // object and make an instanced vertex attribute out of it.
    glBindBuffer(GL_ARRAY_BUFFER, color_buffer);
    glVertexAttribPointer(color_loc, 4, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(color_loc);
    // This is the important bit... set the divisor for the color array to
    // 1 to get OpenGL to give us a new value of 'color' per-instance
    // rather than per-vertex.
    glVertexAttribDivisor(color_loc, 1);

    // Likewise, we can do the same with the model matrix. Note that a
    // matrix input to the vertex shader consumes N consecutive input
    // locations, where N is the number of columns in the matrix. So...
    // we have four vertex attributes to set up.
    glGenBuffers(1, &model_matrix_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, model_matrix_buffer);
    glBufferData(GL_ARRAY_BUFFER, INSTANCE_COUNT * sizeof(mat4), NULL, GL_DYNAMIC_DRAW);
    // Loop over each column of the matrix...
    for (int i = 0; i < 4; i++)
    {
        // Set up the vertex attribute
        glVertexAttribPointer(matrix_loc + i,              // Location
                              4, GL_FLOAT, GL_FALSE,       // vec4
                              sizeof(mat4),                // Stride
                              (void *)(sizeof(vec4) * i)); // Start offset
        // Enable it
        glEnableVertexAttribArray(matrix_loc + i);
        // Make it instanced
        glVertexAttribDivisor(matrix_loc + i, 1);
    }

    // Done (unbind the object's VAO)
    glBindVertexArray(0);
}

static inline int min(int a, int b)
{
    return a < b ? a : b;
}

void InstancingExample::Display(bool auto_redraw)
{
    float t = float(app_time() & 0x3FFF) / float(0x3FFF);
    static float q = 0.0f;
    static const vec3 X(1.0f, 0.0f, 0.0f);
    static const vec3 Y(0.0f, 1.0f, 0.0f);
    static const vec3 Z(0.0f, 0.0f, 1.0f);
    int n;

    // Clear
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Setup
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // Bind the weight VBO and change its data
    glBindBuffer(GL_ARRAY_BUFFER, model_matrix_buffer);

    // Set model matrices for each instance
    mat4 * matrices = (mat4 *)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

    for (n = 0; n < INSTANCE_COUNT; n++)
    {
        float a = 50.0f * float(n) / 4.0f;
        float b = 50.0f * float(n) / 5.0f;
        float c = 50.0f * float(n) / 6.0f;

        matrices[n] = rotate(a + t * 360.0f, 1.0f, 0.0f, 0.0f) *
                      rotate(b + t * 360.0f, 0.0f, 1.0f, 0.0f) *
                      rotate(c + t * 360.0f, 0.0f, 0.0f, 1.0f) *
                      translate(10.0f + a, 40.0f + b, 50.0f + c);
    }

    glUnmapBuffer(GL_ARRAY_BUFFER);

    // Activate instancing program
    glUseProgram(render_prog);

    // Set up the view and projection matrices
    mat4 view_matrix(translate(0.0f, 0.0f, -1500.0f) * rotate(t * 360.0f * 2.0f, 0.0f, 1.0f, 0.0f));
    mat4 projection_matrix(frustum(-1.0f, 1.0f, -aspect, aspect, 1.0f, 5000.0f));

    glUniformMatrix4fv(view_matrix_loc, 1, GL_FALSE, view_matrix);
    glUniformMatrix4fv(projection_matrix_loc, 1, GL_FALSE, projection_matrix);

    // Render INSTANCE_COUNT objects
    object.Render(0, INSTANCE_COUNT);

    lookat(vec3(0.0f, 0.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));

    base::Display();
}

void InstancingExample::Finalize(void)
{
    glUseProgram(0);
    glDeleteProgram(render_prog);
    glDeleteBuffers(1, &color_buffer);
    glDeleteBuffers(1, &model_matrix_buffer);
}

void InstancingExample::Resize(int width, int height)
{
    glViewport(0, 0 , width, height);

    aspect = float(height) / float(width);
}
