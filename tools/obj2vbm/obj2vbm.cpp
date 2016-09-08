#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

#include <string>
#include <vector>
#include <algorithm>
#include <map>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
// #include <windows.h>
#endif

#define VBM_FILE_TYPES_ONLY
#include "vbm.h"

#define GL_NONE                     0x0000
#define GL_UNSIGNED_SHORT           0x1403
#define GL_UNSIGNED_INT             0x1405
#define GL_FLOAT                    0x1406

#define VBM_FLAG_HAS_VERTICES       0x00000001
#define VBM_FLAG_HAS_INDICES        0x00000002
#define VBM_FLAG_HAS_FRAMES         0x00000004
#define VBM_FLAG_HAS_MATERIALS      0x00000008

std::map<std::string, VBM_MATERIAL> materials;

void extract_vec3(const char * buf, VBM_VEC3F &v3)
{
    switch (sscanf(buf, "%*s %f %f %f", &v3.x, &v3.y, &v3.z))
    {
        case 1: v3.y = v3.z = v3.x;
        case 3: // yay!
            break;
        default: // bah!
            break;
    }
}

void replace_chars(char * str, char old_char, char new_char)
{
    char * p = strchr(str, old_char);

    while (p)
    {
        *p = new_char;
        p = strchr(p + 1, old_char);
    }
}

void parse_material_file(const char * filename)
{
    FILE * infile = fopen(filename, "rb");
    bool done = false;
    char buffer1[1024];
    char buffer2[1024];
    VBM_MATERIAL * mat = NULL;
    char * p;

    do
    {
        if (feof(infile))
            break;
        memset(buffer1, 0, sizeof(buffer1));
        fgets(buffer1, sizeof(buffer1), infile);

        buffer2[0] = 0;
        sscanf(buffer1, "%s", buffer2);

        if (buffer2[0] == 0 || buffer2[0] == '#')
            continue;

        if (!strcmp(buffer2, "newmtl"))
        {
            p = strchr(buffer1, ' ');
            if (!p)
                continue;

            sscanf(buffer1, "%*s %s", buffer2);

            if (materials.count(buffer2) == 0)
            {
                static const VBM_VEC3F ones = { 1.0f, 1.0f, 1.0f };
                mat = &materials[buffer2];
                memset(mat, 0, sizeof(*mat));
                mat->alpha = 1.0f;
                mat->diffuse = ones;
                strncpy(mat->name, buffer2, sizeof(mat->name) - 1);
            }
            else
            {
                mat = &materials[buffer2];
            }

            continue;
        }

        if (!mat)
            continue;

        if (!strcmp(buffer2, "Ns"))
        {
            sscanf(buffer1, "%*s %f", &mat->shininess);

            continue;
        }

        if (!strcmp(buffer2, "Ni"))
        {
            sscanf(buffer1, "%*s %f", &mat->ior);

            continue;
        }

        if (!strcmp(buffer2, "Tf"))
        {
            extract_vec3(buffer1, mat->transmission);

            continue;
        }

        if (!strcmp(buffer2, "d") || !strcmp(buffer2, "Tr"))
        {
            sscanf(buffer1, "%*s %f", &mat->alpha);

            continue;
        }

        if (!strcmp(buffer2, "Ka"))
        {
            extract_vec3(buffer1, mat->ambient);

            continue;
        }

        if (!strcmp(buffer2, "Kd"))
        {
            extract_vec3(buffer1, mat->diffuse);

            continue;
        }

        if (!strcmp(buffer2, "Ks"))
        {
            extract_vec3(buffer1, mat->specular);

            continue;
        }

        if (!strcmp(buffer2, "Ke"))
        {
            extract_vec3(buffer1, mat->specular_exp);

            continue;
        }

        if (!strcmp(buffer2, "map_Ka"))
        {
            memset(buffer2, 0, sizeof(mat->ambient_map));
            sscanf(buffer1, "%*s %s", buffer2);

            strncpy(mat->ambient_map, buffer2, sizeof(mat->ambient_map) - 1);
            replace_chars(mat->ambient_map, '\\', '/');

            continue;
        }

        if (!strcmp(buffer2, "map_Kd"))
        {
            memset(buffer2, 0, sizeof(mat->diffuse_map));
            sscanf(buffer1, "%*s %s", buffer2);

            strncpy(mat->diffuse_map, buffer2, sizeof(mat->diffuse_map) - 1);
            replace_chars(mat->diffuse_map, '\\', '/');

            continue;
        }

        if (!strcmp(buffer2, "map_bump") || !strcmp(buffer2, "bump"))
        {
            memset(buffer2, 0, sizeof(mat->diffuse_map));
            sscanf(buffer1, "%*s %s", buffer2);

            strncpy(mat->normal_map, buffer2, sizeof(mat->normal_map) - 1);
            replace_chars(mat->normal_map, '\\', '/');

            continue;
        }

    } while (!done);

    fclose(infile);
}

struct triangle
{
    unsigned int v_index;
    unsigned int t_index;
    unsigned int n_index;
    VBM_MATERIAL * material;

    triangle(unsigned int v, unsigned int t, unsigned int n, VBM_MATERIAL * m)
        : v_index(v), t_index(t), n_index(n), material(m) {}
};

int main(int argc, char ** argv)
{
    FILE * infile = fopen(argv[1], "rb");
    FILE * outfile;
    bool done = false;
    char buffer[1024];
    char buffer2[1024];
    VBM_VEC4F vec;
    VBM_VEC2F tc;
    int a, b, c;
    int index[32];
    int count;
    int n;
    char * p;
    VBM_MATERIAL * current_material = NULL;

    std::string objectname;
    std::vector<VBM_VEC4F> vertices;
    std::vector<VBM_VEC4F> normals;
    std::vector<VBM_VEC4F> texcoords;
    std::vector<unsigned int> indices;
    std::vector<unsigned int> normal_indices;
    std::vector<unsigned int> texcoord_indices;
    std::vector<triangle> triangles;

    if (argc >= 4 && argv[3] != NULL)
    {
        parse_material_file(argv[3]);
    }

    do {
        if (feof(infile))
            break;
        fgets(buffer, sizeof(buffer) - 1, infile);
        if (buffer[0] == '\n' || buffer[0] == '\r' || buffer[0] == 0)
            continue;
        sscanf(buffer, "%s", buffer2);
        if (buffer2[0] == '#') {
            continue;
        } else if (!strcmp(buffer2, "g") || !strcmp(buffer2, "o")) {
            objectname = buffer + 2;
        } else if (!strcmp(buffer2, "v")) {
            vec.x = vec.y = vec.z = 0.0f;
            vec.w = 1.0f;
            sscanf(buffer + 1, "%f %f %f", &vec.x, &vec.y, &vec.z, &vec.w);
            vertices.push_back(vec);
        } else if (!strcmp(buffer2, "vn")) {
            sscanf(buffer + 2, "%f %f %f\n", &vec.x, &vec.y, &vec.z);
            normals.push_back(vec);
        } else if (!strcmp(buffer2, "vt")) {
            sscanf(buffer + 2, "%f %f", &vec.x, &vec.y);
            texcoords.push_back(vec);
        } else if (!strcmp(buffer2, "f")) {

            count = sscanf(buffer + 1, "%d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d", &a, &b, &c, &index[0], &index[1], &index[2], &index[3], &index[4], &index[5], &index[6], &index[7], &index[8]);

            if (count >= 9)
            {
                for (n = 1; n < count / 3 - 1; n++)
                {
                    triangles.push_back(triangle(indices.size(), texcoord_indices.size(), normal_indices.size(), current_material));
                    indices.push_back(a - 1);
                    if (b < 0)
                    {
                        texcoord_indices.push_back(texcoords.size() + b);
                    }
                    else
                    {
                        texcoord_indices.push_back(b - 1);
                    }
                    if (c < 0)
                    {
                        normal_indices.push_back(normals.size() + c);
                    }
                    else
                    {
                        normal_indices.push_back(c - 1);
                    }

                    indices.push_back(index[n * 3 - 3] - 1);
                    if (index[n * 3 - 2] < 0)
                    {
                        texcoord_indices.push_back(texcoords.size() + index[n * 3 - 2]);
                    }
                    else
                    {
                        texcoord_indices.push_back(index[n * 3 - 2] - 1);
                    }
                    if (index[n * 3 - 1] < 0)
                    {
                        normal_indices.push_back(texcoords.size() + index[n * 3 - 1]);
                    }
                    else
                    {
                        normal_indices.push_back(index[n * 3 - 1] - 1);
                    }

                    indices.push_back(index[n * 3] - 1);
                    if (index[n * 3 + 1] < 0)
                    {
                        texcoord_indices.push_back(texcoords.size() + index[n * 3 + 1]);
                    }
                    else
                    {
                        texcoord_indices.push_back(index[n * 3 + 1] - 1);
                    }
                    if (index[n * 3 + 2] < 0)
                    {
                        normal_indices.push_back(texcoords.size() + index[n * 3 + 2]);
                    }
                    else
                    {
                        normal_indices.push_back(index[n * 3 + 2] - 1);
                    }

                }
                continue;
            }

            //*
            count = sscanf(buffer + 1, "%d/%d %d/%d %d/%d %d/%d", &a, &b, &index[0], &index[1], &index[2], &index[3], &index[4], &index[5]);
            if (count >= 6)
            {
                for (n = 1; n < count / 2 - 1; n++)
                {
                    triangles.push_back(triangle(indices.size(), texcoord_indices.size(), normal_indices.size(), current_material));
                    indices.push_back(a - 1);
                    if (b < 0)
                    {
                        texcoord_indices.push_back(texcoords.size() + b);
                    }
                    else
                    {
                        texcoord_indices.push_back(b - 1);
                    }
                    indices.push_back(index[n * 2 - 2] - 1);
                    if (index[n * 2 - 1] < 0)
                    {
                        texcoord_indices.push_back(texcoords.size() + index[n * 2 - 1]);
                    }
                    else
                    {
                        texcoord_indices.push_back(index[n * 2 - 1] - 1);
                    }
                    indices.push_back(index[n * 2] - 1);
                    if (index[n * 2 + 1] < 0)
                    {
                        texcoord_indices.push_back(texcoords.size() + index[n * 2 + 1]);
                    }
                    else
                    {
                        texcoord_indices.push_back(index[n * 2 + 1] - 1);
                    }
                }
                continue;
            }
            //*/

            /*
            count = sscanf(buffer + 1, "%d//%d %d//%d %d//%d %d//%d", &a, &b, &index[0], &index[1], &index[2], &index[3], &index[4], &index[5]);
            if (count >= 6)
            {
                for (n = 1; n < count / 2 - 1; n++)
                {
                    indices.push_back(a - 1);
                    indices.push_back(index[n * 2 - 2] - 1);
                    indices.push_back(index[n * 2] - 1);
                }
                continue;
            }
            //*/

            //*
            unsigned int verts_this_poly = 0;

            p = strchr(buffer, ' ') + 1;

            count = sscanf(p, "%d %d", &a, &b);

            if (count == 2)
            {
                verts_this_poly = 2;
                p = strchr(p, ' ') + 1;
                p = strchr(p, ' ');
                do {
                    p++;
                    sscanf(p, "%d", &c);
                    triangles.push_back(triangle(indices.size(), texcoord_indices.size(), normal_indices.size(), current_material));
                    indices.push_back(a - 1);
                    indices.push_back(b - 1);
                    indices.push_back(c - 1);
                    normal_indices.push_back(0);
                    normal_indices.push_back(0);
                    normal_indices.push_back(0);
                    texcoord_indices.push_back(0);
                    texcoord_indices.push_back(0);
                    texcoord_indices.push_back(0);
                    p = strchr(p, ' ');
                    b = c;
                    verts_this_poly++;
                } while (p && verts_this_poly < 5);
            }
            /*
            count = sscanf(buffer + 1, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
                                        &a,
                                        &index[0], &index[1], &index[2], &index[3], &index[4], &index[5], &index[6], &index[7], &index[8], &index[9],
                                        &index[10], &index[11], &index[12], &index[13], &index[14], &index[15], &index[16], &index[17], &index[18], &index[19],
                                        &index[20], &index[21], &index[22], &index[23], &index[24], &index[25], &index[26], &index[27], &index[28], &index[29],
                                        &index[30], &index[31]);

            if (count >= 3)
            {
                if (count == 32)
                    count = 3;
                for (n = 1; n < count - 1; n++) {
                    indices.push_back(a - 1);
                    indices.push_back(index[n - 1] - 1);
                    indices.push_back(index[n] - 1);
                    texcoord_indices.push_back(a - 1);
                    texcoord_indices.push_back(index[n - 1] - 1);
                    texcoord_indices.push_back(index[n] - 1);
                }
                continue;
            }
            //*/
        } else if (!strcmp(buffer2, "usemtl"))
        {
            p = strchr(buffer, ' ');
            if (!p)
                continue;

            sscanf(buffer, "%*s %s", buffer2);

            if (materials.count(buffer2))
            {
                VBM_MATERIAL& material = materials[buffer2];
                current_material = &material;

                if (material.name[0] == 0)
                {
                    strncpy(material.name, p, sizeof(material.name) - 1);
                }
            }
            else
            {
                current_material = 0;
            }
            continue;
        } else
        {
            count = 42;
        }
    } while (!done);

    fclose(infile);

    struct comparator
    {
        inline bool operator() (triangle &a, triangle& b)
        {
            if (a.material == NULL || b.material == NULL)
                return false;

            const char * a_name = a.material->name;
            const char * b_name = b.material->name;
            return strcmp(a_name, b_name) > 0;
        }
    } compare;

    std::sort(triangles.begin(), triangles.end(), compare);

    VBM_MATERIAL material_array[256];

    unsigned int material_index = 0;
    unsigned int material_count;
    for (auto it = materials.begin(); it != materials.end(); it++)
    {
        material_array[material_index] = it->second;
        material_index++;
    }

    material_count = material_index;
    VBM_RENDER_CHUNK chunks[256];
    unsigned int chunk_start = 0;
    unsigned int chunk_length = 0;
    VBM_MATERIAL * mat = NULL;
    VBM_RENDER_CHUNK *chunk = chunks;
    chunk->first = chunk->count = 0;

    for (auto tri = triangles.begin(); tri != triangles.end(); tri++)
    {
        if (tri->material != mat)
        {
            material_index = 0;
            for (material_index = 0; material_index < material_count; material_index++)
            {
                if (!strcmp(tri->material->name, material_array[material_index].name))
                    break;
            }

            if (mat == 0)
            {
                chunk->material_index = material_index;
            }

            if (chunk->count != 0 && mat != 0)
            {
                chunk++;
                chunk->first = (chunk - 1)->first + (chunk - 1)->count;
                chunk->count = 0;
                chunk->material_index = material_index;
            }
            mat = tri->material;
        }
        chunk->count += 3;
    }

    if (chunk->count != 0)
    {
        chunk++;
    }

    std::vector<unsigned int> real_vertex_indices;
    std::vector<unsigned int> real_texcoord_indices;
    std::vector<unsigned int> real_normal_indices;

    for (auto triangle = triangles.begin(); triangle != triangles.end(); triangle++)
    {
        real_vertex_indices.push_back(indices[triangle->v_index]);
        real_vertex_indices.push_back(indices[triangle->v_index + 1]);
        real_vertex_indices.push_back(indices[triangle->v_index + 2]);
        real_texcoord_indices.push_back(texcoord_indices[triangle->t_index]);
        real_texcoord_indices.push_back(texcoord_indices[triangle->t_index + 1]);
        real_texcoord_indices.push_back(texcoord_indices[triangle->t_index + 2]);
        real_normal_indices.push_back(normal_indices[triangle->n_index]);
        real_normal_indices.push_back(normal_indices[triangle->n_index + 1]);
        real_normal_indices.push_back(normal_indices[triangle->n_index + 2]);
    }

    unsigned int num_attribs = 0;
    if (vertices.size() != 0)
        num_attribs++;
    if (normals.size() != 0)
        num_attribs++;
    if (texcoords.size() != 0)
        num_attribs++;

    size_t i;
    bool can_do_indexed = true;
    unsigned int max_index = 0;

    if (indices.size() != texcoord_indices.size() || indices.size() != normal_indices.size())
        can_do_indexed = false;
    else
    {
        for (i = 0; i < indices.size(); i++)
        {
            if (indices[i] != texcoord_indices[i])
            {
                can_do_indexed = false;
                break;
            }
            if (indices[i] > max_index)
                max_index = indices[i];
        }
    }

    outfile = fopen(argv[2], "wb");

    VBM_HEADER file_header;

    memset(&file_header, 0, sizeof(file_header));
    file_header.magic = '1MBS';
    strncpy(file_header.name, objectname.c_str(), sizeof(file_header.name) - 1);
    file_header.size = sizeof(file_header);
    file_header.num_attribs = num_attribs;
    file_header.num_frames = 1;
    if (can_do_indexed)
    {
        file_header.num_vertices = vertices.size();
        file_header.num_indices = indices.size();
        file_header.index_type = max_index > 0xFFFF ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
    }
    else
    {
        file_header.num_vertices = indices.size();
        file_header.num_indices = 0;
        file_header.index_type = GL_NONE;
    }
    if (materials.size() != 0) {
        file_header.flags |= VBM_FLAG_HAS_MATERIALS;
        file_header.num_materials = materials.size();
    }
    file_header.num_chunks = (unsigned int)(chunk - &chunks[0]);

    fwrite(&file_header, sizeof(file_header), 1, outfile);

    VBM_ATTRIB_HEADER attrib_header;

    if (vertices.size() != 0) {
        memset(&attrib_header, 0, sizeof(attrib_header));
        sprintf(attrib_header.name, "position");
        attrib_header.type = GL_FLOAT;
        attrib_header.components = 3;
        attrib_header.flags = 0;
        fwrite(&attrib_header, sizeof(attrib_header), 1, outfile);
    }

    if (normals.size() != 0) {
        memset(&attrib_header, 0, sizeof(attrib_header));
        sprintf(attrib_header.name, "normal");
        attrib_header.type = GL_FLOAT;
        attrib_header.components = 3;
        attrib_header.flags = 0;
        fwrite(&attrib_header, sizeof(attrib_header), 1, outfile);
    }

    if (texcoords.size() != 0) {
        memset(&attrib_header, 0, sizeof(attrib_header));
        sprintf(attrib_header.name, "texcoord");
        attrib_header.type = GL_FLOAT;
        attrib_header.components = 2;
        attrib_header.flags = 0;
        fwrite(&attrib_header, sizeof(attrib_header), 1, outfile);
    }

    VBM_FRAME_HEADER frame_header;

    memset(&frame_header, 0, sizeof(frame_header));
    frame_header.first = 0;
    frame_header.count = indices.size();
    fwrite(&frame_header, sizeof(frame_header), 1, outfile);

    std::vector<VBM_VEC4F>::iterator vert;

    VBM_VEC4F min_vec;
    VBM_VEC4F max_vec;

    min_vec.x = min_vec.y = min_vec.z = 10000000.0f;
    max_vec.x = max_vec.y = max_vec.z = -10000000.0f;

    for (vert = vertices.begin(); vert != vertices.end(); vert++) {
        if (vert->x < min_vec.x)
            min_vec.x = vert->x;
        if (vert->y < min_vec.y)
            min_vec.y = vert->y;
        if (vert->z < min_vec.z)
            min_vec.z = vert->z;
        if (vert->x > max_vec.x)
            max_vec.x = vert->x;
        if (vert->y > max_vec.y)
            max_vec.y = vert->y;
        if (vert->z > max_vec.z)
            max_vec.z = vert->z;
    }

    size_t pos;

    VBM_VEC4F mean_vec;
    mean_vec.x = (max_vec.x + min_vec.x) * 0.5f;
    mean_vec.y = (max_vec.y + min_vec.y) * 0.5f;
    mean_vec.z = (max_vec.z + min_vec.z) * 0.5f;

    if (can_do_indexed)
    {
        for (vert = vertices.begin(); vert != vertices.end(); vert++) {
            vec = *vert;
            fwrite(&vec, sizeof(float), 3, outfile);
        }

        for (vert = normals.begin(); vert != normals.end(); vert++) {
            vec = *vert;
            fwrite(&vec, sizeof(float), 3, outfile);
        }

        for (vert = texcoords.begin(); vert != texcoords.end(); vert++)
        {
            tc.x = vert->x;
            tc.y = vert->y;
            fwrite(&tc, sizeof(float), 2, outfile);
        }

        fwrite(&indices[0], sizeof(int), indices.size(), outfile);
    }
    else
    {
        std::vector<VBM_VEC3F> vertex_data;
        for (i = 0; i < real_vertex_indices.size(); i++)
        {
            VBM_VEC3F v;
            v.x = vertices[real_vertex_indices[i]].x;// - mean_vec.x;
            v.y = vertices[real_vertex_indices[i]].y;// - mean_vec.y;
            v.z = vertices[real_vertex_indices[i]].z;// - mean_vec.z;
            vertex_data.push_back(v);
        }
        fwrite(&vertex_data[0], vertex_data.size(), sizeof(vertex_data[0]), outfile);

        std::vector<VBM_VEC3F> normal_data;
        for (i = 0; i < real_normal_indices.size(); i++)
        {
            VBM_VEC3F v;
            n = real_normal_indices[i];
            if (n < normals.size())
            {
                v.x = normals[n].x;
                v.y = normals[n].y;
                v.z = normals[n].z;
            }
            else
            {
                v.x = v.y = v.z = 0.0f;
            }
            normal_data.push_back(v);
        }
        fwrite(&normal_data[0], normal_data.size(), sizeof(normal_data[0]), outfile);

        std::vector<VBM_VEC2F> texcoord_data;
        for (i = 0; i < real_texcoord_indices.size(); i++)
        {
            VBM_VEC2F v;
            n = real_texcoord_indices[i];
            if (n < texcoords.size())
            {
                v.x = texcoords[n].x;
                v.y = texcoords[n].y;
            }
            else
            {
                v.x = v.y = 0.0f;
            }
            texcoord_data.push_back(v);
        }
        fwrite(&texcoord_data[0], texcoord_data.size(), sizeof(texcoord_data[0]), outfile);
    }

    for (auto it = materials.begin(); it != materials.end(); it++)
    {
        VBM_MATERIAL &mat = it->second;
        fwrite(&mat, sizeof(VBM_MATERIAL), 1, outfile);
    }

    fwrite(chunks, sizeof(*chunk), chunk - &chunks[0], outfile);

    fclose(outfile);
}
