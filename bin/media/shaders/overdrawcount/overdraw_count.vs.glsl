#version 430 core

layout (location = 0) in vec4 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 map1;

uniform mat4 model_matrix;
uniform mat4 view_matrix;
uniform mat4 projection_matrix;

uniform float aspect;
uniform float time;

out gl_PerVertex
{
    vec4 gl_Position;
};

out vec4 surface_color;
out vec3 frag_position;
out vec3 frag_normal;

void main(void)
{
    vec4 offset = vec4(float(gl_InstanceID & 7) * 2.0,
                       float((gl_InstanceID >> 3) & 7) * 2.0,
                       float((gl_InstanceID >> 6) & 7) * 2.0, 0.0) -
                  vec4(8.0, 8.0, 8.0, 0.0);

    surface_color = normalize(offset) * 0.5 + vec4(0.5, 0.5, 0.5, 0.4);

    vec4 object_pos = (position + offset);
    vec4 world_pos = model_matrix * object_pos;
    frag_position = world_pos.xyz;
    frag_normal = mat3(model_matrix * view_matrix) * normal;

    gl_Position = (projection_matrix * view_matrix) * world_pos;
}
