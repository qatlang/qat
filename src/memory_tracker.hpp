#ifndef QAT_MEMORY_TRACKER_HPP
#define QAT_MEMORY_TRACKER_HPP

#include "./utils/helpers.hpp"
#include "./utils/macros.hpp"
#include <map>

namespace qat {

class MemoryTracker {
private:
  static u64 objectsOnHeap;
  static u64 totalSize;
  static u64 negativeCounter;

public:
  useit static u64 getHeapCounter();
  useit static u64 getTotalSize();
  static void      addHeapCounter();
  static void      incrementSize(usize size);
  static void      subtractHeapCounter();
  static void      report();
};

} // namespace qat

void* operator new(qat::usize size)
#if !PLATFORM_IS_MAC
_GLIBCXX_THROW(std::bad_alloc)
#endif
; // NOLINT(readability-redundant-declaration)

void operator delete(void* ptr) noexcept; // NOLINT(readability-redundant-declaration)

#endif