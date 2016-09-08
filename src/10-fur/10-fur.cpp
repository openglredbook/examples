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
#include <stdlib.h>

BEGIN_APP_DECLARATION(FurApplication)
    // Override functions from base class
    virtual void Initialize(const char * title);
    virtual void Display(bool auto_redraw);
    virtual void Finalize(void);
    virtual void Resize(int width, int height);

    // Member variables
    float aspect;
    GLuint base_prog;
    GLuint fur_prog;
    GLuint fur_texture;
    VBObject object;

    GLint fur_model_matrix_pos;
    GLint fur_projection_matrix_pos;
    GLint base_model_matrix_pos;
    GLint base_projection_matrix_pos;
END_APP_DECLARATION()

DEFINE_APP(FurApplication, "Fur Rendering")

void FurApplication::Initialize(const char * title)
{
    base::Initialize(title);

    base_prog = glCreateProgram();

    static const char base_vs_source[] =
        "#version 410\n"
        "\n"
        "layout (location = 0) in vec4 position_in;\n"
        "layout (location = 1) in vec3 normal_in;\n"
        "layout (location = 2) in vec2 texcoord_in;\n"
        "\n"
        "uniform mat4 model_matrix;\n"
        "uniform mat4 projection_matrix;\n"
        "\n"
        "out VS_FS_VERTEX\n"
        "{\n"
        "    vec3 normal;\n"
        "} vertex_out;\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    vertex_out.normal = normal_in;\n"
        "    gl_Position = projection_matrix * (model_matrix * position_in);\n"
        "}\n";

    static const char base_fs_source[] =
        "#version 410\n"
        "\n"
        "layout (location = 0) out vec4 color;\n"
        "\n"
        "in VS_FS_VERTEX\n"
        "{\n"
        "    vec3 normal;\n"
        "} vertex_in;\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    vec3 normal = vertex_in.normal;\n"
        "    color = vec4(0.2, 0.1, 0.5, 1.0) * (0.2 + pow(abs(normal.z), 4.0)) + vec4(0.8, 0.8, 0.8, 0.0) * pow(abs(normal.z), 137.0);\n"
        "}\n";

    vglAttachShaderSource(base_prog, GL_VERTEX_SHADER, base_vs_source);
    vglAttachShaderSource(base_prog, GL_FRAGMENT_SHADER, base_fs_source);

    glLinkProgram(base_prog);
    glUseProgram(base_prog);

    base_model_matrix_pos = glGetUniformLocation(base_prog, "model_matrix");
    base_projection_matrix_pos = glGetUniformLocation(base_prog, "projection_matrix");

    fur_prog = glCreateProgram();

    static const char fur_vs_source[] =
        "#version 410\n"
        "\n"
        "layout (location = 0) in vec4 position_in;\n"
        "layout (location = 1) in vec3 normal_in;\n"
        "layout (location = 2) in vec2 texcoord_in;\n"
        "\n"
        "out VS_GS_VERTEX\n"
        "{\n"
        "    vec3 normal;\n"
        "    vec2 tex_coord;\n"
        "} vertex_out;\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    vertex_out.normal = normal_in;\n"
        "    vertex_out.tex_coord = texcoord_in;\n"
        "    gl_Position = position_in;\n"
        "}\n";

    static const char fur_gs_source[] =
        "#version 410\n"
        "\n"
        "layout (triangles) in;\n"
        "layout (triangle_strip, max_vertices = 240) out;\n"
        "\n"
        "uniform mat4 model_matrix;\n"
        "uniform mat4 projection_matrix;\n"
        "\n"
        "uniform int fur_layers = 30;\n"
        "uniform float fur_depth = 5.0;\n"
        "\n"
        "in VS_GS_VERTEX\n"
        "{\n"
        "    vec3 normal;\n"
        "    vec2 tex_coord;\n"
        "} vertex_in[];\n"
        "\n"
        "out GS_FS_VERTEX\n"
        "{\n"
        "    vec3 normal;\n"
        "    vec2 tex_coord;\n"
        "    flat float fur_strength;\n"
        "} vertex_out;\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    int i, layer;\n"
        "    float disp_delta = 1.0 / float(fur_layers);\n"
        "    float d = 0.0;\n"
        "    vec4 position;\n"
        "\n"
        "    for (layer = 0; layer < fur_layers; layer++)\n"
        "    {\n"
        "        for (i = 0; i < gl_in.length(); i++) {\n"
        "            vec3 n = vertex_in[i].normal;\n"
        "            vertex_out.normal = n;\n"
        "            vertex_out.tex_coord = vertex_in[i].tex_coord;\n"
        "            vertex_out.fur_strength = 1.0 - d;\n"
        "            position = gl_in[i].gl_Position + vec4(n * d * fur_depth, 0.0);\n"
        "            gl_Position = projection_matrix * (model_matrix * position);\n"
        "            EmitVertex();\n"
        "        }\n"
        "        d += disp_delta;\n"
        "        EndPrimitive();\n"
        "    }\n"
        "}\n";

    static const char fur_fs_source[] =
        "#version 410\n"
        "\n"
        "layout (location = 0) out vec4 color;\n"
        "\n"
        "uniform sampler2D fur_texture;\n"
        "uniform vec4 fur_color = vec4(0.8, 0.8, 0.9, 1.0);\n"
        "\n"
        "in GS_FS_VERTEX\n"
        "{\n"
        "    vec3 normal;\n"
        "    vec2 tex_coord;\n"
        "    flat float fur_strength;\n"
        "} fragment_in;\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    vec4 rgba = texture(fur_texture, fragment_in.tex_coord);\n"
        "    float t = rgba.a;\n"
        "    t *= fragment_in.fur_strength;\n"
        "    color = fur_color * vec4(1.0, 1.0, 1.0, t);\n"
        "}\n";

    vglAttachShaderSource(fur_prog, GL_VERTEX_SHADER, fur_vs_source);
    vglAttachShaderSource(fur_prog, GL_GEOMETRY_SHADER, fur_gs_source);
    vglAttachShaderSource(fur_prog, GL_FRAGMENT_SHADER, fur_fs_source);

    glLinkProgram(fur_prog);
    glUseProgram(fur_prog);

    fur_model_matrix_pos = glGetUniformLocation(fur_prog, "model_matrix");
    fur_projection_matrix_pos = glGetUniformLocation(fur_prog, "projection_matrix");

    glGenTextures(1, &fur_texture);
    unsigned char * tex = (unsigned char *)malloc(1024 * 1024 * 4);
    memset(tex, 0, 1024 * 1024 * 4);

    int n, m;

    for (n = 0; n < 256; n++)
    {
        for (m = 0; m < 1270; m++)
        {
            int x = rand() & 0x3FF;
            int y = rand() & 0x3FF;
            tex[(y * 1024 + x) * 4 + 0] = (rand() & 0x3F) + 0xC0;
            tex[(y * 1024 + x) * 4 + 1] = (rand() & 0x3F) + 0xC0;
            tex[(y * 1024 + x) * 4 + 2] = (rand() & 0x3F) + 0xC0;
            tex[(y * 1024 + x) * 4 + 3] = n;
        }
    }

    glBindTexture(GL_TEXTURE_2D, fur_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    free(tex);

    object.LoadFromVBM("media/ninja.vbm", 0, 1, 2);
}

void FurApplication::Display(bool auto_redraw)
{
    float t = float(app_time() & 0x3FFF) / float(0x3FFF);
    static const vmath::vec3 X(1.0f, 0.0f, 0.0f);
    static const vmath::vec3 Y(0.0f, 1.0f, 0.0f);
    static const vmath::vec3 Z(0.0f, 0.0f, 1.0f);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    vmath::mat4 p(vmath::frustum(-1.0f, 1.0f, aspect, -aspect, 1.0f, 5000.0f));
    vmath::mat4 m;

    m = vmath::mat4(vmath::translate(0.0f,
                                       0.0f,
                                       /* 100.0f * sinf(6.28318531f * t)*/ - 130.0f) *
                    //vmath::rotation(360.0f * t, X) *
                    vmath::rotate(360.0f * t * 1.0f, Y) *
                    vmath::rotate(180.0f, Z) *
                    vmath::translate(0.0f, -80.0f, 0.0f));

    glUseProgram(base_prog);

    glUniformMatrix4fv(base_model_matrix_pos, 1, GL_FALSE, m[0]);
    glUniformMatrix4fv(base_projection_matrix_pos, 1, GL_FALSE, p);

    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    object.Render();

    glUseProgram(fur_prog);

    glUniformMatrix4fv(fur_model_matrix_pos, 1, GL_FALSE, m[0]);
    glUniformMatrix4fv(fur_projection_matrix_pos, 1, GL_FALSE, p);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDepthMask(GL_FALSE);

    object.Render();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    base::Display();
}

void FurApplication::Finalize(void)
{
    glUseProgram(0);
    glDeleteProgram(fur_prog);
}

void FurApplication::Resize(int width, int height)
{
    glViewport(0, 0 , width, height);

    aspect = float(height) / float(width);
}
