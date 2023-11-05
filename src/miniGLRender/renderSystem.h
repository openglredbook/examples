#ifndef __RENDER_SYSTEM_H__
#define __RENDER_SYSTEM_H__

#include "tr_common.h"

// qqRenderSystem
class qqRenderSystem
{
public:

	virtual ~qqRenderSystem() {}
	virtual void Init() = 0;
	virtual void Shutdown() = 0;

	virtual const emptyCommand_t* SwapCommandBuffers(uint64* frontEndMicroSec, uint64* backEndMicroSec, uint64* shadowMicroSec, uint64* gpuMicroSec) = 0;

};
#endif // __RENDER_SYSTEM_H__
