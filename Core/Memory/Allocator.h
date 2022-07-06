#pragma once

#include <Lib/Assert.h>

namespace nv
{
    using Byte = unsigned char;
    using BytePtr = Byte*;

    class IAllocator
    {
    public:
        virtual void*   Allocate(size_t size) = 0;
        virtual void    Free(void* ptr) = 0;
        virtual void    Reset() {};
    };

    class SystemAllocator : public IAllocator
    {
    public:
        void*   Allocate(size_t size) override;
        void    Free(void* ptr) override;

        static SystemAllocator* gPtr;
    };

    class ArenaAllocator : public IAllocator
    {
    public:
        static constexpr size_t kArenaDefaultSize = 1024;

        ArenaAllocator(IAllocator* allocator, size_t capacity = kArenaDefaultSize):
            mBuffer(nullptr),
            mCurrent(nullptr),
            mCapacity(capacity),
            mAllocator(allocator) 
        {
            assert(mAllocator);
            mBuffer = (Byte*)allocator->Allocate(capacity);
            mCurrent = mBuffer;
        }

        void*   Allocate(size_t size) override;
        void    Free(void* ptr) override { } // Do nothing

        ~ArenaAllocator() 
        {
            if (mBuffer)
            {
                mAllocator->Free(mBuffer);
                mBuffer = nullptr;
                mCurrent = nullptr;
                mCapacity = 0;
            }
        }

    private:
        Byte*           mBuffer;
        Byte*           mCurrent;
        size_t          mCapacity;
        IAllocator*     mAllocator;
    };

    extern SystemAllocator gSysAllocator;
}