#include "./qat_region.hpp"
#include "helpers.hpp"

namespace qat {

thread_local void* QatRegion::blockTail   = nullptr;
std::mutex         QatRegion::regionMutex = std::mutex();
Vec<void*>         QatRegion::allBlockTails{};

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