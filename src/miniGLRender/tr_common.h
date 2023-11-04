#ifndef __TR_COMMON_H__
#define __TR_COMMON_H__

#include "common.h"
#include "math_common.h"

typedef unsigned short triIndex_t;

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
