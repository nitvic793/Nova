#pragma once

namespace nv
{
	template<typename T, int size>
	constexpr int ArrayCountOf(T(&)[size]) { return size; }

	template<typename T>
	struct Span
	{
	public:
		T*			mData;
		size_t		mSize;

	public:
		constexpr Span() = default;
		constexpr Span(T* data, size_t size) :
			mData(data),
			mSize(size)
		{}

		constexpr Span(std::initializer_list<T> items) :
			mData(const_cast<T*>(items.begin())),
			mSize(items.size())
		{}

		constexpr Span(T items[]) :
			mData(items),
			mSize(ArrayCountOf(items))
		{}

		static constexpr Span<T> Empty() { return { nullptr, 0 }; }

		static constexpr Span<T> Create(std::initializer_list<T> items)
		{
			return { items.begin(), items.size() };
		}

		constexpr Span<T> Slice(size_t start, size_t end) const
		{
			assert(start >= 0 && start < Size());
			assert(end <= Size() && end >= start);
			return { mData + start, end - start };
		}

		constexpr T* begin() const
		{
			return mData;
		}

		constexpr T* end() const
		{
			return mData + mSize;
		}

		constexpr T& operator[](size_t index) const noexcept
		{
			return mData[index];
		}

		constexpr size_t Size() const { return mSize; }
	};

    template<typename TData>
    struct Array
    {
        size_t mSize;
        TData* mData;

        constexpr TData* begin()  const { return mData; }
        constexpr TData* end()    const { return mData + mSize; }

        constexpr size_t Size()         const { return mSize; }
        constexpr size_t SizeInBytes()  const { return mSize * sizeof(TData); }
        constexpr TData& operator[](size_t index) { return mData[index]; }

		constexpr Span<TData> Slice(size_t start, size_t end) const
		{
			return { mData + start, end - start + 1 };
		}
    };
}