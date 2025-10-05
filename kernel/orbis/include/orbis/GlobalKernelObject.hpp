#pragma once
#include <kernel/GlobalKernelObject.hpp>

namespace orbis {
struct OrbisNamespace;

template <rx::Serializable T>
using GlobalKernelObject = kernel::GlobalKernelObject<T, OrbisNamespace>;

template <rx::Serializable T> GlobalKernelObject<T> createGlobalObject() {
  return {};
}

inline void constructAllGlobals() {
  kernel::GlobalKernelObjectStorage<OrbisNamespace>::ConstructAll();
}
inline void destructAllGlobals() {
  kernel::GlobalKernelObjectStorage<OrbisNamespace>::DestructAll();
}

template <typename T> T &getGlobalObject() {
  assert(detail::GlobalKernelObjectInstance<GlobalKernelObject<T>>::instance);
  return kernel::detail::GlobalKernelObjectInstance<
             OrbisNamespace, GlobalKernelObject<T>>::instance->get();
}
} // namespace orbis
