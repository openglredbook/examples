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

BEGIN_APP_DECLARATION(SimpleComputeShaderExample)
    // Override functions from base class
    virtual void Initialize(const char * title);
    virtual void Display(bool auto_redraw);
    virtual void Finalize(void);
    virtual void Resize(int width, int height);

    // Member variables
    GLuint  compute_prog;
    GLuint  compute_shader;

    // Texture for compute shader to write into
    GLuint  output_image;

    // Program, vao and vbo to render a full screen quad
    GLuint  render_prog;
    GLuint  render_vao;
    GLuint  render_vbo;
END_APP_DECLARATION()

DEFINE_APP(SimpleComputeShaderExample, "Simple Compute Shader Example")

void SimpleComputeShaderExample::Initialize(const char * title)
{
    base::Initialize(title);

    // Initialize our compute program
    compute_prog = glCreateProgram();

    static const char compute_shader_source[] =
        "#version 430 core\n"
        "\n"
        "layout (local_size_x = 32, local_size_y = 16) in;\n"
        "\n"
        "layout (rgba32f) uniform image2D output_image;\n"
        "void main(void)\n"
        "{\n"
        "    imageStore(output_image,\n"
        "    ivec2(gl_GlobalInvocationID.xy),\n"
        "    vec4(vec2(gl_LocalInvocationID.xy) / vec2(gl_WorkGroupSize.xy), 0.0, 0.0));\n"
        "}\n"
    ;

    vglAttachShaderSource(compute_prog, GL_COMPUTE_SHADER, compute_shader_source);

    glLinkProgram(compute_prog);

    // This is the texture that the compute program will write into
    glGenTextures(1, &output_image);
    glBindTexture(GL_TEXTURE_2D, output_image);
    glTexStorage2D(GL_TEXTURE_2D, 8, GL_RGBA32F, 256, 256);

    // Now create a simple program to visualize the result
    render_prog = glCreateProgram();

    static const char render_vs[] =
        "#version 430 core\n"
        "\n"
        "in vec4 vert;\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    gl_Position = vert;\n"
        "}\n";

    static const char render_fs[] =
        "#version 430 core\n"
        "\n"
        "layout (location = 0) out vec4 color;\n"
        "\n"
        "uniform sampler2D output_image;\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    color = texture(output_image, vec2(gl_FragCoord.xy) / vec2(textureSize(output_image, 0)));\n"
        "}\n";

    vglAttachShaderSource(render_prog, GL_VERTEX_SHADER, render_vs);
    vglAttachShaderSource(render_prog, GL_FRAGMENT_SHADER, render_fs);

    glLinkProgram(render_prog);

    // This is the VAO containing the data to draw the quad (including its associated VBO)
    glGenVertexArrays(1, &render_vao);
    glBindVertexArray(render_vao);
    glEnableVertexAttribArray(0);
    glGenBuffers(1, &render_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, render_vbo);
    static const float verts[] =
    {
        -1.0f, -1.0f, 0.5f, 1.0f,
         1.0f, -1.0f, 0.5f, 1.0f,
         1.0f,  1.0f, 0.5f, 1.0f,
        -1.0f,  1.0f, 0.5f, 1.0f,
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
}

void SimpleComputeShaderExample::Display(bool auto_redraw)
{
    // Activate the compute program and bind the output texture image
    glUseProgram(compute_prog);
    glBindImageTexture(0, output_image, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glDispatchCompute(8, 16, 1);

    // Now bind the texture for rendering _from_
    glBindTexture(GL_TEXTURE_2D, output_image);

    // Clear, select the rendering program and draw a full screen quad
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(render_prog);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    base::Display();
}

void SimpleComputeShaderExample::Finalize(void)
{
    glUseProgram(0);
    glDeleteProgram(compute_prog);
    glDeleteProgram(render_prog);
    glDeleteTextures(1, &output_image);
    glDeleteVertexArrays(1, &render_vao);
}

void SimpleComputeShaderExample::Resize(int width, int height)
{
    glViewport(0, 0, width, height);
}
