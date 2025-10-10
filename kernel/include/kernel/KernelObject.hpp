#pragma once

#include "rx/Rc.hpp"
#include "rx/Serializer.hpp"
#include "rx/TypeId.hpp"
#include <memory>
#include <mutex>

namespace kernel {
class KernelObjectBase : public rx::RcBase {
  rx::TypeId mType;

public:
  KernelObjectBase(rx::TypeId type) : mType(type) {}
  [[nodiscard]] rx::TypeId getType() const { return mType; }

  template <typename T> [[nodiscard]] bool isa() const {
    return mType == rx::TypeId::get<T>();
  }
};

template <rx::Serializable StateT>
struct KernelObject : KernelObjectBase, StateT {
  template <typename... Args>
  KernelObject(Args &&...args)
      : KernelObjectBase(rx::TypeId::get<StateT>()),
        StateT(std::forward<Args>(args)...) {}

  virtual void serialize(rx::Serializer &s) const {
    if constexpr (requires(const KernelObject &instance) {
                    instance.lock();
                    instance.unlock();
                  }) {
      std::lock_guard lock(*this);
      s.serialize(static_cast<const StateT &>(*this));
    } else {
      s.serialize(static_cast<const StateT &>(*this));
    }
  }

  virtual void deserialize(rx::Deserializer &s) {
    s.deserialize(static_cast<StateT &>(*this));
  }
};

namespace detail {
struct StaticObjectCtl {
  std::size_t offset = -1ull;
  void (*construct)(void *object);
  void (*destruct)(void *object);
  void (*serialize)(void *object, rx::Serializer &);
  void (*deserialize)(void *object, rx::Deserializer &);

  template <typename T> constexpr static StaticObjectCtl Create() {
    return {
        .construct =
            +[](void *object) {
              std::construct_at(reinterpret_cast<T *>(object));
            },
        .destruct = +[](void *object) { reinterpret_cast<T *>(object)->~T(); },
        .serialize =
            +[](void *object, rx::Serializer &serializer) {
              serializer.serialize(*reinterpret_cast<T *>(object));
            },
        .deserialize =
            +[](void *object, rx::Deserializer &deserializer) {
              deserializer.deserialize(*reinterpret_cast<T *>(object));
            },
    };
  }
};

struct GlobalScope;
struct ProcessScope;
struct ThreadScope;
} // namespace detail

template <typename NamespaceT, typename ScopeT> std::byte *getScopeStorage();

template <typename NamespaceT, typename ScopeT>
struct StaticKernelObjectStorage {
  template <typename T> static std::uint32_t Allocate() {
    auto &instance = GetInstance();

    auto object = detail::StaticObjectCtl::Create<T>();
    instance.m_registry.push_back(object);

    auto offset = instance.m_size;
    offset = rx::alignUp(offset, alignof(T));
    instance.m_registry.back().offset = offset;
    instance.m_size = offset + sizeof(T);
    instance.m_alignment =
        std::max<std::size_t>(alignof(T), instance.m_alignment);
    // std::printf(
    //     "%s::Allocate(%s, %zu, %zu) -> %zu\n",
    //     rx::TypeId::get<StaticKernelObjectStorage<NamespaceT,
    //     ScopeT>>().getName().data(), rx::TypeId::get<T>().getName().data(),
    //     sizeof(T), alignof(T), offset);
    return offset;
  }

  static std::size_t GetSize() { return GetInstance().m_size; }
  static std::size_t GetAlignment() { return GetInstance().m_alignment; }
  static std::byte *getScopeStorage() {
    return kernel::getScopeStorage<NamespaceT, ScopeT>();
  }

  static void ConstructAll() {
    auto &instance = GetInstance();
    auto storage = getScopeStorage();

    for (auto objectCtl : instance.m_registry) {
      objectCtl.construct(storage + objectCtl.offset);
    }
  }

  static void DestructAll() {
    auto &instance = GetInstance();
    auto storage = getScopeStorage();

    for (auto objectCtl : instance.m_registry) {
      objectCtl.destruct(storage + objectCtl.offset);
    }
  }

  static void SerializeAll(rx::Serializer &s) {
    auto &instance = GetInstance();
    auto storage = getScopeStorage();

    s.serialize(instance.m_size);
    s.serialize(instance.m_registry.size());

    for (auto objectCtl : instance.m_registry) {
      objectCtl.serialize(storage + objectCtl.offset, s);
    }
  }

  static void DeserializeAll(rx::Deserializer &s) {
    auto &instance = GetInstance();
    auto storage = getScopeStorage();

    auto size = s.deserialize<std::size_t>();
    auto registrySize = s.deserialize<std::size_t>();

    if (size != instance.m_size || registrySize != instance.m_registry.size()) {
      s.setFailure();
      return;
    }

    for (auto objectCtl : instance.m_registry) {
      objectCtl.deserialize(storage + objectCtl.offset, s);
    }
  }

private:
  static StaticKernelObjectStorage &GetInstance() {
    static StaticKernelObjectStorage instance;
    return instance;
  }

  std::vector<detail::StaticObjectCtl> m_registry;
  std::size_t m_size = 0;
  std::size_t m_alignment = 1;
};

template <typename NamespaceT, typename ScopeT, rx::Serializable T>
class StaticObjectRef {
  std::uint32_t mOffset;

public:
  explicit StaticObjectRef(std::uint32_t offset) : mOffset(offset) {}

  T *get() {
    return reinterpret_cast<T *>(getScopeStorage<NamespaceT, ScopeT>() +
                                 mOffset);
  }

  T *operator->() { return get(); }
};
} // namespace kernel
