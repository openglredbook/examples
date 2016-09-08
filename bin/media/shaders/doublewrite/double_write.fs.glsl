#version 430 core

// Buffer containing a palette of colors to be used to mark primitives by ID
layout (binding = 0, rgba32f) uniform imageBuffer colors;

// The buffer that we will write to
layout (binding = 1, rgba32f) uniform image2D output_buffer;

out vec4 color;

void main(void)
{
    // Load a color from the palette buffer based on primitive ID % 256
    vec4 col = imageLoad(colors, gl_PrimitiveID & 255);

    // Store the resulting fragment at two locations. First at the fragments
    // window space coordinate shifted left...
    imageStore(output_buffer, ivec2(gl_FragCoord.xy) - ivec2(200, 0), col);

    // ... then at the location shifted right
    imageStore(output_buffer, ivec2(gl_FragCoord.xy) + ivec2(200, 0), col);
}
