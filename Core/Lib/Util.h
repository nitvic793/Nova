#ifndef NV_UTIL
#define NV_UTIL

#pragma once

#include <type_traits>
#include <string>

namespace nv
{
    template <class T>
    struct RemoveReference { typedef T Type; };

    template <class T>
    struct RemoveReference<T&> { typedef T Type; };

    template <class T>
    struct RemoveReference<T&&> { typedef T Type; };

    template <class T> 
    T&& Forward(typename RemoveReference<T>::Type& t) noexcept
    {
        return static_cast<T&&>(t);
    }

    template <class T> 
    T&& Forward(typename RemoveReference<T>::Type&& t) noexcept
    {
        return static_cast<T&&>(t);
    }

    template<class T, class U>
    concept Derived = std::is_base_of<U, T>::value;

    std::wstring    ToWString(const std::string& str);
    std::string     ToString(const std::wstring& str);
}

#endif // !NV_UTIL
