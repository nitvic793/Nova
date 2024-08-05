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
        virtual void*   Realloc(size_t size, void* ptr) { return nullptr; }
        virtual void    Free(void* ptr) = 0;
        virtual void    Reset() {};
    };

    class SystemAllocator : public IAllocator
    {
    public:
        void*   Allocate(size_t size) override;
        void    Free(void* ptr) override;
        void*   Realloc(size_t size, void* ptr) override;

        static SystemAllocator* gPtr;
    };

    extern SystemAllocator gSysAllocator;

    class ArenaAllocator : public IAllocator
    {
    public:
        static constexpr size_t kArenaDefaultSize = 1024;

        ArenaAllocator(size_t capacity = kArenaDefaultSize, IAllocator* allocator = SystemAllocator::gPtr):
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

        // Provide buffer directly, decoupling buffer management
        // from this allocator.
        ArenaAllocator(void* pBuffer, size_t capacity):
            mBuffer((Byte*)pBuffer),
            mCurrent((Byte*)pBuffer),
            mCapacity(capacity),
            mAllocator(nullptr) 
        {}

        void*   Allocate(size_t size) override;
        void    Free(void* ptr) override { } // Do nothing
        void    Reset() override;

        constexpr size_t  GetAllocatedSize() const { return mCurrent - mBuffer; }

        ~ArenaAllocator() 
        {
            if (mBuffer)
            {
                if(mAllocator)
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

    class LocalAllocator : public IAllocator
    {
    public:
        LocalAllocator(Byte* pBuffer, size_t capacity) :
            mBuffer(pBuffer),
            mCurrent(nullptr),
            mCapacity(capacity)
        {
            assert(mBuffer);
            mCurrent = mBuffer;
        }

        constexpr virtual void* Allocate(size_t size) override
        {
            assert((mCurrent - mBuffer) + size <= mCapacity);
            auto buffer = mCurrent;
            mCurrent += size;
            return buffer;
        }

        virtual void Free(void* ptr) override {}

    protected:
        Byte* mBuffer;
        Byte* mCurrent;
        size_t mCapacity;
    };

    using LinearAllocator = ArenaAllocator;

    class StackAllocator : public ArenaAllocator
    {
    public:
        virtual void Free(void* ptr) override
        {
            mCurrent = (Byte*)ptr;
        }
    };

    void*   Alloc(size_t size, IAllocator* alloc = SystemAllocator::gPtr);
    void*   AllocTagged(const char* tag, IAllocator* alloc, size_t size);
    void    Free(void* ptr, IAllocator* alloc = SystemAllocator::gPtr);

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