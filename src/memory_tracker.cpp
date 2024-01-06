#include "./memory_tracker.hpp"
#include "cli/color.hpp"
#include "cli/version.hpp"
#include "show.hpp"
#include "utils/logger.hpp"
#include "utils/qat_region.hpp"
#include <iostream>

namespace qat {

u64 MemoryTracker::totalSize = 0;

u64 MemoryTracker::getTotalSize() { return totalSize; }

void MemoryTracker::incrementSize(usize size) { totalSize += size; }

void MemoryTracker::report() {
  auto& log = qat::Logger::get();
  log->diagnostic("Memory Used   -> " + (totalSize > 1048576
                                             ? (std::to_string(((double)totalSize) / 1048576) + " MB")
                                             : (totalSize > 1024 ? (std::to_string((double)totalSize / 1024) + " KB")
                                                                 : (std::to_string(totalSize) + " bytes"))));
}

} // namespace qat

void* operator new(qat::usize size)
#if PlatformIsLinux
    _GLIBCXX_THROW(std::bad_alloc)
#endif
{
  qat::MemoryTracker::incrementSize(size);
  return malloc(size);
}