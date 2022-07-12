#pragma once

#ifndef NV_MEMORY
#define NV_MEMORY

#define NV_ENABLE_MEM_TRACKING 1

#include <cstdint>

#if NV_ENABLE_MEM_TRACKING
#include <Lib/Map.h>
#include <Lib/StringHash.h>
#endif

namespace nv
{
    class MemTracker
    {
    public:
        using PtrType = uint64_t;
        using TagType = const char*;
    public:
        MemTracker() :
            mBytesAllocated(0),
            mBytesSystemAllocated(0),
            mbEnableTracking(true)
        {}

        void TrackAlloc(void* ptr, size_t size);
        void TrackAllocTagged(void* ptr, size_t size, TagType tag);
        void TrackSysAlloc(void* ptr, size_t size);
        void TrackFree(void* ptr);
        void TrackSysFree(void* ptr);
        void SetEnableTracking(bool bEnabled) { mbEnableTracking = bEnabled; }

        constexpr uint64_t GetCurrentAllocatedBytes() { return mBytesAllocated; }
        constexpr uint64_t GetSystemAllocatedBytes() { return mBytesSystemAllocated; }

        static MemTracker* gPtr;
    private:
        uint64_t mBytesAllocated;
        uint64_t mBytesSystemAllocated;
#if NV_ENABLE_MEM_TRACKING
        bool mbEnableTracking;
        HashMap<PtrType,        size_t>     mPtrSizeMap;
        HashMap<PtrType,        size_t>     mSysPtrSizeMap;
        HashMap<PtrType,       TagType>     mTagSizeMap;
#endif
    };

    void            InitMemoryTracker();
    MemTracker*&    GetMemoryTracker();
    void            DestroyMemoryTracker(MemTracker*& memTracker = GetMemoryTracker());
    void            SetMemoryTracker(MemTracker* memTracker);
    
    size_t          GetCurrentAllocatedBytes();
    size_t          GetSystemAllocatedBytes();
}

#endif