#pragma once
#include "orbis-config.hpp"

#include "../event.hpp"
#include "../evf.hpp"
#include "../ipmi.hpp"
#include "../osem.hpp"
#include "../thread/Thread.hpp"
#include "../thread/types.hpp"
#include "ProcessState.hpp"
#include "cpuset.hpp"
#include "orbis/AppInfo.hpp"
#include "orbis/AuthInfo.hpp"
#include "orbis/Budget.hpp"
#include "orbis/file.hpp"
#include "orbis/module/Module.hpp"
#include "rx/IdMap.hpp"
#include "rx/SharedMutex.hpp"
#include <optional>

namespace orbis {
class KernelContext;
struct Thread;
struct ProcessOps;
struct sysentvec;

struct NamedObjInfo {
  void *idptr;
  uint16_t ty;
};

struct NamedMemoryRange {
  uint64_t begin, end;

  constexpr bool operator<(const NamedMemoryRange &rhs) const {
    return end <= rhs.begin;
  }

  friend constexpr bool operator<(const NamedMemoryRange &lhs, uint64_t ptr) {
    return lhs.end <= ptr;
  }

  friend constexpr bool operator<(uint64_t ptr, const NamedMemoryRange &rhs) {
    return ptr < rhs.begin;
  }
};

enum class ProcessType : std::uint8_t {
  FreeBsd,
  Ps4,
  Ps5,
};

struct Process final {
  KernelContext *context = nullptr;
  pid_t pid = -1;
  int gfxRing = 0;
  std::uint64_t hostPid = -1;
  sysentvec *sysent = nullptr;
  ProcessState state = ProcessState::NEW;
  Process *parentProcess = nullptr;
  rx::shared_mutex mtx;
  int vmId = -1;
  ProcessType type = ProcessType::FreeBsd;
  void (*onSysEnter)(Thread *thread, int id, uint64_t *args,
                     int argsCount) = nullptr;
  void (*onSysExit)(Thread *thread, int id, uint64_t *args, int argsCount,
                    SysResult result) = nullptr;
  ptr<void> processParam = nullptr;
  uint64_t processParamSize = 0;
  const ProcessOps *ops = nullptr;
  AppInfoEx appInfo{};
  AuthInfo authInfo{};
  kstring cwd;
  kstring root = "/";
  cpuset affinity{(1 << 7) - 1};
  sint memoryContainer{1};
  sint budgetId{};
  Budget::ProcessType budgetProcessType{};
  bool isInSandbox = false;
  EventEmitter event;
  std::optional<sint> exitStatus;

  std::uint32_t sdkVersion = 0;
  std::uint64_t nextTlsSlot = 1;
  std::uint64_t lastTlsOffset = 0;

  rx::RcIdMap<EventFlag, sint, 4097, 1> evfMap;
  rx::RcIdMap<Semaphore, sint, 4097, 1> semMap;
  rx::RcIdMap<Module, ModuleHandle> modulesMap;
  rx::OwningIdMap<Thread, lwpid_t> threadsMap;
  rx::RcIdMap<orbis::File, sint> fileDescriptors;

  // Named objects for debugging
  rx::shared_mutex namedObjMutex;
  kmap<void *, kstring> namedObjNames;
  rx::OwningIdMap<NamedObjInfo, uint, 65535, 1> namedObjIds;

  kmap<std::int32_t, SigAction> sigActions;

  // Named memory ranges for debugging
  rx::shared_mutex namedMemMutex;
  kmap<NamedMemoryRange, kstring> namedMem;
};
} // namespace orbis
