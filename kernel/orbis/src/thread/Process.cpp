#include "thread/Process.hpp"
#include "KernelAllocator.hpp"
#include "KernelContext.hpp"
#include "KernelObject.hpp"
#include "kernel/KernelObject.hpp"
#include "rx/LinkedNode.hpp"
#include "rx/Serializer.hpp"
#include "rx/align.hpp"
#include "thread/Thread.hpp"
#include <algorithm>

struct ProcessIdList {
  rx::OwningIdMap<std::uint8_t, orbis::pid_t, 256, 0> pidMap;

  void serialize(rx::Serializer &s) const {
    pidMap.walk([&s](std::uint8_t id, orbis::pid_t value) {
      s.serialize(id);
      s.serialize(value);
    });

    s.serialize<std::uint8_t>(-1);
  }

  void deserialize(rx::Deserializer &s) {
    while (true) {
      auto id = s.deserialize<std::uint8_t>();
      if (id == static_cast<std::uint8_t>(-1)) {
        break;
      }

      auto value = s.deserialize<orbis::pid_t>();

      if (s.failure()) {
        break;
      }

      if (!pidMap.emplace_at(id, value)) {
        s.setFailure();
        return;
      }
    }
  }
};

static auto g_processIdList =
    orbis::createGlobalObject<kernel::LockableKernelObject<ProcessIdList>>();

orbis::pid_t orbis::allocatePid() {
  std::lock_guard lock(*g_processIdList);
  return g_processIdList->pidMap.emplace(0).first * 10000 + 1;
}

struct ProcessList {
  rx::LinkedNode<orbis::Process> *list = nullptr;

  void serialize(rx::Serializer &s) const {
    for (auto proc = list; proc != nullptr; proc = proc->next) {
      s.serialize(proc->object.pid);
      s.serialize(proc->object);
    }

    s.serialize<pid_t>(-1);
  }

  void deserialize(rx::Deserializer &s) {
    while (true) {
      auto pid = s.deserialize<pid_t>();
      if (pid == static_cast<pid_t>(-1) || s.failure()) {
        break;
      }

      auto process = orbis::createProcess(nullptr, pid);
      s.deserialize(*process);

      if (s.failure()) {
        break;
      }
    }
  }
};

static auto g_processList =
    orbis::createGlobalObject<kernel::LockableKernelObject<ProcessList>>();

void orbis::deleteProcess(orbis::Process *proc) {
  auto procNode = reinterpret_cast<rx::LinkedNode<Process> *>(proc);

  {
    std::lock_guard lock(*g_processList);
    auto next = procNode->erase();

    if (procNode == g_processList->list) {
      g_processList->list = next;
    }
  }

  kdelete(procNode);
}

orbis::Process *orbis::findProcessById(pid_t pid) {
  for (std::size_t i = 0; i < 20; ++i) {
    {
      std::lock_guard lock(*g_processList);
      for (auto proc = g_processList->list; proc != nullptr;
           proc = proc->next) {
        if (proc->object.pid == pid) {
          return &proc->object;
        }
      }
    }
    std::this_thread::sleep_for(std::chrono::microseconds(50));
  }

  return nullptr;
}

orbis::Process *orbis::findProcessByHostId(std::uint64_t pid) {
  for (std::size_t i = 0; i < 20; ++i) {
    {
      std::lock_guard lock(*g_processList);
      for (auto proc = g_processList->list; proc != nullptr;
           proc = proc->next) {
        if (proc->object.hostPid == pid) {
          return &proc->object;
        }
      }
    }

    std::this_thread::sleep_for(std::chrono::microseconds(50));
  }

  return nullptr;
}

void orbis::Process::serialize(rx::Serializer &s) const {
  Process::Storage::SerializeAll(storage, s);
}
void orbis::Process::deserialize(rx::Deserializer &s) {
  Process::Storage::DeserializeAll(storage, s);
}

orbis::Process::~Process() {
  Process::Storage::DestructAll(storage);

  auto size = sizeof(Process);
  size = rx::alignUp(size, Process::Storage::GetAlignment());
  size += Process::Storage::GetSize();

  kfree(this, size);
}

orbis::Process *orbis::createProcess(Process *parentProcess, pid_t pid) {
  if (pid == static_cast<pid_t>(-1)) {
    pid = allocatePid();
  }

  auto size = sizeof(rx::LinkedNode<Process>);
  size = rx::alignUp(size, Process::Storage::GetAlignment());
  auto storageOffset = size;
  size += Process::Storage::GetSize();

  auto memory = (std::byte *)kalloc(
      size, std::max<std::size_t>(alignof(rx::LinkedNode<Process>),
                                  Process::Storage::GetAlignment()));

  auto result = new (memory) rx::LinkedNode<Process>();
  result->object.context = g_context.get();
  result->object.storage = memory + storageOffset;
  result->object.parentProcess = parentProcess;
  result->object.pid = pid;
  Process::Storage::ConstructAll(result->object.storage);

  std::lock_guard lock(*g_processList);
  if (auto list = g_processList->list) {
    list->insertPrev(*result);
  }
  g_processList->list = result;
  return &result->object;
}
