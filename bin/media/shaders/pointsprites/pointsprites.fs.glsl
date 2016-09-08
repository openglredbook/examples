#version 330

uniform sampler2D sprite_texture;

out vec4 color;

void main(void)
{
    color = texture(sprite_texture, vec2(gl_PointCoord.x, 1.0 - gl_PointCoord.y));
}
