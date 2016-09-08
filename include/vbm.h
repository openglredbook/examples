#ifndef __VBM_H__
#define __VBM_H__

// Define VBM_FILE_TYPES_ONLY before including this file to only include
// definitions of types used in VBM files. This can be used to create
// loaders/converters/exporters that have no dependencies outside this
// file. Note that in that case, gl.h doesn't get included and so in order
// to include some of the tokens required by the files (GL_UNSIGNED_INT,
// for example), you'll need to define them yourself.
#ifndef VBM_FILE_TYPES_ONLY
  #include "vgl.h"
  #include "vmath.h"
#endif

#define VBM_FLAG_HAS_VERTICES       0x00000001
#define VBM_FLAG_HAS_INDICES        0x00000002
#define VBM_FLAG_HAS_FRAMES         0x00000004
#define VBM_FLAG_HAS_MATERIALS      0x00000008

typedef struct VBM_HEADER_t
{
    unsigned int magic;
    unsigned int size;
    char name[64];
    unsigned int num_attribs;
    unsigned int num_frames;
    // unsigned int num_chunks;
    unsigned int num_vertices;
    unsigned int num_indices;
    unsigned int index_type;
    unsigned int num_materials;
    unsigned int flags;
} VBM_HEADER;

typedef struct VBM_HEADER_OLD_t
{
    unsigned int magic;
    unsigned int size;
    char name[64];
    unsigned int num_attribs;
    unsigned int num_frames;
    unsigned int num_chunks;
    unsigned int num_vertices;
    unsigned int num_indices;
    unsigned int index_type;
    unsigned int num_materials;
    unsigned int flags;
} VBM_HEADER_OLD;

typedef struct VBM_ATTRIB_HEADER_t
{
    char name[64];
    unsigned int type;
    unsigned int components;
    unsigned int flags;
} VBM_ATTRIB_HEADER;

typedef struct VBM_FRAME_HEADER_t
{
    unsigned int first;
    unsigned int count;
    unsigned int flags;
} VBM_FRAME_HEADER;

typedef struct VBM_RENDER_CHUNK_t
{
    unsigned int material_index;
    unsigned int first;
    unsigned int count;
} VBM_RENDER_CHUNK;

typedef struct VBM_VEC4F_t
{
    float x;
    float y;
    float z;
    float w;
} VBM_VEC4F;

typedef struct VBM_VEC3F_t
{
    float x;
    float y;
    float z;
} VBM_VEC3F;

typedef struct VBM_VEC2F_t
{
    float x;
    float y;
} VBM_VEC2F;

typedef struct VBM_MATERIAL_t
{
    char name[32];              /// Name of material
    VBM_VEC3F ambient;          /// Ambient color
    VBM_VEC3F diffuse;          /// Diffuse color
    VBM_VEC3F specular;         /// Specular color
    VBM_VEC3F specular_exp;     /// Specular exponent
    float shininess;            /// Shininess
    float alpha;                /// Alpha (transparency)
    VBM_VEC3F transmission;     /// Transmissivity
    float ior;                  /// Index of refraction (optical density)
    char ambient_map[64];       /// Ambient map (texture)
    char diffuse_map[64];       /// Diffuse map (texture)
    char specular_map[64];      /// Specular map (texture)
    char normal_map[64];        /// Normal map (texture)
} VBM_MATERIAL;

#ifndef VBM_FILE_TYPES_ONLY

class VBObject
{
public:
    VBObject(void);
    virtual ~VBObject(void);

    bool LoadFromVBM(const char * filename, int vertexIndex, int normalIndex, int texCoord0Index);
    void Render(unsigned int frame_index = 0, unsigned int instances = 0);
    bool Free(void);

    unsigned int GetVertexCount(unsigned int frame = 0)
    {
        return frame < m_header.num_frames ? m_frame[frame].count : 0;
    }

    unsigned int GetAttributeCount(void) const
    {
        return m_header.num_attribs;
    }

    const char * GetAttributeName(unsigned int index) const
    {
        return index < m_header.num_attribs ? m_attrib[index].name : 0;
    }

    unsigned int GetFrameCount(void) const
    {
        return m_header.num_frames;
    }

    unsigned int GetMaterialCount(void) const
    {
        return m_header.num_materials;
    }

    const char * GetMaterialName(unsigned int material_index) const
    {
        return m_material[material_index].name;
    }

    const vmath::vec3 GetMaterialAmbient(unsigned int material_index) const
    {
        return vmath::vec3(m_material[material_index].ambient.x, m_material[material_index].ambient.y, m_material[material_index].ambient.z);
    }

    const vmath::vec3 GetMaterialDiffuse(unsigned int material_index) const
    {
        return vmath::vec3(m_material[material_index].diffuse.x, m_material[material_index].diffuse.y, m_material[material_index].diffuse.z);
    }

    const char * GetMaterialDiffuseMapName(unsigned int material_index) const
    {
        return m_material[material_index].diffuse_map;
    }

    const char * GetMaterialSpecularMapName(unsigned int material_index) const
    {
        return m_material[material_index].specular_map;
    }

    const char * GetMaterialNormalMapName(unsigned int material_index) const
    {
        return m_material[material_index].normal_map;
    }

    void SetMaterialDiffuseTexture(unsigned int material_index, GLuint texname)
    {
        m_material_textures[material_index].diffuse = texname;
    }

    void SetMaterialSpecularTexture(unsigned int material_index, GLuint texname)
    {
        m_material_textures[material_index].specular = texname;
    }

    void SetMaterialNormalTexture(unsigned int material_index, GLuint texname)
    {
        m_material_textures[material_index].normal = texname;
    }

    void BindVertexArray()
    {
        glBindVertexArray(m_vao);
    }

protected:
    GLuint m_vao;
    GLuint m_attribute_buffer;
    GLuint m_index_buffer;

    VBM_HEADER m_header;
    VBM_ATTRIB_HEADER * m_attrib;
    VBM_FRAME_HEADER * m_frame;
    VBM_MATERIAL * m_material;
    VBM_RENDER_CHUNK * m_chunks;

    struct material_texture
    {
        GLuint diffuse;
        GLuint specular;
        GLuint normal;
    };

    material_texture * m_material_textures;
};
#endif /* VBM_FILE_TYPES_ONLY */

#endif /* __VBM_H__ */
