#pragma once
#include "rx/Serializer.hpp"
#include <cstddef>
#include "kernel/KernelObject.hpp"

namespace cellos {

struct CellOsNamespace;

extern std::byte* g_globalStorage;

template <rx::Serializable T>
class GlobalObjectRef {
    std::uint32_t mOffset;

public:
    explicit GlobalObjectRef(std::uint32_t offset) : mOffset(offset) {}
    T* get() { return reinterpret_cast<T*>(g_globalStorage + mOffset); }
    T* operator->() { return get(); }
    T& operator*() { return *get(); }
};

template <rx::Serializable StateT>
GlobalObjectRef<StateT> createGlobalObject() {
    auto layoutOffset =
        kernel::StaticKernelObjectStorage<
            CellOsNamespace,
            kernel::detail::GlobalScope
        >::template Allocate<StateT>();

    return GlobalObjectRef<StateT>(layoutOffset);
}

template <rx::Serializable StateT>
kernel::StaticObjectRef<CellOsNamespace, kernel::detail::ProcessScope, StateT>
createProcessLocalObject() {
    auto layoutOffset =
        kernel::StaticKernelObjectStorage<
            CellOsNamespace,
            kernel::detail::ProcessScope
        >::template Allocate<StateT>();

    return { layoutOffset };
}

template <rx::Serializable StateT>
kernel::StaticObjectRef<CellOsNamespace, kernel::detail::ThreadScope, StateT>
createThreadLocalObject() {
    auto layoutOffset =
        kernel::StaticKernelObjectStorage<
            CellOsNamespace,
            kernel::detail::ThreadScope
        >::template Allocate<StateT>();

    return { layoutOffset };
}

inline void constructAllGlobals() {
    kernel::StaticKernelObjectStorage<
        CellOsNamespace,
        kernel::detail::GlobalScope
    >::ConstructAll(g_globalStorage);
}

inline void destructAllGlobals() {
    kernel::StaticKernelObjectStorage<
        CellOsNamespace,
        kernel::detail::GlobalScope
    >::DestructAll(g_globalStorage);
}

inline void serializeAllGlobals(rx::Serializer& s) {
    kernel::StaticKernelObjectStorage<
        CellOsNamespace,
        kernel::detail::GlobalScope
    >::SerializeAll(g_globalStorage, s);
}

inline void deserializeAllGlobals(rx::Deserializer& s) {
    kernel::StaticKernelObjectStorage<
        CellOsNamespace,
        kernel::detail::GlobalScope
    >::DeserializeAll(g_globalStorage, s);
}

} // namespace cellos
