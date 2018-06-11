#pragma once
// Generic memory manager 
class IMemoryManager {
public:
	template<class T>
	inline T* allocate(size_t s) {
		return reinterpret_cast<T*>(internalAllocate(s * sizeof(T)));
	};
	virtual void  free(void*) = 0;

protected:
	virtual void* internalAllocate(size_t) = 0;
};
