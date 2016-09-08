//////////////////////////////////////////////////////////////////////////////
//
//  Triangles.cpp
//
//////////////////////////////////////////////////////////////////////////////

#include <vermilion.h>
#include "vapp.h"
#include "LoadShaders.h"

enum VAO_IDs { Triangles, NumVAOs };
enum Buffer_IDs { ArrayBuffer, NumBuffers };
enum Attrib_IDs { vPosition = 0, vColor = 1 };

GLuint  VAOs[NumVAOs];
GLuint  Buffers[NumBuffers];

const GLuint  NumVertices = 6;

BEGIN_APP_DECLARATION(GouraudFloatExample)
    virtual void Initialize(const char * title);
    virtual void Display(bool auto_redraw);
    virtual void OnKey(int key, int scancode, int action, int mods);
END_APP_DECLARATION()

DEFINE_APP(GouraudFloatExample, "Gouraud (Float) Example")

//----------------------------------------------------------------------------
//
// init
//

void GouraudFloatExample::Initialize(const char * title)
{
    base::Initialize(title);

    glGenVertexArrays( NumVAOs, VAOs );
    glBindVertexArray( VAOs[Triangles] );

    struct VertexData {
        GLfloat color[3];
        GLfloat position[4];
    };

    VertexData vertices[NumVertices] = {
        {{  1.00f, 0.00f, 0.00f }, { -0.90f, -0.90f }},  // Triangle 1
        {{  0.00f, 1.00f, 0.00f }, {  0.85f, -0.90f }},
        {{  0.00f, 0.00f, 1.00f }, { -0.90f,  0.85f }},
        {{  0.04f, 0.04f, 0.04f }, {  0.90f, -0.85f }},  // Triangle 2
        {{  0.40f, 0.40f, 0.40f }, {  0.90f,  0.90f }},
        {{  1.00f, 1.00f, 1.00f }, { -0.85f,  0.90f }}
    };

    glGenBuffers( NumBuffers, Buffers );
    glBindBuffer( GL_ARRAY_BUFFER, Buffers[ArrayBuffer] );
    glBufferData( GL_ARRAY_BUFFER, sizeof(vertices),
                  vertices, GL_STATIC_DRAW );

    ShaderInfo  shaders[] = {
        { GL_VERTEX_SHADER, "media/shaders/gouraud/gouraud.vert" },
        { GL_FRAGMENT_SHADER, "media/shaders/gouraud/gouraud.frag" },
        { GL_NONE, NULL }
    };

    GLuint program = LoadShaders( shaders );
    glUseProgram( program );

    glVertexAttribPointer( vColor, 3, GL_FLOAT,
                           GL_TRUE, sizeof(VertexData), BUFFER_OFFSET(0) );
    glVertexAttribPointer( vPosition, 2, GL_FLOAT,
                           GL_FALSE, sizeof(VertexData),
                           BUFFER_OFFSET(sizeof(vertices[0].color)) );

    glEnableVertexAttribArray( vColor );
    glEnableVertexAttribArray( vPosition );
}

void GouraudFloatExample::OnKey(int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        switch (key)
        {
            case GLFW_KEY_M:
            {
                static GLenum mode = GL_FILL;

                mode = (mode == GL_FILL ? GL_LINE : GL_FILL);
                glPolygonMode(GL_FRONT_AND_BACK, mode);
                return;
            }
        }
    }

    base::OnKey(key, scancode, action, mods);
}

//----------------------------------------------------------------------------
//
// display
//

void GouraudFloatExample::Display(bool auto_redraw)
{
    glClear( GL_COLOR_BUFFER_BIT );

    glBindVertexArray( VAOs[Triangles] );
    glDrawArrays( GL_TRIANGLES, 0, NumVertices );

    // Done
    base::Display();
}
