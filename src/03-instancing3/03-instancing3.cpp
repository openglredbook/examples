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

BEGIN_APP_DECLARATION(InstanceIDExample)
    // Override functions from base class
    virtual void Initialize(const char * title);
    virtual void Display(bool auto_redraw);
    virtual void Finalize(void);
    virtual void Resize(int width, int height);

    // Member variables
    float aspect;

    GLuint color_buffer;
    GLuint model_matrix_buffer;
    GLuint color_tbo;
    GLuint model_matrix_tbo;
    GLuint render_prog;

    GLint view_matrix_loc;
    GLint projection_matrix_loc;

    VBObject object;
END_APP_DECLARATION()

DEFINE_APP(InstanceIDExample, "gl_InstanceID Example")

#define INSTANCE_COUNT 100

void InstanceIDExample::Initialize(const char * title)
{
    int n;

    base::Initialize(title);

    // Create the program for rendering the model
    render_prog = glCreateProgram();

    // This is the rendering vertex shader
    static const char render_vs[] =
        "#version 410\n"
        "\n"
        "// 'position' and 'normal' are regular vertex attributes\n"
        "layout (location = 0) in vec4 position;\n"
        "layout (location = 1) in vec3 normal;\n"
        "\n"
        "// Color is a per-instance attribute\n"
        "layout (location = 2) in vec4 color;\n"
        "\n"
        "// The view matrix and the projection matrix are constant across a draw\n"
        "uniform mat4 view_matrix;\n"
        "uniform mat4 projection_matrix;\n"
        "\n"
        "// These are the TBOs that hold per-instance colors and per-instance\n"
        "// model matrices\n"
        "uniform samplerBuffer color_tbo;\n"
        "uniform samplerBuffer model_matrix_tbo;\n"
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
        "    // Use gl_InstanceID to obtain the instance color from the color TBO\n"
        "    vec4 color = texelFetch(color_tbo, gl_InstanceID);\n"
        "\n"
        "    // Generating the model matrix is more complex because you can't\n"
        "    // store mat4 data in a TBO. Instead, we need to store each matrix\n"
        "    // as four vec4 variables and assemble the matrix in the shader.\n"
        "    // First, fetch the four columns of the matrix (remember, matrices are\n"
        "    // stored in memory in column-primary order).\n"
        "    vec4 col1 = texelFetch(model_matrix_tbo, gl_InstanceID * 4);\n"
        "    vec4 col2 = texelFetch(model_matrix_tbo, gl_InstanceID * 4 + 1);\n"
        "    vec4 col3 = texelFetch(model_matrix_tbo, gl_InstanceID * 4 + 2);\n"
        "    vec4 col4 = texelFetch(model_matrix_tbo, gl_InstanceID * 4 + 3);\n"
        "\n"
        "    // Now assemble the four columns into a matrix.\n"
        "    mat4 model_matrix = mat4(col1, col2, col3, col4);\n"
        "\n"
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
        "#version 410\n"
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

    // Set up the TBO samplers
    GLuint color_tbo_loc = glGetUniformLocation(render_prog, "color_tbo");
    GLuint model_matrix_tbo_loc = glGetUniformLocation(render_prog, "model_matrix_tbo");

    // Set them to the right texture unit indices
    glUniform1i(color_tbo_loc, 0);
    glUniform1i(model_matrix_tbo_loc, 1);

    // Load the object
    object.LoadFromVBM("media/armadillo_low.vbm", 0, 1, 2);

    /*

        THIS IS COMMENTED OUT HERE BECAUSE THE VBM OBJECT TAKES
        CARE OF IT FOR US

    // Get the locations of the vertex attributes in 'prog', which is the
    // (linked) program object that we're going to be rendering with. Note
    // that this isn't really necessary because we specified locations for
    // all the attributes in our vertex shader. This code could be made
    // more concise by assuming the vertex attributes are where we asked
    // the compiler to put them.
    int position_loc    = glGetAttribLocation(prog, "position");
    int normal_loc      = glGetAttribLocation(prog, "normal");

    // Configure the regular vertex attribute arrays - position and normal.
    glBindBuffer(GL_ARRAY_BUFFER, position_buffer);
    glVertexAttribPointer(position_loc, 4, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(position_loc);
    glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
    glVertexAttribPointer(normal_loc, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(normal_loc);

    */

    // Now we set up the TBOs for the instance colors and the model matrices...

    // First, create the TBO to store colors, bind a buffer to it and initialize
    // its format. The buffer has previously been created and sized to store one
    // vec4 per-instance.
    glGenTextures(1, &color_tbo);
    glBindTexture(GL_TEXTURE_BUFFER, color_tbo);

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

    // Create the buffer, initialize it and attach it to the buffer texture
    glGenBuffers(1, &color_buffer);
    glBindBuffer(GL_TEXTURE_BUFFER, color_buffer);
    glBufferData(GL_TEXTURE_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, color_buffer);

    // Now do the same thing with a TBO for the model matrices. The buffer object
    // (model_matrix_buffer) has been created and sized to store one mat4 per-
    // instance.
    glGenTextures(1, &model_matrix_tbo);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, model_matrix_tbo);
    glGenBuffers(1, &model_matrix_buffer);
    glBindBuffer(GL_TEXTURE_BUFFER, model_matrix_buffer);
    glBufferData(GL_TEXTURE_BUFFER, INSTANCE_COUNT * sizeof(mat4), NULL, GL_DYNAMIC_DRAW);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, model_matrix_buffer);
    glActiveTexture(GL_TEXTURE0);
}

static inline int min(int a, int b)
{
    return a < b ? a : b;
}

void InstanceIDExample::Display(bool auto_redraw)
{
    float t = float(app_time() & 0x3FFF) / float(0x3FFF);
    static float q = 0.0f;
    static const vec3 X(1.0f, 0.0f, 0.0f);
    static const vec3 Y(0.0f, 1.0f, 0.0f);
    static const vec3 Z(0.0f, 0.0f, 1.0f);
    int n;

    // Set model matrices for each instance
    mat4 matrices[INSTANCE_COUNT];

    for (n = 0; n < INSTANCE_COUNT; n++)
    {
        float a = 50.0f * float(n) / 4.0f;
        float b = 50.0f * float(n) / 5.0f;
        float c = 50.0f * float(n) / 6.0f;

        matrices[n] = vmath::rotate(a + t * 360.0f, 1.0f, 0.0f, 0.0f) *
                      vmath::rotate(b + t * 360.0f, 0.0f, 1.0f, 0.0f) *
                      vmath::rotate(c + t * 360.0f, 0.0f, 0.0f, 1.0f) *
                      vmath::translate(10.0f + a, 40.0f + b, 50.0f + c);
    }

    // Bind the weight VBO and change its data
    glBindBuffer(GL_TEXTURE_BUFFER, model_matrix_buffer);
    glBufferData(GL_TEXTURE_BUFFER, sizeof(matrices), matrices, GL_DYNAMIC_DRAW);

    // Clear
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Setup
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // Activate instancing program
    glUseProgram(render_prog);

    // Set up the view and projection matrices
    mat4 view_matrix(vmath::translate(0.0f, 0.0f, -1500.0f) * vmath::rotate(t * 360.0f * 2.0f, 0.0f, 1.0f, 0.0f));
    mat4 projection_matrix(frustum(-1.0f, 1.0f, -aspect, aspect, 1.0f, 5000.0f));

    glUniformMatrix4fv(view_matrix_loc, 1, GL_FALSE, view_matrix);
    glUniformMatrix4fv(projection_matrix_loc, 1, GL_FALSE, projection_matrix);

    // Render INSTANCE_COUNT objects
    object.Render(0, INSTANCE_COUNT);

    base::Display();
}

void InstanceIDExample::Finalize(void)
{
    glUseProgram(0);
    glDeleteProgram(render_prog);
    glDeleteBuffers(1, &color_buffer);
    glDeleteBuffers(1, &model_matrix_buffer);
}

void InstanceIDExample::Resize(int width, int height)
{
    glViewport(0, 0 , width, height);

    aspect = float(height) / float(width);
}
