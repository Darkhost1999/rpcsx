#pragma once

#include "error/ErrorCode.hpp"
#include "rx/Rc.hpp"

namespace orbis {
enum OpenFlags {
  kOpenFlagReadOnly = 0x0,
  kOpenFlagWriteOnly = 0x1,
  kOpenFlagReadWrite = 0x2,
  kOpenFlagNonBlock = 0x4,
  kOpenFlagAppend = 0x8,
  kOpenFlagShLock = 0x10,
  kOpenFlagExLock = 0x20,
  kOpenFlagAsync = 0x40,
  kOpenFlagFsync = 0x80,
  kOpenFlagCreat = 0x200,
  kOpenFlagTrunc = 0x400,
  kOpenFlagExcl = 0x800,
  kOpenFlagDSync = 0x1000,
  kOpenFlagDirect = 0x10000,
  kOpenFlagDirectory = 0x20000,
};

struct File;
struct Thread;

struct IoDevice : rx::RcBase {
  virtual ErrorCode open(rx::Ref<File> *file, const char *path,
                         std::uint32_t flags, std::uint32_t mode,
                         Thread *thread) = 0;

  virtual ErrorCode unlink(const char *path, bool recursive, Thread *thread) {
    return ErrorCode::NOTSUP;
  }

  virtual ErrorCode createSymlink(const char *target, const char *linkPath,
                                  Thread *thread) {
    return ErrorCode::NOTSUP;
  }

  virtual ErrorCode mkdir(const char *path, int mode, Thread *thread) {
    return ErrorCode::NOTSUP;
  }

  virtual ErrorCode rmdir(const char *path, Thread *thread) {
    return ErrorCode::NOTSUP;
  }

  virtual ErrorCode rename(const char *from, const char *to, Thread *thread) {
    return ErrorCode::NOTSUP;
  }
};
} // namespace orbis
