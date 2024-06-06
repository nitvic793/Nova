#include "pch.h"

#include "Allocator.h"
#include "Memory/Memory.h"
#include <cstdint>
#include <cstdlib>


#include <Debug/Profiler.h>

#if NV_TEST_BED
#define NV_MEM_ALLOC(ptr, size) ((void)0)
#define NV_MEM_FREE(ptr)        ((void)0)
#endif

namespace nv
{
    SystemAllocator gSysAllocator;
    SystemAllocator* SystemAllocator::gPtr = &gSysAllocator;

    void* SystemAllocator::Allocate(size_t size)
    {
        void* ptr = malloc(size);
        NV_MEM_ALLOC(ptr, size);
        if(nv::MemTracker::gPtr)
            nv::MemTracker::gPtr->TrackSysAlloc(ptr, size);
        return ptr;
    }

    void SystemAllocator::Free(void* ptr)
    {
        NV_MEM_FREE(ptr);
        if (nv::MemTracker::gPtr)
            nv::MemTracker::gPtr->TrackSysFree(ptr);
        free(ptr);
    }

    void* SystemAllocator::Realloc(size_t size, void* ptr)
    {
        auto buffer = realloc(ptr, size);
        if (nv::MemTracker::gPtr)
        {
            nv::MemTracker::gPtr->TrackSysFree(ptr);
            nv::MemTracker::gPtr->TrackSysAlloc(buffer, size);
        }
        return buffer;
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

