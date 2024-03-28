#ifndef QAT_REGION_HPP
#define QAT_REGION_HPP

#include "helpers.hpp"
#include <mutex>

#define OwnNormal(TYPE_NAME) (TYPE_NAME*)QatRegion::getMemory(sizeof(TYPE_NAME))

namespace qat {

class QatRegion {
  thread_local static void* blockTail;
  static Vec<void*>         allBlockTails;
  static std::mutex         regionMutex;

public:
  static void* getMemory(usize size);
  static void  destroyAllBlocks();
};

} // namespace qat

#endif