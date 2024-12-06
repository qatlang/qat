#ifndef QAT_WORKGROUP_HPP
#define QAT_WORKGROUP_HPP

#include "helpers.hpp"
#include <thread>

namespace qat {

class Workgroup {
  Vec<std::thread> workers;

public:
  void add(std::thread thr) { workers.push_back(std::move(thr)); }
  void wait() {
    for (auto& thr : workers) {
      thr.join();
    }
    workers.clear();
  }
};

} // namespace qat

#endif
