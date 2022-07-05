#pragma once

#include <Memory/Memory.h>

namespace nv
{
	template<typename T, bool TCallSystemFree = false>
	class ScopedPtr
	{
	public:
		explicit ScopedPtr(T* p)
		{
			ptr = p;
		}

		ScopedPtr(ScopedPtr&& p) :
			ptr(p.ptr)
		{
			p.ptr = nullptr;
		}

		ScopedPtr& operator=(const ScopedPtr& rhs) = delete;

		ScopedPtr& operator=(ScopedPtr&& rhs)
		{
			ptr = rhs.ptr;
			rhs.ptr = nullptr;
			return *this;
		}

		ScopedPtr() {}

		T& operator * () { return *ptr; }
		T* operator -> () { return ptr; }
		T* operator -> () const { return ptr; }
		T* Get() { return ptr; }
		T* Get() const { return ptr; }

		//For compatibility
		T* get() { return ptr; }
		T* get() const { return ptr; }

		~ScopedPtr()
		{
			if (ptr)
			{
				ptr->~T();
				if constexpr (TCallSystemFree)
					SystemAllocator::gPtr->Free(ptr);
				ptr = nullptr;
			}
		}
	private:
		T* ptr = nullptr;
	};

	template<typename T>
	static ScopedPtr<T> MakeScoped()
	{
		auto allocator = SystemAllocator::gPtr;
		return MakeScoped<T>(allocator);
	}

	template<typename T>
	static ScopedPtr<T> MakeScoped(IAllocator* allocator)
	{
		auto alloc = allocator->Allocate(sizeof(T));
		T* p = new(alloc) T();
		ScopedPtr<T> ptr(p);
		return ptr;
	}

	template<typename T, typename ...Args>
	static ScopedPtr<T> MakeScopedArgs(Args&& ... args)
	{
		auto allocator = SystemAllocator::gPtr;
		auto alloc = allocator->Allocate(sizeof(T));
		ScopedPtr<T> ptr(new(alloc) T(std::forward<Args>(args)...));
		return ptr;
	}

	template<typename T, typename ...Args>
	static ScopedPtr<T> MakeScopedArgsAlloc(IAllocator* allocator, Args&&... args)
	{
		auto alloc = allocator->Allocate(sizeof(T));
		ScopedPtr<T> ptr(new(alloc) T(std::forward<Args>(args)...));
		return ptr;
	}
}