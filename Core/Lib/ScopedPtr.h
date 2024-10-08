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

		ScopedPtr(ScopedPtr&& p) noexcept :
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

		constexpr T& operator * () { return *ptr; }
		constexpr T* operator -> () { return ptr; }
		constexpr T* operator -> () const { return ptr; }
		constexpr T* Get() { return ptr; }
		constexpr T* Get() const { return ptr; }

		template<typename TT>
		constexpr TT* As() { return static_cast<TT*>(ptr); }

		template<typename TT>
		constexpr TT* As() const { return static_cast<TT*>(ptr); }

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

	template<typename T, bool TCallSysFree = false>
	static ScopedPtr<T, TCallSysFree> MakeScoped()
	{
		auto allocator = SystemAllocator::gPtr;
		return MakeScoped<T, TCallSysFree>(allocator);
	}

	template<typename T, bool TCallSysFree = false>
	static ScopedPtr<T, TCallSysFree> MakeScoped(IAllocator* allocator)
	{
		auto alloc = allocator->Allocate(sizeof(T));
		T* p = new(alloc) T();
		ScopedPtr<T, TCallSysFree> ptr(p);
		return ptr;
	}

	template<typename T, bool TCallSysFree = false, typename ...Args>
	static ScopedPtr<T, TCallSysFree> MakeScopedArgs(Args&& ... args)
	{
		auto allocator = SystemAllocator::gPtr;
		auto alloc = allocator->Allocate(sizeof(T));
		ScopedPtr<T> ptr(new(alloc) T(std::forward<Args>(args)...));
		return ptr;
	}

	template<typename T, bool TCallSysFree = false, typename ...Args>
	static ScopedPtr<T, TCallSysFree> MakeScopedArgsAlloc(IAllocator* allocator, Args&&... args)
	{
		auto alloc = allocator->Allocate(sizeof(T));
		ScopedPtr<T> ptr(new(alloc) T(std::forward<Args>(args)...));
		return ptr;
	}
}