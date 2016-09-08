/* $URL$
   $Rev$
   $Author$
   $Date$
   $Id$
 */

// #define USE_GL3W
#include <vermilion.h>

#include "vgl.h"
#include "vapp.h"
#include "vutils.h"
#include "vbm.h"

#include "vmath.h"

#include <stdio.h>

enum
{
    PARTICLE_GROUP_SIZE     = 1024,
    PARTICLE_GROUP_COUNT    = 8192,
    PARTICLE_COUNT          = (PARTICLE_GROUP_SIZE * PARTICLE_GROUP_COUNT),
    MAX_ATTRACTORS          = 64
};

BEGIN_APP_DECLARATION(ComputeParticleSimulator)
    // Override functions from base class
    virtual void Initialize(const char * title);
    virtual void Display(bool auto_redraw);
    virtual void Finalize(void);
    virtual void Resize(int width, int height);

    // Compute program
    GLuint  compute_prog;
    GLint   dt_location;

    // Posisition and velocity buffers
    union
    {
        struct
        {
            GLuint position_buffer;
            GLuint velocity_buffer;
        };
        GLuint buffers[2];
    };

    // TBOs
    union
    {
        struct
        {
            GLuint position_tbo;
            GLuint velocity_tbo;
        };
        GLuint tbos[2];
    };

    // Attractor UBO
    GLuint  attractor_buffer;

    // Program, vao and vbo to render a full screen quad
    GLuint  render_prog;
    GLuint  render_vao;
    GLuint  render_vbo;

    // Mass of the attractors
    float attractor_masses[MAX_ATTRACTORS];

    float aspect_ratio;
END_APP_DECLARATION()

DEFINE_APP(ComputeParticleSimulator, "Compute Shader Particle System")

#define STRINGIZE(a) #a

static inline float random_float()
{
    float res;
    unsigned int tmp;
    static unsigned int seed = 0xFFFF0C59;

    seed *= 16807;

    tmp = seed ^ (seed >> 4) ^ (seed << 15);

    *((unsigned int *) &res) = (tmp >> 9) | 0x3F800000;

    return (res - 1.0f);
}

static vmath::vec3 random_vector(float minmag = 0.0f, float maxmag = 1.0f)
{
    vmath::vec3 randomvec(random_float() * 2.0f - 1.0f, random_float() * 2.0f - 1.0f, random_float() * 2.0f - 1.0f);
    randomvec = normalize(randomvec);
    randomvec *= (random_float() * (maxmag - minmag) + minmag);

    return randomvec;
}

void ComputeParticleSimulator::Initialize(const char * title)
{
    base::Initialize(title);

    int i;

    // Initialize our compute program
    compute_prog = glCreateProgram();

    static const char compute_shader_source[] =
        STRINGIZE(
#version 430 core\n

layout (std140, binding = 0) uniform attractor_block
{
    vec4 attractor[64]; // xyz = position, w = mass
};

layout (local_size_x = 1024) in;

layout (rgba32f, binding = 0) uniform imageBuffer velocity_buffer;
layout (rgba32f, binding = 1) uniform imageBuffer position_buffer;

uniform float dt = 1.0;

void main(void)
{
    vec4 vel = imageLoad(velocity_buffer, int(gl_GlobalInvocationID.x));
    vec4 pos = imageLoad(position_buffer, int(gl_GlobalInvocationID.x));

    int i;

    pos.xyz += vel.xyz * dt;
    pos.w -= 0.0001 * dt;

    for (i = 0; i < 4; i++)
    {
        vec3 dist = (attractor[i].xyz - pos.xyz);
        vel.xyz += dt * dt * attractor[i].w * normalize(dist) / (dot(dist, dist) + 10.0);
    }

    if (pos.w <= 0.0)
    {
        pos.xyz = -pos.xyz * 0.01;
        vel.xyz *= 0.01;
        pos.w += 1.0f;
    }

    imageStore(position_buffer, int(gl_GlobalInvocationID.x), pos);
    imageStore(velocity_buffer, int(gl_GlobalInvocationID.x), vel);
}
        );

    vglAttachShaderSource(compute_prog, GL_COMPUTE_SHADER, compute_shader_source);

    glLinkProgram(compute_prog);

    dt_location = glGetUniformLocation(compute_prog, "dt");

    glGenVertexArrays(1, &render_vao);
    glBindVertexArray(render_vao);

    glGenBuffers(2, buffers);
    glBindBuffer(GL_ARRAY_BUFFER, position_buffer);
    glBufferData(GL_ARRAY_BUFFER, PARTICLE_COUNT * sizeof(vmath::vec4), NULL, GL_DYNAMIC_COPY);

    vmath::vec4 * positions = (vmath::vec4 *)glMapBufferRange(GL_ARRAY_BUFFER,
                                                              0,
                                                              PARTICLE_COUNT * sizeof(vmath::vec4),
                                                              GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

    for (i = 0; i < PARTICLE_COUNT; i++)
    {
        positions[i] = vmath::vec4(random_vector(-10.0f, 10.0f), random_float());
    }

    glUnmapBuffer(GL_ARRAY_BUFFER);

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, velocity_buffer);
    glBufferData(GL_ARRAY_BUFFER, PARTICLE_COUNT * sizeof(vmath::vec4), NULL, GL_DYNAMIC_COPY);

    vmath::vec4 * velocities = (vmath::vec4 *)glMapBufferRange(GL_ARRAY_BUFFER,
                                                               0,
                                                               PARTICLE_COUNT * sizeof(vmath::vec4),
                                                               GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

    for (i = 0; i < PARTICLE_COUNT; i++)
    {
        velocities[i] = vmath::vec4(random_vector(-0.1f, 0.1f), 0.0f);
    }

    glUnmapBuffer(GL_ARRAY_BUFFER);

    glGenTextures(2, tbos);

    for (i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_BUFFER, tbos[i]);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, buffers[i]);
    }

    glGenBuffers(1, &attractor_buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, attractor_buffer);
    glBufferData(GL_UNIFORM_BUFFER, 32 * sizeof(vmath::vec4), NULL, GL_STATIC_DRAW);

    for (i = 0; i < MAX_ATTRACTORS; i++)
    {
        attractor_masses[i] = 0.5f + random_float() * 0.5f;
    }

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, attractor_buffer);

    // Now create a simple program to visualize the result
    render_prog = glCreateProgram();

    static const char render_vs[] =
        "#version 430 core\n"
        "\n"
        "in vec4 vert;\n"
        "\n"
        "uniform mat4 mvp;\n"
        "\n"
        "out float intensity;\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    intensity = vert.w;\n"
        "    gl_Position = mvp * vec4(vert.xyz, 1.0);\n"
        "}\n";

    static const char render_fs[] =
        "#version 430 core\n"
        "\n"
        "layout (location = 0) out vec4 color;\n"
        "\n"
        "in float intensity;\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    color = mix(vec4(0.0f, 0.2f, 1.0f, 1.0f), vec4(0.2f, 0.05f, 0.0f, 1.0f), intensity);\n"
        "}\n";

    vglAttachShaderSource(render_prog, GL_VERTEX_SHADER, render_vs);
    vglAttachShaderSource(render_prog, GL_FRAGMENT_SHADER, render_fs);

    glLinkProgram(render_prog);
}

void ComputeParticleSimulator::Display(bool auto_redraw)
{
    static const GLuint start_ticks = app_time() - 100000;
    GLuint current_ticks = app_time();
    static GLuint last_ticks = current_ticks;
    float time = ((start_ticks - current_ticks) & 0xFFFFF) / float(0xFFFFF);
    float delta_time = (float)(current_ticks - last_ticks) * 0.075f;

    if (delta_time < 0.01f)
    {
        return;
    }

    vmath::vec4 * attractors = (vmath::vec4 *)glMapBufferRange(GL_UNIFORM_BUFFER,
                                                               0,
                                                               32 * sizeof(vmath::vec4),
                                                               GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

    int i;

    for (i = 0; i < 32; i++)
    {
        attractors[i] = vmath::vec4(sinf(time * (float)(i + 4) * 7.5f * 20.0f) * 50.0f,
                                    cosf(time * (float)(i + 7) * 3.9f * 20.0f) * 50.0f,
                                    sinf(time * (float)(i + 3) * 5.3f * 20.0f) * cosf(time * (float)(i + 5) * 9.1f) * 100.0f,
                                    attractor_masses[i]);
    }

    glUnmapBuffer(GL_UNIFORM_BUFFER);

    // If dt is too large, the system could explode, so cap it to
    // some maximum allowed value
    if (delta_time >= 2.0f)
    {
        delta_time = 2.0f;
    }

    // Activate the compute program and bind the position and velocity buffers
    glUseProgram(compute_prog);
    glBindImageTexture(0, velocity_tbo, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    glBindImageTexture(1, position_tbo, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    // Set delta time
    glUniform1f(dt_location, delta_time);
    // Dispatch
    glDispatchCompute(PARTICLE_GROUP_COUNT, 1, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    vmath::mat4 mvp = vmath::perspective(45.0f, aspect_ratio, 0.1f, 1000.0f) *
                      vmath::translate(0.0f, 0.0f, -160.0f) *
                      vmath::rotate(time * 1000.0f, vmath::vec3(0.0f, 1.0f, 0.0f));

    // Clear, select the rendering program and draw a full screen quad
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glUseProgram(render_prog);
    glUniformMatrix4fv(0, 1, GL_FALSE, mvp);
    glBindVertexArray(render_vao);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    // glPointSize(2.0f);
    glDrawArrays(GL_POINTS, 0, PARTICLE_COUNT);

    last_ticks = current_ticks;

    base::Display();
}

void ComputeParticleSimulator::Finalize(void)
{
    glUseProgram(0);
    glDeleteProgram(compute_prog);
    glDeleteProgram(render_prog);
    glDeleteVertexArrays(1, &render_vao);
}

void ComputeParticleSimulator::Resize(int width, int height)
{
    glViewport(0, 0, width, height);
    aspect_ratio = (float)width / (float)height;
}
