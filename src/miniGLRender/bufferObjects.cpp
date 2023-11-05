#include "bufferObjects.h"


//static const GLenum bufferUsage = GL_STATIC_DRAW_ARB;
static const GLenum bufferUsage = GL_DYNAMIC_DRAW;

bool IsWriteCombined(void* base)
{
	MEMORY_BASIC_INFORMATION info;
	SIZE_T size = VirtualQueryEx(GetCurrentProcess(), base, &info, sizeof(info));
	if (size == 0) {
		DWORD error = GetLastError();
		error = error;
		return false;
	}
	bool isWriteCombined = ((info.AllocationProtect & PAGE_WRITECOMBINE) != 0);
	return isWriteCombined;
}

// qqVertexBuffer
qqVertexBuffer::qqVertexBuffer()
{
	size = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = 0;
	SetUnmapped();
}

qqVertexBuffer::~qqVertexBuffer()
{
	FreeBufferObject();
}

bool qqVertexBuffer::AllocBufferObject(const void* data, int allocSize)
{
	assert(apiObject == NULL);
	assert_16_byte_aligned(data);

	if (allocSize <= 0) {
		return false;
	}

	size = allocSize;

	bool allocationFailed = false;

	int numBytes = GetAllocedSize();


	// clear out any previous error
	glGetError();

	GLuint bufferObject = 0xFFFF;
	glGenBuffers(1, &bufferObject);
	if (bufferObject == 0xFFFF) {
		//idLib::FatalError("idVertexBuffer::AllocBufferObject: failed");
		return false;
	}
	glBindBuffer(GL_ARRAY_BUFFER, bufferObject);

	// these are rewritten every frame
	glBufferData(GL_ARRAY_BUFFER, numBytes, NULL, bufferUsage);
	apiObject = (bufferObject);

	GLenum err = glGetError();
	if (err == GL_OUT_OF_MEMORY) {
		//idLib::Warning("idVertexBuffer::AllocBufferObject: allocation failed");
		allocationFailed = true;
	}

	// copy the data
	if (data != NULL) {
		Update(data, allocSize);
	}

	return !allocationFailed;
}

void qqVertexBuffer::FreeBufferObject()
{
	if (IsMapped()) {
		UnmapBuffer();
	}

	// if this is a sub-allocation inside a larger buffer, don't actually free anything.
	if (OwnsBuffer() == false) {
		ClearWithoutFreeing();
		return;
	}

	if (apiObject == NULL) {
		return;
	}

	//if (r_showBuffers.GetBool()) {
	//	idLib::Printf("vertex buffer free %p, api %p (%i bytes)\n", this, GetAPIObject(), GetSize());
	//}

	GLuint bufferObject = (apiObject);
	glDeleteBuffers(1, &bufferObject);

	ClearWithoutFreeing();
}

void qqVertexBuffer::Reference(const qqVertexBuffer& other)
{
	assert(IsMapped() == false);
	//assert( other.IsMapped() == false );	// this happens when building idTriangles while at the same time setting up idDrawVerts
	assert(other.GetAPIObject() != NULL);
	assert(other.GetSize() > 0);

	FreeBufferObject();
	size = other.GetSize();						// this strips the MAPPED_FLAG
	offsetInOtherBuffer = other.GetOffset();	// this strips the OWNS_BUFFER_FLAG
	apiObject = other.apiObject;
	assert(OwnsBuffer() == false);
}

void qqVertexBuffer::Reference(const qqVertexBuffer& other, int refOffset, int refSize)
{
	assert(IsMapped() == false);
	//assert( other.IsMapped() == false );	// this happens when building idTriangles while at the same time setting up idDrawVerts
	assert(other.GetAPIObject() != NULL);
	assert(refOffset >= 0);
	assert(refSize >= 0);
	assert(refOffset + refSize <= other.GetSize());

	FreeBufferObject();
	size = refSize;
	offsetInOtherBuffer = other.GetOffset() + refOffset;
	apiObject = other.apiObject;
	assert(OwnsBuffer() == false);
}

void qqVertexBuffer::Update(const void* data, int updateSize) const
{
	assert(apiObject != NULL);
	assert(IsMapped() == false);
	assert_16_byte_aligned(data);
	assert((GetOffset() & 15) == 0);

	if (updateSize > size) {
		//idLib::FatalError("idVertexBuffer::Update: size overrun, %i > %i\n", updateSize, GetSize());
		return;
	}

	int numBytes = (updateSize + 15) & ~15;

	GLuint bufferObject = apiObject;

	glBindBuffer(GL_ARRAY_BUFFER, bufferObject);
	glBufferSubData(GL_ARRAY_BUFFER, GetOffset(), (GLsizeiptr)numBytes, data);
	/*
		void * buffer = MapBuffer( BM_WRITE );
		CopyBuffer( (byte *)buffer + GetOffset(), (byte *)data, numBytes );
		UnmapBuffer();
	*/
}

void* qqVertexBuffer::MapBuffer(bufferMapType_t mapType) const
{
	assert(apiObject != NULL);
	assert(IsMapped() == false);

	void* buffer = NULL;

	GLuint bufferObject = (apiObject);

	glBindBuffer(GL_ARRAY_BUFFER, bufferObject);
	if (mapType == BM_READ) {
		//buffer = qglMapBufferARB( GL_ARRAY_BUFFER_ARB, GL_READ_ONLY_ARB );
		buffer = glMapBufferRange(GL_ARRAY_BUFFER, 0, GetAllocedSize(), GL_MAP_READ_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
		if (buffer != NULL) {
			buffer = (byte*)buffer + GetOffset();
		}
	}
	else if (mapType == BM_WRITE) {
		//buffer = qglMapBufferARB( GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB );
		buffer = glMapBufferRange(GL_ARRAY_BUFFER, 0, GetAllocedSize(), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
		if (buffer != NULL) {
			buffer = (byte*)buffer + GetOffset();
		}
		assert(IsWriteCombined(buffer));
	}
	else {
		assert(false);
		return nullptr;
	}

	SetMapped();

	if (buffer == NULL) {
		//idLib::FatalError("idVertexBuffer::MapBuffer: failed");
		return nullptr;
	}
	return buffer;
}

void qqVertexBuffer::UnmapBuffer() const
{
	assert(apiObject != NULL);
	assert(IsMapped());

	GLuint bufferObject = (apiObject);
	glBindBuffer(GL_ARRAY_BUFFER, bufferObject);
	if (!glUnmapBuffer(GL_ARRAY_BUFFER)) {
		//idLib::Printf("idVertexBuffer::UnmapBuffer failed\n");
	}

	SetUnmapped();
}

void qqVertexBuffer::ClearWithoutFreeing() {
	size = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = 0;
}


// qqIndexBuffer
qqIndexBuffer::qqIndexBuffer()
{
	size = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = NULL;
	SetUnmapped();
}

qqIndexBuffer::~qqIndexBuffer()
{
	FreeBufferObject();
}

bool qqIndexBuffer::AllocBufferObject(const void* data, int allocSize)
{
	assert(apiObject == NULL);
	assert_16_byte_aligned(data);

	if (allocSize <= 0) {
		//idLib::Error("qqIndexBuffer::AllocBufferObject: allocSize = %i", allocSize);
		return false;
	}

	size = allocSize;

	bool allocationFailed = false;

	int numBytes = GetAllocedSize();


	// clear out any previous error
	glGetError();

	GLuint bufferObject = 0xFFFF;
	glGenBuffers(1, &bufferObject);
	if (bufferObject == 0xFFFF) {
		GLenum error = glGetError();
		//idLib::FatalError("qqIndexBuffer::AllocBufferObject: failed - GL_Error %d", error);
		return false;
	}
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferObject);

	// these are rewritten every frame
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, numBytes, NULL, bufferUsage);
	apiObject = (bufferObject);

	GLenum err = glGetError();
	if (err == GL_OUT_OF_MEMORY)
	{
		//idLib::Warning("qqIndexBuffer:AllocBufferObject: allocation failed");
		allocationFailed = true;
	}

	//if (r_showBuffers.GetBool()) {
	//	idLib::Printf("index buffer alloc %p, api %p (%i bytes)\n", this, GetAPIObject(), GetSize());
	//}

	// copy the data
	if (data != NULL) {
		Update(data, allocSize);
	}

	return !allocationFailed;
}

void qqIndexBuffer::FreeBufferObject()
{
	if (IsMapped()) {
		UnmapBuffer();
	}

	// if this is a sub-allocation inside a larger buffer, don't actually free anything.
	if (OwnsBuffer() == false) {
		ClearWithoutFreeing();
		return;
	}

	if (apiObject == NULL) {
		return;
	}

	//if (r_showBuffers.GetBool()) {
	//	idLib::Printf("index buffer free %p, api %p (%i bytes)\n", this, GetAPIObject(), GetSize());
	//}

	GLuint bufferObject = (apiObject);
	glDeleteBuffers(1, &bufferObject);

	ClearWithoutFreeing();
}

void qqIndexBuffer::Reference(const qqIndexBuffer& other)
{
	assert(IsMapped() == false);
	//assert( other.IsMapped() == false );	// this happens when building idTriangles while at the same time setting up triIndex_t
	assert(other.GetAPIObject() != NULL);
	assert(other.GetSize() > 0);

	FreeBufferObject();
	size = other.GetSize();						// this strips the MAPPED_FLAG
	offsetInOtherBuffer = other.GetOffset();	// this strips the OWNS_BUFFER_FLAG
	apiObject = other.apiObject;
	assert(OwnsBuffer() == false);
}

void qqIndexBuffer::Reference(const qqIndexBuffer& other, int refOffset, int refSize)
{
	assert(IsMapped() == false);
	//assert( other.IsMapped() == false );	// this happens when building idTriangles while at the same time setting up triIndex_t
	assert(other.GetAPIObject() != NULL);
	assert(refOffset >= 0);
	assert(refSize >= 0);
	assert(refOffset + refSize <= other.GetSize());

	FreeBufferObject();
	size = refSize;
	offsetInOtherBuffer = other.GetOffset() + refOffset;
	apiObject = other.apiObject;
	assert(OwnsBuffer() == false);
}

void qqIndexBuffer::Update(const void* data, int updateSize) const
{

	assert(apiObject != NULL);
	assert(IsMapped() == false);
	assert_16_byte_aligned(data);
	assert((GetOffset() & 15) == 0);

	if (updateSize > size) {
		//idLib::FatalError("qqIndexBuffer::Update: size overrun, %i > %i\n", updateSize, GetSize());
		return;
	}

	int numBytes = (updateSize + 15) & ~15;

	GLuint bufferObject = (apiObject);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferObject);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, GetOffset(), (GLsizeiptr)numBytes, data);
	/*
		void * buffer = MapBuffer( BM_WRITE );
		CopyBuffer( (byte *)buffer + GetOffset(), (byte *)data, numBytes );
		UnmapBuffer();
	*/
}

void* qqIndexBuffer::MapBuffer(bufferMapType_t mapType) const
{

	assert(apiObject != NULL);
	assert(IsMapped() == false);

	void* buffer = NULL;

	GLuint bufferObject = (apiObject);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferObject);
	if (mapType == BM_READ)	{
		//buffer = qglMapBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, GL_READ_ONLY_ARB );
		buffer = glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, GetAllocedSize(), GL_MAP_READ_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
		if (buffer != NULL) {
			buffer = (byte*)buffer + GetOffset();
		}
	}
	else if (mapType == BM_WRITE) {
		//buffer = qglMapBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB );
		buffer = glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, GetAllocedSize(), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
		if (buffer != NULL) {
			buffer = (byte*)buffer + GetOffset();
		}
		assert(IsWriteCombined(buffer));
	}
	else {
		assert(false);
	}

	SetMapped();

	if (buffer == NULL) {
		//idLib::FatalError("qqIndexBuffer::MapBuffer: failed");
	}
	return buffer;
}

void qqIndexBuffer::UnmapBuffer() const
{
	assert(apiObject != NULL);
	assert(IsMapped());

	GLuint bufferObject = (apiObject);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferObject);
	if (!glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER)) {
		//idLib::Printf("qqIndexBuffer::UnmapBuffer failed\n");
	}

	SetUnmapped();
}

void qqIndexBuffer::ClearWithoutFreeing()
{
	size = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = NULL;
}
