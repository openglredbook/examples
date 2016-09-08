#version 400 core

layout (vertices = 16) out;

uniform float  Inner;
uniform float  Outer;

void
main()
{
    gl_TessLevelInner[0] = Inner;
    gl_TessLevelInner[1] = Inner;
    gl_TessLevelOuter[0] = Outer;
    gl_TessLevelOuter[1] = Outer;
    gl_TessLevelOuter[2] = Outer;
    gl_TessLevelOuter[3] = Outer;

    gl_out[gl_InvocationID].gl_Position =
	gl_in[gl_InvocationID].gl_Position;
}
