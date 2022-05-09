#include "stout/atomic-backoff.hpp"

#include "gtest/gtest.h"

TEST(AtomicBackoffTest, Backoff) {
  size_t i = 0;
  for (stout::AtomicBackoff b;; b.pause()) {
    if (i < 10) {
      break;
    }
  }
}
