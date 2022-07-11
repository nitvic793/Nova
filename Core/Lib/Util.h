#ifndef NV_UTIL
#define NV_UTIL

#pragma once

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
}

#endif // !NV_UTIL
