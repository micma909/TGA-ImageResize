#include "IMemoryManager.h"
#include <map>
#include <vector>
class MemoryManager : public IMemoryManager {
	struct FreeStore {
		FreeStore *next;
	};
	void expandPoolSize(size_t size);
	void cleanUp();
	FreeStore* freeStoreHead;

	size_t chunkSize;
public:
	MemoryManager() {
		freeStoreHead = 0;
	}
	virtual ~MemoryManager() {
		cleanUp();
	}
	// client must set nr of preallocations
	int _NODECOUNT = 0;

	virtual void  setNumberOfAllocations(size_t);
	virtual void* internalAllocate(size_t);
	virtual void  free(void*);
};

// std::map fascilitating different size allocations
//
// The strategy is as follows: If a user decides to to run 
// TargaHandler over a set of images with similar resolutions
// then each image of a specific resolution WxH creates its  
// dedicated MemoryManager. If the same resolution is encountered
// then there is already memory reserved for a resize operation.

extern std::map<uint32_t, MemoryManager*> g_MemoryMap;

// simplified allocation 
template<class T>
static T* newMemory(size_t size, unsigned int nrOfAlloc) {
	if (g_MemoryMap.find(size) == std::end(g_MemoryMap)) {
		g_MemoryMap[size] = new MemoryManager();
		g_MemoryMap[size]->setNumberOfAllocations(nrOfAlloc);
	}
	return g_MemoryMap[size]->allocate<T>(size);
}

static bool freeMemory(void* ptr, uint32_t size) {
	if (g_MemoryMap.find(size) == std::end(g_MemoryMap)) {
		printf("Error, no key in map corresponding to size %i", size);
		return false;
	}
	g_MemoryMap[size]->free(ptr);
	return true;
}

/*template<class T>
static T* newMemory(uint32_t id) {
return g_MemoryMap[id]->allocate<T>(sizeof(T));
}*/