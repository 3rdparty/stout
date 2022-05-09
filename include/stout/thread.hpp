#pragma once

#include <thread>

////////////////////////////////////////////////////////////////////////

namespace stout {

////////////////////////////////////////////////////////////////////////

namespace this_thread {

////////////////////////////////////////////////////////////////////////

inline void pause() {
#if (defined(_M_X64) || defined(__x86_64__)) && !defined(_MSC_VER)
  __asm__ volatile("pause"
                   :
                   :
                   : "memory");
#elif (defined(_M_X64) || defined(__x86_64__)) && defined(_MSC_VER)
  // On this architecture `__asm__` , `__asm` , `asm` are not recognized.
  // We use `_mm_pause()` and `_ReadWriteBarrier()` instead. See the link
  // below:
  // https://github.com/facebook/folly/blob/main/folly/portability/Asm.h
  ::_mm_pause();
  ::_ReadWriteBarrier();
#elif defined(_M_IX86) || defined(__i386__) || defined(__i386)
  __asm__ volatile("pause"
                   :
                   :
                   : "memory");
#else
  std::this_thread::yield();
#endif
}

////////////////////////////////////////////////////////////////////////

} // namespace this_thread

////////////////////////////////////////////////////////////////////////

} // namespace stout

////////////////////////////////////////////////////////////////////////
