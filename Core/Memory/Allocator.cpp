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

    void* ArenaAllocator::Allocate(size_t size)
    {
        assert((mCurrent - mBuffer) + size <= mCapacity);
        auto buffer = mCurrent;
        mCurrent += size;
        return buffer;
    }

    void* Alloc(size_t size, IAllocator* alloc)
    {
        return alloc->Allocate(size);
    }

    void* AllocTagged(const char* tag, IAllocator* alloc, size_t size)
    {
        void* ptr = alloc->Allocate(size);
        if (nv::MemTracker::gPtr)
            nv::MemTracker::gPtr->TrackSysAlloc(ptr, size);
        return ptr;
    }

    void Free(void* ptr, IAllocator* alloc)
    {
        alloc->Free(ptr);
    }
}

