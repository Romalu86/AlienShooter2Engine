#pragma once

#include <type_traits>

// Storage only: the owning subsystem starts and ends T's lifetime explicitly.
// This preserves native startup and first-use ordering without a C++ global
// constructor or an automatically registered destructor.
template<class T>
struct StaticObjectStorage
{
    typename std::aligned_storage<sizeof(T), alignof(T)>::type memory;

    T* Address()
    {
        return reinterpret_cast<T*>(&memory);
    }

    T& Get()
    {
        return *Address();
    }
};
