/*

    VBM object exporter for Maya

    (C) 2011 Graham Sellers

    Simple exporter for Maya to produce VBM objects for use in
    Vermilion Book examples. Do whatever you want with this code!

*/

#define _CRT_SECURE_NO_WARNINGS
#include <fstream>
#include <iostream>
#include <vector>

#define _BOOL
#include <maya/MObject.h>
#include <maya/MFnPlugin.h>
#include <maya/MItDag.h>
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnMesh.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MPointArray.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MPxFileTranslator.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>

#define VBM_FILE_TYPES_ONLY
#include "vbm.h"

#ifdef _DEBUG
#define DEBUG_MSG(a) do { std::cout << a; } while (0)
#define DEBUG_VERSION " debug"
#else
#define DEBUG_MSG(a) do { } while (0)
#define DEBUG_VERSION
#endif /* _DEBUG */

const char *const vbmDefaultOptions =
    "materials=1;"
    "normals=1;"
    "boundingbox=1;"
    ;

class MayaSBMExporter : public MPxFileTranslator
{
public:
    MayaSBMExporter() {}
    ~MayaSBMExporter() {}

    MStatus writer(const MFileObject &file, const MString &optionsString, MPxFileTranslator::FileAccessMode mode);

    bool haveWriteMethod() const { return true; }

    MString defaultExtension() const { return "vbm"; }

    static void *creator() { return new MayaSBMExporter; }
};

// ********************************************************************
// MItMeshPolygon::getTriangle() returns object-relative vertex
// indices; BUT MItMeshPolygon::normalIndex() and ::getNormal() need
// face-relative vertex indices! This converts vertex indices from
// object-relative to face-relative.
//
// param  getVertices: Array of object-relative vertex indices for
//                     entire face.
// param  getTriangle: Array of object-relative vertex indices for
//                     local triangle in face.
//
// return Array of face-relative indicies for the specified vertices.
//        Number of elements in returned array == number in getTriangle
//        (should be 3).
//
// note   If getTriangle array does not include a corresponding vertex
//        in getVertices array then a value of (-1) will be inserted
//        in that position within the returned array.
// ********************************************************************
MIntArray GetLocalIndex( MIntArray & getVertices, MIntArray & getTriangle )
{
  MIntArray   localIndex;
  unsigned    gv, gt;

//  assert ( getTriangle.length() == 3 );    // Should always deal with a triangle

  for ( gt = 0; gt < getTriangle.length(); gt++ )
  {
    for ( gv = 0; gv < getVertices.length(); gv++ )
    {
      if ( getTriangle[gt] == getVertices[gv] )
      {
        localIndex.append( gv );
        break;
      }
    }

    // if nothing was added, add default "no match"
    if ( localIndex.length() == gt )
      localIndex.append( -1 );
  }

  return localIndex;
}

inline VBM_VEC3F float_vector_to_SBM(const MFloatVector& v)
{
    VBM_VEC3F vec;

    vec.x = v.x;
    vec.y = v.y;
    vec.z = v.z;

    return vec;
}

inline VBM_VEC4F color_to_SBM(const MColor& c)
{
    VBM_VEC4F col;

    col.x = c.r;
    col.y = c.g;
    col.z = c.b;
    col.w = c.a;

    return col;
}

inline unsigned short float_bits_to_half_bits(const float f)
{
    union
    {
        struct
        {
            unsigned short m : 10;
            unsigned short e : 5;
            unsigned short s : 1;
        };
        unsigned short all_bits;
    } f16;

    union
    {
        struct
        {
            unsigned int m : 23;
            unsigned int e : 8;
            unsigned int s : 1;
        };
        unsigned int all_bits;
       float float_bits;
    } f32;

    f32.float_bits = f;

    f16.all_bits = 0;
    f16.s = f32.s;
    f16.e = f32.e - 112;
    f16.m = f32.m >> 13;

    return f16.all_bits;
}

MStatus MayaSBMExporter::writer(const MFileObject &file, const MString &optionsString, MPxFileTranslator::FileAccessMode mode)
{
    MStatus status;
    unsigned int i, j;
    bool bSelected = (mode == kExportActiveAccessMode);

    FILE * f = fopen(file.expandedFullName().asChar(), "wb");

    if (f == NULL)
    {
        return MS::kFailure;
    }

    std::vector<VBM_VEC4F> vertices;            // Positions
    std::vector<unsigned int> elements;         // Indices
    std::vector<VBM_VEC3F> normals;             // Normals
    std::vector<VBM_VEC3F> tangents;            // Tangents
    std::vector<VBM_VEC3F> binormals;           // Binormals
    std::vector<std::string> uv_names;          // Names of UVs
    std::vector<VBM_VEC2F> uv[16];              // Up to 16 UVs supported
    std::vector<VBM_VEC4F> colors;              // Vertex colors

    VBM_HEADER header;

    memset(&header, 0, sizeof(header));
    header.magic = VBM_MAGIC_CURRENT;
    sprintf(header.name, "model");
    header.num_attribs = 1;
    header.num_frames = 1;
    header.num_vertices = 1;
    header.num_indices = 1;

    VBM_ATTRIB_HEADER attrib_header;

    memset(&attrib_header, 0, sizeof(attrib_header));
    sprintf(attrib_header.name, "position");
    attrib_header.type = 0x1406;
    attrib_header.components = 4;
    attrib_header.flags = 0;

    VBM_FRAME_HEADER frame_header;

    memset(&frame_header, 0, sizeof(frame_header));
    frame_header.first = 0;
    frame_header.count = 0;

    MSelectionList sel;
    MGlobal::getActiveSelectionList(sel);

    MItDag dagIter(MItDag::kDepthFirst, MFn::kInvalid, &status);
    MItSelectionList selIter(sel, MFn::kGeometric);

    for ( ; bSelected ? !selIter.isDone() : !dagIter.isDone(); bSelected ? selIter.next() : dagIter.next() ) {

        MDagPath dagPath;
        status = bSelected ? selIter.getDagPath( dagPath ) : dagIter.getPath( dagPath );

        if (status) {
            MFnDagNode dagNode(dagPath, &status);

            if ( dagNode.isIntermediateObject()) continue;
            if ( !dagPath.hasFn( MFn::kMesh )) continue;
            if ( dagPath.hasFn( MFn::kTransform )) continue;

            MFnMesh mesh(dagPath);

            MPointArray vertexList;
            MFloatVectorArray normalList;
            MFloatVectorArray tangentList;
            MFloatVectorArray binormalList;
            MColorArray colorList;
            MStringArray  UVSets;
            MFloatArray u[16], v[16];

            mesh.getPoints(vertexList);
            mesh.getNormals(normalList);
            mesh.getTangents(tangentList);
            mesh.getBinormals(binormalList);
            mesh.getColors(colorList);
            mesh.getUVSetNames(UVSets);

            for (i = 0; i < UVSets.length(); i++) {
                uv_names.push_back(UVSets[i].asChar());
                mesh.getUVs(u[i], v[i], &UVSets[i]);
            }

            MItMeshPolygon itPolygon(dagPath, MObject::kNullObj);

            for (; !itPolygon.isDone(); itPolygon.next()) {

                int numTriangles = 0;
                MIntArray polygonVertices;

                itPolygon.getVertices(polygonVertices);
                itPolygon.numTriangles(numTriangles);

                while (numTriangles--) {

                    MPointArray     nonTweaked;
                    MIntArray       triangleVertices;
                    MIntArray       localIndex;

                    status = itPolygon.getTriangle(numTriangles, nonTweaked, triangleVertices, MSpace::kObject);

                    localIndex = GetLocalIndex(polygonVertices, triangleVertices);

                    VBM_VEC4F vec4;
                    VBM_VEC2F vec2;

                    for (i = 0; i < 3; i++) {
                        vec4.x = (float)vertexList[ triangleVertices[i] ].x;
                        vec4.y = (float)vertexList[ triangleVertices[i] ].y;
                        vec4.z = (float)vertexList[ triangleVertices[i] ].z;
                        vec4.w = (float)vertexList[ triangleVertices[i] ].w;

                        elements.push_back(static_cast<unsigned int>(vertices.size()));
                        vertices.push_back(vec4);
                    }

                    if (normalList.length() != 0) {
                        for (i = 0; i < 3; i++) {
                            normals.push_back(float_vector_to_SBM(normalList[itPolygon.normalIndex(localIndex[i])]));
                        }
                    }

                    if (tangentList.length() != 0) {
                        for (i = 0; i < 3; i++) {
                            tangents.push_back(float_vector_to_SBM(tangentList[itPolygon.tangentIndex(localIndex[i])]));
                        }
                    }

                    if (binormalList.length() != 0) {
                        for (i = 0; i < 3; i++) {
                            binormals.push_back(float_vector_to_SBM(tangentList[itPolygon.tangentIndex(localIndex[i])]));
                        }
                    }

                    if (colorList.length() != 0) {
                        for (i = 0; i < 3; i++) {
                            colors.push_back(color_to_SBM(colorList[ triangleVertices[i] ]));
                        }
                    }

                    for (i = 0; i < 3; i++) {
                        for (j = 0; j < uv_names.size(); j++) {
                            int uvID;
                            itPolygon.getUVIndex(localIndex[i], uvID, &UVSets[j]);
                            vec2.x = u[j][uvID];
                            vec2.y = v[j][uvID];
                            uv[j].push_back(vec2);
                        }
                    }

                }

            }

            break;
        }

    }

    header.num_vertices = static_cast<unsigned int>(vertices.size());
    header.num_indices = 0; // elements.size();
    header.index_type = 0; // GL_NONE
    frame_header.count =  static_cast<unsigned int>(elements.size());

    float min_x = 0.0f;
    float max_x = 0.0f;
    float min_y = 0.0f;
    float max_y = 0.0f;
    float min_z = 0.0f;
    float max_z = 0.0f;

    // Calculate bounding box
    if (header.num_vertices != 0)
    {
        min_x = vertices[0].x;
        max_x = min_x;
        min_y = vertices[0].y;
        max_y = min_y;
        min_z = vertices[0].z;
        max_z = min_z;

        for (i = 1; i < header.num_vertices; i++)
        {
            min_x = std::min(min_x, vertices[i].x);
            min_y = std::min(min_y, vertices[i].y);
            min_z = std::min(min_z, vertices[i].z);
            max_x = std::max(max_x, vertices[i].x);
            max_y = std::max(max_y, vertices[i].y);
            max_z = std::max(max_z, vertices[i].z);
        }
    }

    // Centralize...

    float center_x = (max_x + min_x) * 0.5f;
    float center_y = (max_y + min_y) * 0.5f;
    float center_z = (max_z + min_z) * 0.5f;

    for (i = 0; i < header.num_vertices; i++)
    {
        vertices[i].x -= center_x;
        vertices[i].y -= center_y;
        vertices[i].z -= center_z;
    }

    if (normals.size() != 0)
        header.num_attribs++;
    if (tangents.size() != 0)
        header.num_attribs++;

    header.num_attribs +=  static_cast<unsigned int>(uv_names.size());

    header.size = sizeof(header);
    fwrite(&header, sizeof(header), 1, f);
    fwrite(&attrib_header, sizeof(attrib_header), 1, f);
    if (colors.size() != 0) {
        sprintf(attrib_header.name, "color");
        attrib_header.components = 4;
        fwrite(&attrib_header, sizeof(attrib_header), 1, f);
    }
    if (normals.size() != 0) {
        sprintf(attrib_header.name, "normal");
        attrib_header.components = 3;
        fwrite(&attrib_header, sizeof(attrib_header), 1, f);
    }
    if (tangents.size() != 0) {
        sprintf(attrib_header.name, "tangent");
        attrib_header.components = 3;
        fwrite(&attrib_header, sizeof(attrib_header), 1, f);
    }
    if (tangents.size() != 0) {
        sprintf(attrib_header.name, "binormal");
        attrib_header.components = 3;
        fwrite(&attrib_header, sizeof(attrib_header), 1, f);
    }
    for (i = 0; i < uv_names.size(); i++) {
        sprintf(attrib_header.name, uv_names[i].c_str());
        attrib_header.components = 2;
        fwrite(&attrib_header, sizeof(attrib_header), 1, f);
    }
    fwrite(&frame_header, sizeof(frame_header), 1, f);

    unsigned short s = float_bits_to_half_bits(vertices[0].x);

    if (header.num_vertices != 0)
    {
        fwrite((const VBM_VEC4F *)&vertices[0], sizeof(VBM_VEC4F), header.num_vertices, f);
        if (colors.size() != 0)
            fwrite((const float *)&colors[0], sizeof(VBM_VEC4F), header.num_vertices, f);
        if (normals.size() != 0)
            fwrite((const float *)&normals[0], sizeof(VBM_VEC3F), header.num_vertices, f);
        if (tangents.size() != 0)
            fwrite((const float *)&tangents[0], sizeof(VBM_VEC3F), header.num_vertices, f);
        if (binormals.size() != 0)
            fwrite((const float *)&binormals[0], sizeof(binormals[0]), header.num_vertices, f);
        for (i = 0; i < uv_names.size(); i++) {
            fwrite((const float *)&uv[i][0], sizeof(VBM_VEC2F), header.num_vertices, f);
        }
    }
    // fwrite((const unsigned int *)&elements[0], sizeof(unsigned int), elements.size(), f);

    fclose(f);

    return MS::kSuccess;
}

#if defined _MSC_VER
#define MLL_EXPORT extern __declspec(dllexport)
#pragma comment (lib, "Foundation.lib")
#pragma comment(lib,"OpenMaya.lib")
#else
#define MLL_EXPORT
#endif

#define STRINGIZE_(a) #a
#define STRINGIZE(a) STRINGIZE_(a)

MLL_EXPORT MStatus initializePlugin(MObject object)
{
    MStatus status;

    MFnPlugin plugin(object, "Graham Sellers", "3.0", "Any"); // "1." STRINGIZE(VBM_VERSION) " (" __DATE__ "."__TIME__ DEBUG_VERSION ")", "Any");

    status = plugin.registerFileTranslator("VBM Exporter", "none",
                                           MayaSBMExporter::creator,
                                           "vbmExportOptions"); // , const_cast<char *>(vbmDefaultOptions));

    if (status != MS::kSuccess) {
        status.perror("Error - initializePlugin");
    }

    return status;
}

MLL_EXPORT MStatus uninitializePlugin(MObject obj)
{
    MFnPlugin plugin( obj );
    return plugin.deregisterFileTranslator( "VBM Exporter" );
}
