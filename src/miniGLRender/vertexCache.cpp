#include "vertexCache.h"

/*
==============
ClearGeoBufferSet
==============
*/
static void ClearGeoBufferSet(geoBufferSet_t& gbs)
{
	gbs.indexMemUsed.SetValue(0);
	gbs.vertexMemUsed.SetValue(0);
	gbs.jointMemUsed.SetValue(0);
	gbs.allocations = 0;
}

/*
==============
MapGeoBufferSet
==============
*/
static void MapGeoBufferSet(geoBufferSet_t& gbs) {
	if (gbs.mappedVertexBase == NULL) {
		gbs.mappedVertexBase = (byte*)gbs.vertexBuffer.MapBuffer(BM_WRITE);
	}
	if (gbs.mappedIndexBase == NULL) {
		gbs.mappedIndexBase = (byte*)gbs.indexBuffer.MapBuffer(BM_WRITE);
	}
	//if (gbs.mappedJointBase == NULL && gbs.jointBuffer.GetAllocedSize() != 0) {
	//	gbs.mappedJointBase = (byte*)gbs.jointBuffer.MapBuffer(BM_WRITE);
	//}
}

/*
==============
UnmapGeoBufferSet
==============
*/
static void UnmapGeoBufferSet(geoBufferSet_t& gbs) {
	if (gbs.mappedVertexBase != NULL) {
		gbs.vertexBuffer.UnmapBuffer();
		gbs.mappedVertexBase = NULL;
	}
	if (gbs.mappedIndexBase != NULL) {
		gbs.indexBuffer.UnmapBuffer();
		gbs.mappedIndexBase = NULL;
	}
	//if (gbs.mappedJointBase != NULL) {
	//	gbs.jointBuffer.UnmapBuffer();
	//	gbs.mappedJointBase = NULL;
	//}
}

/*
==============
AllocGeoBufferSet
==============
*/
static void AllocGeoBufferSet(geoBufferSet_t& gbs, const int vertexBytes, const int indexBytes, const int jointBytes) {
	gbs.vertexBuffer.AllocBufferObject(NULL, vertexBytes);
	gbs.indexBuffer.AllocBufferObject(NULL, indexBytes);
	if (jointBytes != 0) {
		// todo
		//gbs.jointBuffer.AllocBufferObject(NULL, jointBytes / sizeof(idJointMat));
	}
	ClearGeoBufferSet(gbs);
}

//// qqVertexCache
//void qqVertexCache::Init(bool restart)
//{
//	currentFrame = 0;
//	listNum = 0;
//
//	mostUsedVertex = 0;
//	mostUsedIndex = 0;
//	mostUsedJoint = 0;
//
//	for (int i = 0; i < VERTCACHE_NUM_FRAMES; i++) {
//		AllocGeoBufferSet(frameData[i], VERTCACHE_VERTEX_MEMORY_PER_FRAME, VERTCACHE_INDEX_MEMORY_PER_FRAME, VERTCACHE_JOINT_MEMORY_PER_FRAME);
//	}
//	AllocGeoBufferSet(staticData, STATIC_VERTEX_MEMORY, STATIC_INDEX_MEMORY, 0);
//
//	MapGeoBufferSet(frameData[listNum]);
//}
//
//void qqVertexCache::Shutdown()
//{
//	for (int i = 0; i < VERTCACHE_NUM_FRAMES; i++) {
//		frameData[i].vertexBuffer.FreeBufferObject();
//		frameData[i].indexBuffer.FreeBufferObject();
//		frameData[i].jointBuffer.FreeBufferObject();
//	}
//}
