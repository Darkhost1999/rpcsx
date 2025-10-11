#pragma once

#include "KernelObject.hpp"
#include "ThreadState.hpp"
#include "cpuset.hpp"
#include "orbis-config.hpp"
#include "rx/Serializer.hpp"
#include "types.hpp"

#include "../KernelAllocator.hpp"
#include "../ucontext.hpp"
#include "rx/SharedAtomic.hpp"
#include "rx/SharedCV.hpp"
#include "rx/SharedMutex.hpp"
#include <thread>

namespace orbis {
struct Process;

static constexpr std::uint32_t kThreadSuspendFlag = 1 << 31;
struct Thread final {
  using Storage =
      kernel::StaticKernelObjectStorage<OrbisNamespace,
                                        kernel::detail::ThreadScope>;

  rx::shared_mutex mtx;
  Process *tproc = nullptr;
  std::byte *storage = nullptr;
  uint64_t retval[2]{};
  void *context{};
  kvector<void *> altStack;
  ptr<void> stackStart;
  ptr<void> stackEnd;
  uint64_t fsBase{};
  uint64_t gsBase{};
  char name[32]{};

  cpuset affinity{~0u};
  SigSet sigMask = {0x7fff'ffff, ~0u, ~0u, ~0u};
  rtprio prio{
      .type = 2,
      .prio = 10,
  };
  rx::shared_mutex suspend_mtx;
  rx::shared_cv suspend_cv;
  kvector<UContext> sigReturns;
  kvector<SigInfo> blockedSignals;
  kvector<SigInfo> queuedSignals;
  rx::shared_atomic32 suspendFlags{0};

  rx::shared_atomic32 interruptedMtx{0};

  std::int64_t hostTid = -1;
  lwpid_t tid = -1;
  unsigned unblocked = 0;
  ThreadState state = ThreadState::INACTIVE;
  std::thread handle;
  std::thread::native_handle_type nativeHandle;

  // Used to wake up thread in sleep queue
  rx::shared_cv sync_cv;
  uint64_t evfResultPattern;
  uint64_t evfIsCancelled;

  [[nodiscard]] std::thread::native_handle_type getNativeHandle() {
    if (handle.joinable()) {
      return handle.native_handle();
    }

    return nativeHandle;
  }

  // Print backtrace
  void where();

  bool unblock();
  bool block();

  void suspend();
  void resume();
  void sendSignal(int signo);
  void notifyUnblockedSignal(int signo);
  void setSigMask(SigSet newSigMask);

  void allocate() {
    if (auto size = Storage::GetSize()) {
      storage = (std::byte *)kalloc(size, Storage::GetAlignment());
    }
  }

  void deallocate() {
    if (auto size = Storage::GetSize()) {
      kfree(storage, size);
      storage = nullptr;
    }
  }

  template <rx::Serializable T>
  T *get(kernel::StaticObjectRef<OrbisNamespace, kernel::detail::ThreadScope, T>
             ref) {
    return ref.get(storage);
  }

  // FIXME: implement thread destruction
  void incRef() {}
  void decRef() {}
};

extern thread_local Thread *g_currentThread;

struct scoped_unblock {
  scoped_unblock();
  ~scoped_unblock();

  scoped_unblock(const scoped_unblock &) = delete;
};

class scoped_unblock_now {
  bool unblocked = false;

public:
  scoped_unblock_now() {
    if (g_currentThread && g_currentThread->context) {
      g_currentThread->unblock();
      unblocked = true;
    }
  }

  ~scoped_unblock_now() {
    if (unblocked) {
      g_currentThread->block();
    }
  }

  scoped_unblock_now(const scoped_unblock_now &) = delete;
};
} // namespace orbis
