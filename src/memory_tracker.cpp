#include "./memory_tracker.hpp"
#include "cli/color.hpp"
#include "cli/version.hpp"
#include "show.hpp"
#include <iostream>

namespace qat {

u64 MemoryTracker::objectsOnHeap = 0;

u64 MemoryTracker::totalSize = 0;

u64 MemoryTracker::negativeCounter = 0;

u64 MemoryTracker::getHeapCounter() { return objectsOnHeap; }

u64 MemoryTracker::getTotalSize() { return totalSize; }

void MemoryTracker::addHeapCounter() { ++objectsOnHeap; }

void MemoryTracker::incrementSize(usize size) { totalSize += size; }

void MemoryTracker::subtractHeapCounter() {
  if (objectsOnHeap > 0u) {
    --objectsOnHeap;
  } else {
    ++negativeCounter;
  }
}

void MemoryTracker::report() {
  SHOW("\nMemory Tracker")
  SHOW("Objects in heap memory :: " << objectsOnHeap)
  SHOW("Total heap memory      :: " << totalSize)
  SHOW("Unbalanced deletes     :: " << negativeCounter << "\n")
}

} // namespace qat

void* operator new(qat::usize size)
#if !PLATFORM_IS_MAC
_GLIBCXX_THROW(std::bad_alloc) 
#endif
{
  qat::MemoryTracker::addHeapCounter();
  qat::MemoryTracker::incrementSize(size);
  return malloc(size);
}

void operator delete(void* ptr) noexcept {
  qat::MemoryTracker::subtractHeapCounter();
  free(ptr);
}