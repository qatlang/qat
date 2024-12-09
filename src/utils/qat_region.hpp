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
	static usize              totalSize;

  public:
	static void* getMemory(usize size);
	static void  destroyAllBlocks();
	static usize get_total_size() { return totalSize; }
};

} // namespace qat

#endif
