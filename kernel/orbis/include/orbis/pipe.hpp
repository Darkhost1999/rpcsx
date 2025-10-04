#pragma once

#include "KernelAllocator.hpp"
#include "file.hpp"
#include "rx/SharedCV.hpp"
#include "rx/SharedMutex.hpp"
#include "utils/Rc.hpp"
#include <utility>

namespace orbis {
struct Pipe final : File {
  rx::shared_cv cv;
  kvector<std::byte> data;
  Ref<Pipe> other;
};

std::pair<Ref<Pipe>, Ref<Pipe>> createPipe();
} // namespace orbis
