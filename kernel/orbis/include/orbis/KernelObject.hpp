#pragma once
#include <kernel/KernelObject.hpp>

namespace orbis {
struct OrbisNamespace;

template <rx::Serializable StateT>
using GlobalObjectRef =
    kernel::StaticObjectRef<OrbisNamespace, kernel::detail::GlobalScope,
                            StateT>;

template <rx::Serializable StateT>
using ProcessLocalObjectRef =
    kernel::StaticObjectRef<OrbisNamespace, kernel::detail::ProcessScope,
                            StateT>;

template <rx::Serializable StateT>
using ThreadLocalObjectRef =
    kernel::StaticObjectRef<OrbisNamespace, kernel::detail::ThreadScope,
                            StateT>;

template <rx::Serializable StateT>
GlobalObjectRef<StateT> createGlobalObject() {
  auto layoutOffset = kernel::StaticKernelObjectStorage<
      OrbisNamespace, kernel::detail::GlobalScope>::template Allocate<StateT>();

  return GlobalObjectRef<StateT>(layoutOffset);
}

template <rx::Serializable StateT>
kernel::StaticObjectRef<OrbisNamespace, kernel::detail::ProcessScope, StateT>
createProcessLocalObject() {
  auto layoutOffset = kernel::StaticKernelObjectStorage<
      OrbisNamespace,
      kernel::detail::ProcessScope>::template Allocate<StateT>();
  return kernel::StaticObjectRef<OrbisNamespace, kernel::detail::ProcessScope,
                                 StateT>(layoutOffset);
}

template <rx::Serializable StateT>
kernel::StaticObjectRef<OrbisNamespace, kernel::detail::ThreadScope, StateT>
createThreadLocalObject() {
  auto layoutOffset = kernel::StaticKernelObjectStorage<
      OrbisNamespace, kernel::detail::ThreadScope>::template Allocate<StateT>();
  return kernel::StaticObjectRef<OrbisNamespace, kernel::detail::ThreadScope,
                                 StateT>(layoutOffset);
}

inline void constructAllGlobals() {
  kernel::StaticKernelObjectStorage<
      OrbisNamespace, kernel::detail::GlobalScope>::ConstructAll();
}

inline void constructAllProcessLocals() {
  kernel::StaticKernelObjectStorage<
      OrbisNamespace, kernel::detail::ProcessScope>::ConstructAll();
}

inline void constructAllThreadLocals() {
  kernel::StaticKernelObjectStorage<
      OrbisNamespace, kernel::detail::ThreadScope>::ConstructAll();
}

inline void destructAllGlobals() {
  kernel::StaticKernelObjectStorage<OrbisNamespace,
                                    kernel::detail::GlobalScope>::DestructAll();
}

inline void destructAllProcessLocals() {
  kernel::StaticKernelObjectStorage<
      OrbisNamespace, kernel::detail::ProcessScope>::DestructAll();
}

inline void destructAllThreadLocals() {
  kernel::StaticKernelObjectStorage<OrbisNamespace,
                                    kernel::detail::ThreadScope>::DestructAll();
}

inline void serializeAllGlobals(rx::Serializer &s) {
  kernel::StaticKernelObjectStorage<
      OrbisNamespace, kernel::detail::GlobalScope>::SerializeAll(s);
}

inline void serializeAllProcessLocals(rx::Serializer &s) {
  kernel::StaticKernelObjectStorage<
      OrbisNamespace, kernel::detail::ProcessScope>::SerializeAll(s);
}

inline void serializeAllThreadLocals(rx::Serializer &s) {
  kernel::StaticKernelObjectStorage<
      OrbisNamespace, kernel::detail::ThreadScope>::SerializeAll(s);
}

inline void deserializeAllGlobals(rx::Deserializer &s) {
  kernel::StaticKernelObjectStorage<
      OrbisNamespace, kernel::detail::GlobalScope>::DeserializeAll(s);
}

inline void deserializeAllProcessLocals(rx::Deserializer &s) {
  kernel::StaticKernelObjectStorage<
      OrbisNamespace, kernel::detail::ProcessScope>::DeserializeAll(s);
}

inline void deserializeAllThreadLocals(rx::Deserializer &s) {
  kernel::StaticKernelObjectStorage<
      OrbisNamespace, kernel::detail::ThreadScope>::DeserializeAll(s);
}
} // namespace orbis
