#include "./qat_region.hpp"
#include "../memory_tracker.hpp"
#include "../show.hpp"
#include "helpers.hpp"
#include <type_traits>

namespace qat {

thread_local void* TrackedRegion::blockTail   = nullptr;
std::mutex         TrackedRegion::regionMutex = std::mutex();
Vec<void*>         TrackedRegion::allBlockTails{};

thread_local void* QatRegion::blockTail   = nullptr;
std::mutex         QatRegion::regionMutex = std::mutex();
Vec<void*>         QatRegion::allBlockTails{};

void TrackedRegion::destroyMembers() {
  while (!regionMutex.try_lock()) {
  }
  for (auto currBlockTail : allBlockTails) {
    auto candidatePtr = currBlockTail;
    while (candidatePtr != nullptr) {
      auto nextBlockPtr   = *(void**)candidatePtr;
      auto blockSizePtr   = (usize*)(((void**)candidatePtr) + 1);
      auto blockOffsetPtr = blockSizePtr + 1;
      auto unitSizePtr    = blockOffsetPtr + 1;
      auto lastPtr        = ((u8*)candidatePtr) + *blockOffsetPtr;
      while (lastPtr != (u8*)unitSizePtr) {
        auto unitDataPtr  = (void*)(unitSizePtr + 1);
        auto unitDestrPtr = (DestructorFnPtrTy*)(((u8*)(unitDataPtr)) + *unitSizePtr);
        if (*unitDestrPtr != nullptr) {
          (*unitDestrPtr)(unitDataPtr);
        }
        unitSizePtr = (usize*)(unitDestrPtr + 1);
      }
      free(candidatePtr);
      candidatePtr = nextBlockPtr;
    }
  }
  allBlockTails.clear();
  regionMutex.unlock();
}

void* TrackedRegion::getMemory(DestructorFnPtrTy dstrFn, usize typeSize) {
  constexpr auto defaultBlockSize = 4096;
  constexpr auto usizeSize        = sizeof(usize);
  constexpr auto u8PtrSize        = sizeof(u8*);
  constexpr auto fnPtrSize        = sizeof(DestructorFnPtrTy);
  void**         nextBlockPtr     = nullptr;
  usize*         blockSizePtr     = nullptr;
  usize*         blockOffsetPtr   = nullptr;
  auto           unitSize         = (usizeSize + typeSize + fnPtrSize);
  if (blockTail == nullptr) {
    auto blockSize = (unitSize < (defaultBlockSize - (2 * usizeSize) - u8PtrSize)) ? defaultBlockSize : (2 * unitSize);
    blockTail      = malloc(blockSize);
    MemoryTracker::incrementSize(blockSize);
    if (blockTail == nullptr) {
      exit(1);
    } else {
      // Updating blockTails for each new thread requesting for memory
      while (!regionMutex.try_lock()) {
      }
      allBlockTails.push_back(blockTail);
      regionMutex.unlock();
      //
      nextBlockPtr    = (void**)blockTail;
      blockSizePtr    = (usize*)(nextBlockPtr + 1);
      blockOffsetPtr  = blockSizePtr + 1;
      *nextBlockPtr   = nullptr;
      *blockSizePtr   = blockSize;
      *blockOffsetPtr = (2 * usizeSize) + u8PtrSize;
    }
  } else {
    nextBlockPtr   = (void**)blockTail;
    blockSizePtr   = (usize*)(nextBlockPtr + 1);
    blockOffsetPtr = blockSizePtr + 1;
  }
  if ((*blockSizePtr - (2 * usizeSize) - u8PtrSize - *blockOffsetPtr) < unitSize) {
    auto blockSize = (unitSize < (defaultBlockSize - (2 * usizeSize) - u8PtrSize)) ? defaultBlockSize : (2 * unitSize);
    auto newBlockTail = malloc(blockSize);
    MemoryTracker::incrementSize(blockSize);
    if (newBlockTail == nullptr) {
      exit(1);
    } else {
      *nextBlockPtr   = (void*)newBlockTail;
      nextBlockPtr    = (void**)newBlockTail;
      blockSizePtr    = (usize*)(nextBlockPtr + 1);
      blockOffsetPtr  = blockSizePtr + 1;
      *nextBlockPtr   = nullptr;
      *blockSizePtr   = blockSize;
      *blockOffsetPtr = (2 * usizeSize) + u8PtrSize;
      blockTail       = newBlockTail;
    }
  }
  auto unitTypeSizePtr      = (usize*)(((u8*)blockTail) + (*blockOffsetPtr));
  auto unitDataPtr          = (u8*)(unitTypeSizePtr + 1);
  auto unitDestructorPtrPtr = (DestructorFnPtrTy*)(unitDataPtr + typeSize);
  *unitTypeSizePtr          = typeSize;
  *unitDestructorPtrPtr     = dstrFn;
  *blockOffsetPtr += unitSize;
  return (void*)unitDataPtr;
}

void QatRegion::destroyAllBlocks() {
  while (!regionMutex.try_lock()) {
  }
  for (auto currBlockTail : allBlockTails) {
    auto candidatePtr = currBlockTail;
    while (candidatePtr != nullptr) {
      auto nextBlockPtr = *(void**)candidatePtr;
      free(candidatePtr);
      candidatePtr = nextBlockPtr;
    }
  }
  allBlockTails.clear();
  regionMutex.unlock();
}

void* QatRegion::getMemory(usize typeSize) {
  constexpr auto defaultBlockSize = 16384;
  constexpr auto usizeSize        = sizeof(usize);
  constexpr auto u8PtrSize        = sizeof(u8*);
  void**         nextBlockPtr     = nullptr;
  usize*         blockSizePtr     = nullptr;
  usize*         blockOffsetPtr   = nullptr;
  auto           unitSize         = typeSize;
  if (blockTail == nullptr) {
    auto blockSize = (unitSize < (defaultBlockSize - (2 * usizeSize) - u8PtrSize)) ? defaultBlockSize : (2 * unitSize);
    blockTail      = malloc(blockSize);
    if (blockTail == nullptr) {
      exit(1);
    } else {
      MemoryTracker::incrementSize(blockSize);
      // Updating blockTails for each new thread requesting for memory
      while (!regionMutex.try_lock()) {
      }
      allBlockTails.push_back(blockTail);
      regionMutex.unlock();
      //
      nextBlockPtr    = (void**)blockTail;
      blockSizePtr    = (usize*)(nextBlockPtr + 1);
      blockOffsetPtr  = blockSizePtr + 1;
      *nextBlockPtr   = nullptr;
      *blockSizePtr   = blockSize;
      *blockOffsetPtr = (2 * usizeSize) + u8PtrSize;
    }
  } else {
    nextBlockPtr   = (void**)blockTail;
    blockSizePtr   = (usize*)(nextBlockPtr + 1);
    blockOffsetPtr = blockSizePtr + 1;
  }
  if ((*blockSizePtr - (2 * usizeSize) - u8PtrSize - *blockOffsetPtr) < unitSize) {
    auto blockSize = (unitSize < (defaultBlockSize - (2 * usizeSize) - u8PtrSize)) ? defaultBlockSize : (2 * unitSize);
    auto newBlockTail = malloc(blockSize);
    if (newBlockTail == nullptr) {
      exit(1);
    } else {
      MemoryTracker::incrementSize(blockSize);
      *nextBlockPtr   = (void*)newBlockTail;
      nextBlockPtr    = (void**)newBlockTail;
      blockSizePtr    = (usize*)(nextBlockPtr + 1);
      blockOffsetPtr  = blockSizePtr + 1;
      *nextBlockPtr   = nullptr;
      *blockSizePtr   = blockSize;
      *blockOffsetPtr = (2 * usizeSize) + u8PtrSize;
      blockTail       = newBlockTail;
    }
  }
  auto unitDataPtr = (u8*)(((u8*)blockTail) + (*blockOffsetPtr));
  *blockOffsetPtr += unitSize;
  return (void*)unitDataPtr;
}

} // namespace qat