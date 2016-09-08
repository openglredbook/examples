//////////////////////////////////////////////////////////////////////////////
//
//  Cube.h - data for unit cube centered at the origin composed of
//             2 individual triangles per cube face
//
//////////////////////////////////////////////////////////////////////////////

#include <GL/gl.h>

const int NumCubeVertices = 8;
const int NumCubeFaces = 6;
const int NumCubeTriangles = 12;
const int NumCubeIndices = 3 * NumCubeTriangles;

//
//  CubeVertices - vertices for a unit cube centered at the origin
//

GLfloat CubeVertices[NumCubeVertices][3] = {
    { -0.5, -0.5, -0.5 }  // Vertex 0
    {  0.5, -0.5, -0.5 }  // Vertex 1
    {  0.5,  0.5, -0.5 }  // Vertex 2
    { -0.5,  0.5, -0.5 }  // Vertex 3
    { -0.5, -0.5,  0.5 }  // Vertex 4
    {  0.5, -0.5,  0.5 }  // Vertex 5
    {  0.5,  0.5,  0.5 }  // Vertex 6
    { -0.5,  0.5,  0.5 }  // Vertex 7
};

//
//  CubeIndices - indices for 12 individual triangles that make up a cube
//

GLint CubeIndices[NumCubeTriangles][3] = 
{ }
};
