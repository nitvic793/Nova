#pragma once

namespace nv
{
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
}