#ifndef __BUFFEROBJECT_H__
#define __BUFFEROBJECT_H__

#include "tr_common.h"


enum bufferMapType_t
{
	BM_READ,			// map for reading
	BM_WRITE			// map for writing
};

/*
================================================
qqVertexBuffer
================================================
*/
class qqVertexBuffer
{
public:
	qqVertexBuffer();
	~qqVertexBuffer();

	// Allocate or free the buffer.
	bool				AllocBufferObject(const void* data, int allocSize);
	void				FreeBufferObject();

	// Make this buffer a reference to another buffer.
	void				Reference(const qqVertexBuffer& other);
	void				Reference(const qqVertexBuffer& other, int refOffset, int refSize);

	// Copies data to the buffer. 'size' may be less than the originally allocated size.
	void				Update(const void* data, int updateSize) const;

	void* MapBuffer(bufferMapType_t mapType) const;
	qqDrawVert* MapVertexBuffer(bufferMapType_t mapType) const { return static_cast<qqDrawVert*>(MapBuffer(mapType)); }
	void				UnmapBuffer() const;
	bool				IsMapped() const { return (size & MAPPED_FLAG) != 0; }

	int					GetSize() const { return (size & ~MAPPED_FLAG); }
	int					GetAllocedSize() const { return ((size & ~MAPPED_FLAG) + 15) & ~15; }
	unsigned int		GetAPIObject() const { return apiObject; }
	int					GetOffset() const { return (offsetInOtherBuffer & ~OWNS_BUFFER_FLAG); }

private:
	int					size;					// size in bytes
	int					offsetInOtherBuffer;	// offset in bytes
	unsigned int		apiObject;

	// sizeof() confuses typeinfo...
	static const int	MAPPED_FLAG = 1 << (4 /* sizeof( int ) */ * 8 - 1);
	static const int	OWNS_BUFFER_FLAG = 1 << (4 /* sizeof( int ) */ * 8 - 1);

private:
	void				ClearWithoutFreeing();
	void				SetMapped() const { const_cast<int&>(size) |= MAPPED_FLAG; }
	void				SetUnmapped() const { const_cast<int&>(size) &= ~MAPPED_FLAG; }
	bool				OwnsBuffer() const { return ((offsetInOtherBuffer & OWNS_BUFFER_FLAG) != 0); }

	DISALLOW_COPY_AND_ASSIGN(qqVertexBuffer);
};

class qqIndexBuffer
{
public:
	qqIndexBuffer();
	~qqIndexBuffer();

	// Allocate or free the buffer.
	bool				AllocBufferObject(const void* data, int allocSize);
	void				FreeBufferObject();

	// Make this buffer a reference to another buffer.
	void				Reference(const qqIndexBuffer& other);
	void				Reference(const qqIndexBuffer& other, int refOffset, int refSize);

	// Copies data to the buffer. 'size' may be less than the originally allocated size.
	void				Update(const void* data, int updateSize) const;

	void* MapBuffer(bufferMapType_t mapType) const;
	triIndex_t* MapIndexBuffer(bufferMapType_t mapType) const { return static_cast<triIndex_t*>(MapBuffer(mapType)); }
	void				UnmapBuffer() const;
	bool				IsMapped() const { return (size & MAPPED_FLAG) != 0; }

	int					GetSize() const { return (size & ~MAPPED_FLAG); }
	int					GetAllocedSize() const { return ((size & ~MAPPED_FLAG) + 15) & ~15; }
	unsigned int		GetAPIObject() const { return apiObject; }
	int					GetOffset() const { return (offsetInOtherBuffer & ~OWNS_BUFFER_FLAG); }

private:
	int					size;					// size in bytes
	int					offsetInOtherBuffer;	// offset in bytes
	unsigned int		apiObject;

	// sizeof() confuses typeinfo...
	static const int	MAPPED_FLAG = 1 << (4 /* sizeof( int ) */ * 8 - 1);
	static const int	OWNS_BUFFER_FLAG = 1 << (4 /* sizeof( int ) */ * 8 - 1);

private:
	void				ClearWithoutFreeing();
	void				SetMapped() const { const_cast<int&>(size) |= MAPPED_FLAG; }
	void				SetUnmapped() const { const_cast<int&>(size) &= ~MAPPED_FLAG; }
	bool				OwnsBuffer() const { return ((offsetInOtherBuffer & OWNS_BUFFER_FLAG) != 0); }

	DISALLOW_COPY_AND_ASSIGN(qqIndexBuffer);
};

#endif
