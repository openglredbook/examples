/* $URL$
   $Rev$
   $Author$
   $Date$
   $Id$
 */

// #define USE_GL3W
#include <vermilion.h>

#include <vgl.h>
#include "vapp.h"
#include "vutils.h"
#include "vbm.h"

#include "vmath.h"

#include <stdio.h>

BEGIN_APP_DECLARATION(RayTracingExample)
    // Override functions from base class
    virtual void Initialize(const char * title);
    virtual void Display(bool auto_redraw);
    virtual void Finalize(void);
    virtual void Resize(int width, int height);

    // Compute programs
    GLuint  initializer_prog;
    GLuint  trace_prog;

    // Texture for compute shader to write into
    GLuint  output_image;

    // Ray buffer
    GLuint  ray_buffer[2];

    // Append count buffer
    GLuint  counter_buffer;

    // Program, vao and vbo to render a quad
    GLuint  render_prog;
    GLuint  render_vao;
    GLuint  render_vbo;
END_APP_DECLARATION()

#define OUTPUT_LODS         9
#define OUTPUT_SIZE_X       (1 << OUTPUT_LODS)
#define OUTPUT_SIZE_Y       (1 << OUTPUT_LODS)

DEFINE_APP(RayTracingExample, "Compute Shader Raytracing Example")

#define RAY_ELEMENT                     \
    "struct RAY\n"                      \
    "{\n"                               \
    "    ivec4  screen_origin;\n"       \
    "    vec4   world_origin;\n"        \
    "    vec4   world_direction;\n"     \
    "};\n"

void RayTracingExample::Initialize(const char * title)
{
    base::Initialize(title);

    // Buffers
    glGenBuffers(2, ray_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ray_buffer[0]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 64 * 1024 * 1024, NULL, GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ray_buffer[1]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 64 * 1024 * 1024, NULL, GL_DYNAMIC_COPY);

    static const GLuint zeros[] = { 0, 0 };

    glGenBuffers(1, &counter_buffer);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counter_buffer);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(zeros), zeros, GL_DYNAMIC_COPY);

    // Initialize our compute program
    initializer_prog = glCreateProgram();

    static const char initialization_shader_source[] =
        "#version 430 core\n"
        "\n"
        "layout (local_size_x = 16, local_size_y = 16) in;\n"
        "\n"
        RAY_ELEMENT
        "\n"
        "layout (std430, binding = 0) buffer ray_buffer\n"
        "{\n"
        "    RAY    ray[];\n"
        "};\n"
        "\n"
        "RAY initialize_ray()\n"
        "{\n"
        "    RAY r;\n"
        "    \n"
        "    r.screen_origin.xy = ivec2(gl_GlobalInvocationID.xy);\n"
        "    r.world_origin.xyz = vec3(0.0, 0.0, 0.0);\n"
        "    r.world_direction.xyz = normalize(vec3(vec2(gl_LocalInvocationID.xy), 100.0f));\n"
        "    \n"
        "    return r;\n"
        "}\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    ray[gl_LocalInvocationID.y * 16 + gl_LocalInvocationID.x] = initialize_ray();\n"
        "}\n"
    ;

    vglAttachShaderSource(initializer_prog, GL_COMPUTE_SHADER, initialization_shader_source);

    glLinkProgram(initializer_prog);

    static const char trace_shader_source[] =
    {
        "#version 430 core\n"
        "\n"
        "layout (local_size_x = 16, local_size_y = 16) in;\n"
        "\n"
        RAY_ELEMENT
        "\n"
        "layout (std430, binding = 0) buffer input_ray_buffer\n"
        "{\n"
        "    RAY    ray[];\n"
        "} input_buffer;\n"
        "\n"
        "layout (std430, binding = 1) buffer output_ray_buffer\n"
        "{\n"
        "    RAY    ray[];\n"
        "} output_buffer;\n"
        "\n"
        "layout (binding = 0, offset = 0) uniform atomic_uint consume_counter;\n"
        "layout (binding = 0, offset = 4) uniform atomic_uint append_counter;\n"
        "\n"
        "layout (rgba32f, binding = 0) uniform image2D output_image;\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    uint ray_index = atomicCounterIncrement(consume_counter);\n"
        "\n"
        "    RAY input_ray = input_buffer.ray[ray_index];\n"
        "\n"
//        "    barrier();\n"
        "\n"
        "    {\n"
        "        imageStore(output_image, input_ray.screen_origin.xy, vec4(input_ray.world_direction.xyz, 1.0));\n"
        "        ray_index = atomicCounterIncrement(append_counter);\n"
        "        input_ray.world_direction.x += 0.01;\n"
        "        output_buffer.ray[ray_index] = input_ray;\n"
        "        ray_index = atomicCounterIncrement(append_counter);\n"
        "        input_ray.world_direction.y += 0.01;\n"
        "        output_buffer.ray[ray_index] = input_ray;\n"
        "    }\n"
        "}\n"
    };
    
    // Initialize our compute program
    trace_prog = glCreateProgram();

    vglAttachShaderSource(trace_prog, GL_COMPUTE_SHADER, trace_shader_source);

    glLinkProgram(trace_prog);

    // This is the texture that the compute program will write into
    glGenTextures(1, &output_image);
    glBindTexture(GL_TEXTURE_2D, output_image);
    glTexStorage2D(GL_TEXTURE_2D, OUTPUT_LODS, GL_RGBA32F, OUTPUT_SIZE_X, OUTPUT_SIZE_Y);

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

#pragma pack (push, 1)
struct RAY
{
    vmath::ivec4    screen_origin;
    vmath::vec4     world_origin;
    vmath::vec4     world_direction;
};
#pragma pack (pop)

void RayTracingExample::Display(bool auto_redraw)
{
    // Activate the initialization compute program
    glUseProgram(initializer_prog);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ray_buffer[0]);
    glDispatchCompute(OUTPUT_SIZE_X / 16, OUTPUT_SIZE_Y / 16, 1);

    // Reset atomic counters
    static const GLuint zeros[] = { 0, 0 };

    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, counter_buffer);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(zeros), zeros, GL_DYNAMIC_COPY);

    // 
    glBindImageTexture(0, output_image, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

    // Bind ray buffers
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ray_buffer[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ray_buffer[1]);

    // Bind the trace program
    glUseProgram(trace_prog);
    glDispatchCompute(OUTPUT_SIZE_X / 16, OUTPUT_SIZE_Y / 16, 1);

    vmath::ivec2 * ptr = (vmath::ivec2 *)glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_READ_ONLY);

    glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);

    // Now bind the texture for rendering _from_
    glBindTexture(GL_TEXTURE_2D, output_image);

    // Clear, select the rendering program and draw a full screen quad
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(render_prog);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    base::Display();
}

void RayTracingExample::Finalize(void)
{
    glUseProgram(0);
    glDeleteProgram(initializer_prog);
    glDeleteProgram(render_prog);
    glDeleteTextures(1, &output_image);
    glDeleteVertexArrays(1, &render_vao);
}

void RayTracingExample::Resize(int width, int height)
{
    glViewport(0, 0, width, height);
}
