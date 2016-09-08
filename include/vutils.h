#ifndef __VUTILS_H__
#define __VUTILS_H__

#include "vgl.h"

void vglAttachShaderSource(GLuint prog, GLenum type, const char * source)
{
    GLuint sh;

    sh = glCreateShader(type);
    glShaderSource(sh, 1, &source, NULL);
    glCompileShader(sh);
    char buffer[4096];
    glGetShaderInfoLog(sh, sizeof(buffer), NULL, buffer);
    glAttachShader(prog, sh);
    glDeleteShader(sh);
}

#endif /* __VUTILS_H__ */
