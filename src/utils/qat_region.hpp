#ifndef QAT_REGION_HPP
#define QAT_REGION_HPP

#include "helpers.hpp"
#include <functional>
#include <mutex>
#include <type_traits>

typedef void (*DestructorFnPtrTy)(void*);

#define GetFromRegion(TYPE_NAME)                                                                                       \
  (TYPE_NAME*)QatRegion::getMemory(                                                                                    \
      std::is_destructible<TYPE_NAME>::value                                                                           \
          ? ((void (*)(void*))([](void* TYPE_INSTANCE) { ((TYPE_NAME*)TYPE_INSTANCE)->TYPE_NAME::~TYPE_NAME(); }))     \
          : nullptr,                                                                                                   \
      sizeof(TYPE_NAME))

namespace qat {

class QatRegion {
  thread_local static void* blockTail;
  static Vec<void*>         allBlockTails;
  static std::mutex         regionMutex;

public:
  static void* getMemory(DestructorFnPtrTy dstrFn, usize size);
  static void  destroyAllBlocks();
};

} // namespace qat

#endif