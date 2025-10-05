#pragma once

#include "rx/LinkedNode.hpp"
#include "rx/Serializer.hpp"
#include <cassert>

namespace kernel {

namespace detail {
struct GlobalObjectCtl {
  void (*construct)();
  void (*destruct)();
  void (*serialize)(rx::Serializer &);
  void (*deserialize)(rx::Deserializer &);
};

template <typename NamespaceT, typename T> struct GlobalKernelObjectInstance {
  static inline T *instance = nullptr;

  static inline rx::LinkedNode<GlobalObjectCtl> ctl = {
      .object = {
          .construct = +[] { instance->construct(); },
          .destruct = +[] { instance->destruct(); },
          .serialize = +[](rx::Serializer &s) { instance->serialize(s); },
          .deserialize = +[](rx::Deserializer &s) { instance->deserialize(s); },
      },
  };
};
} // namespace detail

template <typename NamespaceT> struct GlobalKernelObjectStorage {
  template <typename T> static void AddObject() {
    auto node = &detail::GlobalKernelObjectInstance<NamespaceT, T>::ctl;
    auto head = GetHead();
    if (head) {
      head->prev = node;
      node->next = head;
    }

    *GetHeadPtr() = node;
  }

  static void ConstructAll() {
    for (auto it = GetHead(); it != nullptr; it = it->next) {
      it->object.construct();
    }
  }

  static void DestructAll() {
    for (auto it = GetHead(); it != nullptr; it = it->next) {
      it->object.destruct();
    }
  }

  static void SerializeAll(rx::Serializer &s) {
    for (auto it = GetHead(); it != nullptr; it = it->next) {
      it->object.serialize(s);
    }
  }

  static void DeserializeAll(rx::Deserializer &s) {
    for (auto it = GetHead(); it != nullptr; it = it->next) {
      it->object.deserialize(s);
    }
  }

private:
  static rx::LinkedNode<detail::GlobalObjectCtl> *GetHead() {
    return *GetHeadPtr();
  }

  static rx::LinkedNode<detail::GlobalObjectCtl> **GetHeadPtr() {
    static rx::LinkedNode<detail::GlobalObjectCtl> *registry;
    return &registry;
  }
};

template <rx::Serializable T, typename NamespaceT>
  requires std::is_default_constructible_v<T>
class GlobalKernelObject {
  union U {
    T object;

    U() {}
    ~U() {}
  };

  U mHolder;

public:
  template <typename = void> GlobalKernelObject() {
    auto &instance =
        detail::GlobalKernelObjectInstance<NamespaceT,
                                           GlobalKernelObject>::instance;
    assert(instance == nullptr);
    instance = this;
    GlobalKernelObjectStorage<NamespaceT>::template AddObject<
        GlobalKernelObject>();
  }

  T *operator->() { return &mHolder.object; }
  const T *operator->() const { return &mHolder.object; }
  T &operator*() { return mHolder.object; }
  const T &operator*() const { return mHolder.object; }
  operator T &() { return mHolder.object; }
  operator const T &() const { return mHolder.object; }

  void serialize(rx::Serializer &s)
    requires rx::Serializable<T>
  {
    s.serialize(mHolder.object);
  }

  void deserialize(rx::Deserializer &s)
    requires rx::Serializable<T>
  {
    std::construct_at(&mHolder.object);
    s.deserialize(mHolder.object);
  }

  T &get() { return mHolder.object; }
  const T &get() const { return mHolder.object; }

private:
  template <typename... Args>
    requires(std::is_constructible_v<T, Args && ...>)
  void construct(Args &&...args) noexcept(
      std::is_nothrow_constructible_v<T, Args &&...>) {
    std::construct_at(&mHolder.object, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void destruct() noexcept(std::is_nothrow_destructible_v<T>) {
    mHolder.object.~T();
  }

  friend detail::GlobalKernelObjectInstance<NamespaceT, GlobalKernelObject>;
};
} // namespace kernel
