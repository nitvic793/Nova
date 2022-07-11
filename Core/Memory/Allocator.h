#pragma once

#include <Lib/Assert.h>
#include <Lib/Util.h>

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

    extern SystemAllocator gSysAllocator;

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
            assert(mBuffer);
            mCurrent = mBuffer;
        }

        void*   Allocate(size_t size) override;
        virtual void    Free(void* ptr) override { } // Do nothing

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

    protected:
        Byte*           mBuffer;
        Byte*           mCurrent;
        size_t          mCapacity;
        IAllocator*     mAllocator;
    };

    class LinearAllocator : public ArenaAllocator {};

    class StackAllocator : public ArenaAllocator
    {
    public:
        virtual void Free(void* ptr) override
        {
            mCurrent = (Byte*)ptr;
        }
    };

    template<typename T, typename ...Args>
    constexpr T* Alloc(IAllocator* alloc = SystemAllocator::gPtr, Args&&... args) 
    { 
        auto buffer = alloc->Allocate(sizeof(T)); 
        new (buffer) T(Forward<Args>(args)...);
        return (T*)buffer;
    }

    template<typename T>
    constexpr void Free(T* ptr, IAllocator* alloc = SystemAllocator::gPtr)
    {
        if (ptr)
        {
            ptr->~T();
            alloc->Free(ptr);
        }
    }
}