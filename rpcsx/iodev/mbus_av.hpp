#pragma once

#include "io-device.hpp"
#include "iodev/MBusEvent.hpp"
#include "rx/SharedCV.hpp"
#include "rx/SharedMutex.hpp"

struct MBusAVDevice : IoDevice {
  rx::shared_mutex mtx;
  rx::shared_cv cv;
  orbis::kdeque<MBusEvent> events;
  rx::Ref<orbis::EventEmitter> eventEmitter =
      orbis::knew<orbis::EventEmitter>();

  orbis::ErrorCode open(rx::Ref<orbis::File> *file, const char *path,
                        std::uint32_t flags, std::uint32_t mode,
                        orbis::Thread *thread) override;

  void emitEvent(const MBusEvent &event);
};
