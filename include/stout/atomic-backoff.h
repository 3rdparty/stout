#pragma once

#include "stout/thread.h"

////////////////////////////////////////////////////////////////////////

namespace stout {

////////////////////////////////////////////////////////////////////////

class AtomicBackoff {
 public:
  AtomicBackoff(size_t pauses_before_yield = 16, size_t pauses = 1)
    : pauses_before_yield(pauses_before_yield),
      pauses(pauses) {}

  AtomicBackoff(const AtomicBackoff&) = delete;
  AtomicBackoff& operator=(const AtomicBackoff&) = delete;

 public:
  void pause() {
    if (pauses <= pauses_before_yield) {
      this_thread::pause();
      pauses *= 2;
    } else {
      std::this_thread::yield();
    }
  }

 private:
  const size_t pauses_before_yield;
  size_t pauses;
};

////////////////////////////////////////////////////////////////////////

} // namespace stout

////////////////////////////////////////////////////////////////////////
