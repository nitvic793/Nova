#pragma once

#ifndef NV_MEMORY
#define NV_MEMORY

#define NV_ENABLE_MEM_TRACKING 1

#include <cstdint>
#include <unordered_map>

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
            mBytesSystemAllocated(0)
        {}

        void TrackAlloc(void* ptr, size_t size);
        void TrackSysAlloc(void* ptr, size_t size);
        void TrackFree(void* ptr);
        void TrackSysFree(void* ptr);

        constexpr uint64_t GetCurrentAllocatedBytes() { return mBytesAllocated; }
        constexpr uint64_t GetSystemAllocatedBytes() { return mBytesSystemAllocated; }

        static MemTracker* gPtr;
    private:
        uint64_t mBytesAllocated;
        uint64_t mBytesSystemAllocated;
    };

    void            InitMemoryTracker();
    MemTracker*&    GetMemoryTracker();
    void            DestroyMemoryTracker(MemTracker*& memTracker = GetMemoryTracker());
    void            SetMemoryTracker(MemTracker* memTracker);
    
    size_t          GetCurrentAllocatedBytes();
    size_t          GetSystemAllocatedBytes();
}

#endif