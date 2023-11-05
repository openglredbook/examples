#ifndef __TR_COMMON_H__
#define __TR_COMMON_H__

#include "common.h"
#include "math_common.h"

#include <windows.h>
#include <gl3/gl3.h>
#include <gl3/gl3w.h>

typedef unsigned short triIndex_t;

// renderCommand_t
enum renderCommand_t
{
	RC_NOP,
	RC_DRAW_VIEW_3D,	// may be at a reduced resolution, will be upsampled before 2D GUIs
	RC_DRAW_VIEW_GUI,	// not resolution scaled
	RC_SET_BUFFER,
	RC_COPY_RENDER,
	RC_POST_PROCESS,
};

struct emptyCommand_t {
	renderCommand_t		commandId;
	renderCommand_t* next;
};


struct setBufferCommand_t
{
	renderCommand_t commandId;
	renderCommand_t* next;
	GLenum	buffer;
};

struct drawSurfsCommand_t {
	renderCommand_t commandId;
	renderCommand_t* next;
	//viewDef_t* viewDef;
};

// qqDrawVert
class qqDrawVert
{
public:
	qqVec3				xyz;
	halfFloat_t			st[2];
	unsigned char		normal[4];
	unsigned char		tangent[4];
	unsigned char		color[4];
	unsigned char		color2[4];
};


#endif // __TR_COMMON_H__
