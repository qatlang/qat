#ifndef QAT_MEMORY_TRACKER_HPP
#define QAT_MEMORY_TRACKER_HPP

#include "./utils/helpers.hpp"
#include "./utils/macros.hpp"
#include <map>

namespace qat {

class MemoryTracker {
private:
  static u64 totalSize;

public:
  useit static u64 getTotalSize();
  static void      incrementSize(usize size);
  static void      report();
};

} // namespace qat

void* operator new(qat::usize size)
#if PlatformIsLinux
    _GLIBCXX_THROW(std::bad_alloc)
#endif
        ; // NOLINT(readability-redundant-declaration)
#endif