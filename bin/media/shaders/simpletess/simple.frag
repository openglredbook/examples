#version 400 core

out  vec4 fColor;

void
main()
{
    fColor = (1 - gl_FragCoord.z) * vec4( 1.0, 0.0, 1.0, 1.0 );
}
