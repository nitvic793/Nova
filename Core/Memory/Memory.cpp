
#include <cstdlib>
#include <cstdint>
#include "Memory.h"
#include <cstdio>
#include "Allocator.h"

///
/// Operator new and delete overrides to enable memory allocation tracking.
/////////////////////////////////////////////////////////////////

void* operator new(size_t size, bool track = false)
{
    void* ptr = nv::SystemAllocator::gPtr->Allocate(size);
    return ptr;
}

void* operator new[](size_t size, bool track = false)
{
    void* ptr = nv::SystemAllocator::gPtr->Allocate(size);
    return ptr;
}

void operator delete(void* ptr) 
{
    nv::SystemAllocator::gPtr->Free(ptr);
}

void operator delete[](void* ptr) 
{
    nv::SystemAllocator::gPtr->Free(ptr);
}

namespace nv
{
    MemTracker* MemTracker::gPtr = nullptr;

    void InitMemoryTracker()
    {
        auto buffer = malloc(sizeof(MemTracker));
        MemTracker::gPtr = new (buffer) MemTracker();
        // Track its own mem alloc
        MemTracker::gPtr->TrackSysAlloc(MemTracker::gPtr, sizeof(MemTracker));
    }

    size_t GetCurrentAllocatedBytes()
    {
        return MemTracker::gPtr->GetCurrentAllocatedBytes();
    }

    size_t GetSystemAllocatedBytes()
    {
        return MemTracker::gPtr->GetSystemAllocatedBytes();
    }
    
    void DestroyMemoryTracker(MemTracker* &memTracker)
    {
        memTracker->~MemTracker();
        free(memTracker);
        memTracker = nullptr;
    }

    void SetMemoryTracker(MemTracker* memTracker)
    {
        MemTracker::gPtr = memTracker;
    }

    MemTracker* &GetMemoryTracker()
    {
        return MemTracker::gPtr;
    }

    void MemTracker::TrackAlloc(void* ptr, size_t size)
    {
#if NV_ENABLE_MEM_TRACKING
        mBytesAllocated += size;
        mPtrSizeMap[(PtrType)ptr] = size;
#endif
    }

    void MemTracker::TrackAllocTagged(void* ptr, size_t size, TagType tag)
    {
#if NV_ENABLE_MEM_TRACKING
        mTagSizeMap[(PtrType)ptr] = tag;
        TrackAlloc(ptr, size);
#endif
    }

    void MemTracker::TrackSysAlloc(void* ptr, size_t size)
    {
#if NV_ENABLE_MEM_TRACKING
        if (mbEnableTracking)
        {
            SetEnableTracking(false);
            mBytesSystemAllocated += size;
            mSysPtrSizeMap[(PtrType)ptr] = size;
            SetEnableTracking(true);
        }
#endif
    }

    void MemTracker::TrackFree(void* ptr)
    {
#if NV_ENABLE_MEM_TRACKING
        const size_t size = mPtrSizeMap.at((PtrType)ptr);
        mPtrSizeMap.erase((PtrType)ptr);
        mBytesAllocated -= size;
#endif
    }

    void MemTracker::TrackSysFree(void* ptr)
    {
#if NV_ENABLE_MEM_TRACKING
        if (mbEnableTracking)
        {
            SetEnableTracking(false);
            auto it = mSysPtrSizeMap.find((PtrType)ptr);
            if (it != mSysPtrSizeMap.end())
            {
                const size_t size = it->second;
                mSysPtrSizeMap.erase((PtrType)ptr);
                mBytesSystemAllocated -= size;
            }

            SetEnableTracking(true);
        }
#endif
    }
}

