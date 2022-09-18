#ifndef NV_VECTOR
#define NV_VECTOR

#pragma once

#include "Memory/Allocator.h"
#include "Assert.h"
#include <cstdint>
#include <cstring>
#include <utility>
#include <Lib/Array.h>

namespace nv
{
	constexpr uint32_t kMinVectorSize = 4;
	constexpr uint32_t kVectorGrowthMultiplier = 2;



	template<typename T, bool TGrowDynamic = true, uint32_t TSize = kMinVectorSize>
	class Vector
	{
	public:
		Vector(IAllocator* allocator = nullptr) :
			mBuffer(nullptr)
		{
			Reserve(TSize, allocator);
		}

		Vector(uint32_t count, IAllocator* allocator = nullptr) :
			mBuffer(nullptr)
		{
			// Override TSize if count > TSize
			if constexpr (!TGrowDynamic)
				count = count > TSize ? count : TSize;

			Reserve(count, allocator);
		}

		constexpr Vector(std::initializer_list<T> list, IAllocator* allocator = nullptr) :
			mBuffer(nullptr)
		{
			const auto count = (uint32_t)list.size();
			// Override TSize if count > TSize
			if constexpr (!TGrowDynamic)
				count = count > TSize ? count : TSize;

			Reserve(count, allocator); 
			for (const T& item : list)
				Push(item);
		}

		Vector(Vector&& v) noexcept
		{
			mBuffer = v.mBuffer;
			mCurrentIndex = v.mCurrentIndex;
			mCapacity = v.mCapacity;
			allocator = v.allocator;
			memset(&v, 0, sizeof(v)); //Reset v
		}

		Vector(Vector const&& v)
		{
			mBuffer = v.mBuffer;
			mCurrentIndex = v.mCurrentIndex;
			mCapacity = v.mCapacity;
			allocator = v.allocator;
			memset(&v, 0, sizeof(v));
		}

		Vector& operator=(Vector&& v) noexcept
		{
			if (this == &v)
			{
				return *this;
			}

			Free();

			mBuffer = v.mBuffer;
			mCurrentIndex = v.mCurrentIndex;
			mCapacity = v.mCapacity;
			allocator = v.allocator;
			memset(&v, 0, sizeof(v)); //Reset v
			return *this;
		}

		constexpr void Reserve(uint32_t count = kMinVectorSize, IAllocator* allocator = nullptr)
		{
			this->allocator = allocator;
			if (!allocator)
			{
				this->allocator = SystemAllocator::gPtr;
			}

			Grow(count);
		}

		void Grow()
		{
			Grow(mCapacity * kVectorGrowthMultiplier);
		}

		constexpr void Grow(uint64_t newCapacity, bool bUpdateSize = false)
		{
			if (newCapacity <= mCapacity) return;

			const auto oldCapacity = mCapacity;
			mCapacity = newCapacity;
			auto buffer = allocator->Allocate(sizeof(T) * mCapacity);

			if (mBuffer && oldCapacity)
			{
				memcpy(buffer, mBuffer, oldCapacity * sizeof(T));
				allocator->Free(mBuffer);
			}

			mBuffer = (T*)buffer;

			if (bUpdateSize)
				mCurrentIndex = (uint32_t)mCapacity - 1;
		}

		void SetSize(uint32_t count = kMinVectorSize)
		{
			Reserve(count, allocator);
			mCurrentIndex = count - 1;
			for (auto& val : *this)
			{
				val = T();
			}
		}

		void Push(const T& value)
		{
			GrowIfNeeded();
			assert(mCurrentIndex + 1 < mCapacity);
			mCurrentIndex++;
			new (&mBuffer[mCurrentIndex]) T();
			mBuffer[mCurrentIndex] = value;
		}

		void Push(const T&& value)
		{
			GrowIfNeeded();
			assert(mCurrentIndex + 1 < mCapacity);
			mCurrentIndex++;
			mBuffer[mCurrentIndex] = std::move(value);
		}

		T& Emplace()
		{
			GrowIfNeeded();
			assert(mCurrentIndex + 1 < mCapacity);
			mCurrentIndex++;
			return mBuffer[mCurrentIndex];
		}

		template<typename ...Args>
		T& Emplace(Args&&... args)
		{
			GrowIfNeeded();
			assert(mCurrentIndex + 1 < mCapacity);
			mCurrentIndex++;
			T* buffer = &mBuffer[mCurrentIndex];
			new(buffer) T(args);
			return mBuffer[mCurrentIndex];
		}

		const T& Pop()
		{
			assert(mCurrentIndex >= 0);
			auto& val = mBuffer[mCurrentIndex];
			mCurrentIndex--;
			return val;
		}

		void Pop(T& val)
		{
			assert(mCurrentIndex >= 0);
			auto& val = mBuffer[mCurrentIndex];
			mCurrentIndex--;
		}

		constexpr bool Exists(T& inItem) const
		{
			const auto span = Span();
			for (const auto& item : span)
			{
				if (inItem == item) return true;
			}

			return false;
		}

		constexpr T& operator[](size_t index) const noexcept
		{
			return mBuffer[index];
		}

		constexpr bool IsEmpty() const { return mCurrentIndex == -1; }

		constexpr size_t Size() const
		{
			return mCurrentIndex + 1;
		}

		constexpr size_t size() const
		{
			return Size();
		}

		constexpr size_t Capacity() const
		{
			return mCapacity;
		}

		constexpr T* Data() const
		{
			return mBuffer;
		}

		constexpr Span<T> Slice(size_t start, size_t end) const
		{
			assert(start >= 0 && start < Size());
			assert(end < Size() && end >= start);
			return { mBuffer + start, end - start + 1 };
		}

		constexpr Span<T> Span() const
		{
			return { mBuffer, Size() };
		}

		void CopyFrom(const T* data, const uint32_t count)
		{
			memcpy(mBuffer + Size(), data, sizeof(T) * count);
			mCurrentIndex = (int32_t)(Size() + count - 1);
		}

		void Sort()
		{
			QuickSort(mBuffer, 0, (int)mCurrentIndex);
		}

		~Vector()
		{
			Free();
		}

		constexpr T* begin() const
		{
			return mBuffer;
		}

		constexpr T* end() const
		{
			return mBuffer + (mCurrentIndex + 1);
		}

		constexpr void Clear()
		{
			for (uint32_t i = 0; i < Size(); ++i)
				mBuffer[i].~T();

			mCurrentIndex = -1;
		}

		void Reset()
		{
			mCurrentIndex = -1;
			Free();
			Grow(kMinVectorSize);
		}

	private:
		constexpr void GrowIfNeeded()
		{
			if constexpr (TGrowDynamic)
			{
				if (mCurrentIndex + 1 >= mCapacity)
				{
					Grow();
				}
			}
		}

		int Partition(T* arr, int l, int h)
		{
			T x = arr[h];
			int i = (l - 1);

			for (int j = l; j <= h - 1; j++) {
				if (arr[j] <= x) {
					i++;
					Swap(arr[i], arr[j]);
				}
			}
			Swap(arr[i + 1], arr[h]);
			return (i + 1);
		}

		void QuickSort(T* A, int l, int h)
		{
			if (l < h)
			{
				/* Partitioning index */
				int p = Partition(A, l, h);
				QuickSort(A, l, p - 1);
				QuickSort(A, p + 1, h);
			}
		}

		void Swap(T& lhs, T& rhs)
		{
			T temp = lhs;
			lhs = rhs;
			rhs = temp;
		}

		void Free()
		{
			if (!mBuffer) return;
			for (auto& val : *this)
			{
				val.~T();
			}

			if (allocator && mBuffer)
			{
				allocator->Free(mBuffer);
			}
			else if (mBuffer)
			{
				SystemAllocator::gPtr->Free(mBuffer);
			}

			mBuffer = nullptr;
		}

		T*			mBuffer = nullptr;
		int32_t		mCurrentIndex = -1;
		uint64_t	mCapacity = 0;
		IAllocator* allocator = nullptr;
	};

	template<typename T, uint32_t TSize>
	using FixedVector = Vector<T, false, TSize>;
}

#endif // !NV_VECTOR
