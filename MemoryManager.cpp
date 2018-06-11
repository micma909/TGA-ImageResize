#include "MemoryManager.h"
#include <vector>

// Memory manager based on IBM blog post :
// https://www.ibm.com/developerworks/aix/tutorials/au-memorymanager/index.html
// 
// The assumption has been that TargaHandler could be used for multiple files
// where the best case scenario is encountering a set having the exact same resolution, 
// as then Targhandler deploys only one MemoryManager. 

#define MIN_POOLSIZE 1
#include <type_traits>

template< typename T >
bool is_aligned(T* p) {
	return !(reinterpret_cast<uintptr_t>(p) % std::alignment_of<T>::value);
}

std::map<uint32_t, MemoryManager*> g_MemoryMap;

// setPoolSize
//
// Classes using MemoryManager should set how many pre-allocations
// are to be made per each call to MemoryManager::internalAllocate.
//
// @param size - sets number of nodes / pre-alloc to be made
void MemoryManager::setNumberOfAllocations(size_t size) {
	_NODECOUNT = size;
}

// allocate
//
// Allocates memory for _NODECOUNT number of memory chunks
// 
// @param size - size of object requesting data
void* MemoryManager::internalAllocate(size_t size) {
	if (0 == freeStoreHead)      // if at end of memory pool
		expandPoolSize(size);    // claim new chunk of data acc. to POOLSIZE

	// go to available chunk in pool ...
	FreeStore* head = freeStoreHead;
	freeStoreHead = head->next; // set pointer to next available memory-slot
	return head; // return first available memory-slot 
}


// Expand Poolsize
//
// If MemoryManager has run out of data, this function then allocates
// new nodes corresponding to a specific MemoryManagers size allocation.
// (see newMemory & freeMemory in MemoryManager.h)
void MemoryManager::expandPoolSize(size_t size) {
	// if no _NODECOUNT set, default to 1:1 allocation
	// i.e same cost as using 'new'
	if (_NODECOUNT == 0) _NODECOUNT = MIN_POOLSIZE;

	size_t ss = (size > sizeof(FreeStore*)) ? size : sizeof(FreeStore*);

	// Set total pool to be later carved into pieces
	size_t totalSize = (ss*_NODECOUNT + _NODECOUNT * 2);

	// exaggerated over-alloc
	// totalSize = totalSize * 4.0f;

	char* allocSize = (char*)_aligned_offset_malloc(totalSize, 16, 1);
	memset(allocSize, 0, totalSize);
	FreeStore* head = reinterpret_cast<FreeStore*> (&allocSize[1]);
	freeStoreHead = head;

	// Mark this node's padding as true, meaning: beginning of pool 
	allocSize[0] = true;

	int offset = 0;
	bool aligned = false;
	for (int i = 1; i < _NODECOUNT; i++) {
		head->next = reinterpret_cast<FreeStore*>(&(allocSize)[(ss + 1)*i + offset]);
		offset = 0;
		aligned = false;

		char* tmp = (char*)head->next;
		while (!aligned) {
			aligned = is_aligned((__m128*)(tmp));
			if (aligned) {
				// Mark node as consecutive node (i.e NOT beginning of pool)
				*(tmp - 1) = false;
				head->next = reinterpret_cast<FreeStore*>(tmp);
			}
			else {
				tmp = tmp + 1;
				offset++;
			}
		}
		head = head->next;
	}
	head->next = 0;
}

// Free
//
// Returns memory back to memory manager by placing 
// free'd memory into FreeStore node that is re-attached
// at the beginnign of the linked list. 
void MemoryManager::free(void* deleted) {
	FreeStore* head = static_cast<FreeStore*> (deleted);
	head->next = freeStoreHead;
	freeStoreHead = head;
}

// Cleanup
//
// Destroys all memory claimed by the memory manager
void MemoryManager::cleanUp() {
	FreeStore* nextPtr = freeStoreHead;
	std::vector<char*> ptrs;
	for (; nextPtr; nextPtr = freeStoreHead) {
		char* memChunk = reinterpret_cast<char*> (nextPtr);
		// check if memChunk points to beginning of memory pool
		// otherwise if points to just a consecutive node, skip.
		if (memChunk[-1] == '\x1') {
			memChunk--;
			ptrs.push_back(memChunk);
		}
		freeStoreHead = freeStoreHead->next;
	}
	// delete / free
	for (auto i : ptrs) {
		_aligned_free(i);
	}
}
