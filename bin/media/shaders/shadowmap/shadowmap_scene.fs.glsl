#version 330

uniform sampler2DShadow depth_texture;
uniform vec3 light_position;

uniform vec3 material_ambient;
uniform vec3 material_diffuse;
uniform vec3 material_specular;
uniform float material_specular_power;

layout (location = 0) out vec4 color;

in VS_FS_INTERFACE
{
    vec4 shadow_coord;
    vec3 world_coord;
    vec3 eye_coord;
    vec3 normal;
} fragment;

void main(void)
{
    vec3 N = fragment.normal;
    vec3 L = normalize(light_position - fragment.world_coord);
    float LdotN = dot(N, L);
    vec3 R = reflect(-L, N);

    float diffuse = max(LdotN, 0.0);
    float specular = max(pow(dot(normalize(-fragment.eye_coord), R), material_specular_power), 0.0);

    float f = textureProj(depth_texture, fragment.shadow_coord);

    color = vec4(material_ambient + f * (material_diffuse * diffuse + material_specular * specular), 1.0);
}
