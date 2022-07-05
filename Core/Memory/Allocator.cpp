#include "Allocator.h"
#include "Memory/Memory.h"
#include <cstdint>
#include <cstdlib>

namespace nv
{
    SystemAllocator gSysAllocator;
    SystemAllocator* SystemAllocator::gPtr = &gSysAllocator;

    void* SystemAllocator::Allocate(size_t size)
    {
        void* ptr = malloc(size);
        if(nv::MemTracker::gPtr)
            nv::MemTracker::gPtr->TrackSysAlloc(ptr, size);
        return ptr;
    }

    void SystemAllocator::Free(void* ptr)
    {
        if (nv::MemTracker::gPtr)
            nv::MemTracker::gPtr->TrackSysFree(ptr);
        free(ptr);
    }
}

