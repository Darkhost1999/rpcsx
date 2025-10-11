#include "IoDevice.hpp"
#include "thread/Thread.hpp"
#include "utils/Logs.hpp"

static std::string iocGroupToString(unsigned iocGroup) {
  if (iocGroup >= 128) {
    const char *sceGroups[] = {
        "DEV",
        "DMEM",
        "GC",
        "DCE",
        "UVD",
        "VCE",
        "DBGGC",
        "TWSI",
        "MDBG",
        "DEVENV",
        "AJM",
        "TRACE",
        "IBS",
        "MBUS",
        "HDMI",
        "CAMERA",
        "FAN",
        "THERMAL",
        "PFS",
        "ICC_CONFIG",
        "IPC",
        "IOSCHED",
        "ICC_INDICATOR",
        "EXFATFS",
        "ICC_NVS",
        "DVE",
        "ICC_POWER",
        "AV_CONTROL",
        "ICC_SC_CONFIGURATION",
        "ICC_DEVICE_POWER",
        "SSHOT",
        "DCE_SCANIN",
        "FSCTRL",
        "HMD",
        "SHM",
        "PHYSHM",
        "HMDDFU",
        "BLUETOOTH_HID",
        "SBI",
        "S3DA",
        "SPM",
        "BLOCKPOOL",
        "SDK_EVENTLOG",
    };

    if (iocGroup - 127 >= std::size(sceGroups)) {
      return "'?'";
    }

    return sceGroups[iocGroup - 127];
  }

  if (isprint(iocGroup)) {
    return "'" + std::string(1, (char)iocGroup) + "'";
  }

  return "'?'";
}

orbis::ErrorCode orbis::IoDevice::ioctl(std::uint64_t request,
                                        orbis::ptr<void> argp, Thread *thread) {
  auto group = iocGroupToString(ioctl::group(request));
  auto paramSize = ioctl::paramSize(request);
  ORBIS_LOG_ERROR("unhandled ioctl", request, group, paramSize, argp,
                  thread->tid);
  thread->where();
  return ErrorCode::NOTSUP;
}
