#ifndef __STOUT_OS_SIGNALS_HPP__
#define __STOUT_OS_SIGNALS_HPP__

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

namespace os {

namespace signals {

// Returns true iff the signal is pending.
inline bool pending(int signal)
{
  sigset_t set;
  sigemptyset(&set);
  sigpending(&set);
  return sigismember(&set, signal);
}


// Returns true if the signal has been blocked, or false if the
// signal was already blocked.
inline bool block(int signal)
{
  sigset_t set;
  sigaddset(&set, signal);

  sigset_t oldset;
  sigemptyset(&oldset);

  // We ignore errors here as the only documented one is
  // EINVAL due to a bad value of the SIG_* argument.
  pthread_sigmask(SIG_BLOCK, &set, &oldset);

  return !sigismember(&oldset, signal);
}


// Returns true if the signal has been unblocked, or false if the
// signal was not previously blocked.
inline bool unblock(int signal)
{
  sigset_t set;
  sigaddset(&set, signal);

  sigset_t oldset;
  sigemptyset(&oldset);

  pthread_sigmask(SIG_UNBLOCK, &set, &oldset);

  return sigismember(&oldset, signal);
}

namespace internal {

// Suppresses a signal on the current thread for the lifetime of
// the Suppressor. The signal *must* be synchronous and delivered
// per-thread. The suppression occurs only on the thread of
// execution of the Suppressor.
struct Suppressor
{
  Suppressor(int _signal)
    : signal(_signal), pending(false), unblock(false)
  {
    // Check to see if the signal is already reported as pending.
    // If pending, it means the thread already blocks the signal!
    // Therefore, any new instances of the signal will also be
    // blocked and merged with the pending one since there is no
    // queuing for signals.
    pending = signals::pending(signal);

    if (!pending) {
      // Block the signal for this thread only. If already blocked,
      // there's no need to unblock it.
      unblock = signals::block(signal);
    }
  }

  ~Suppressor()
  {
    // We want to preserve errno when the Suppressor drops out of
    // scope. Otherwise, one needs to potentially store errno when
    // using the suppress() macro.
    int _errno = errno;

    // If the signal has become pending after we blocked it, we
    // need to clear it before unblocking it.
    if (!pending && signals::pending(signal)) {
      // It is possible that in between having observed the pending
      // signal with sigpending() and clearing it with sigwait(),
      // the signal was delivered to another thread before we were
      // able to clear it here. This can happen if the signal was
      // generated for the whole process (e.g. a kill was issued).
      // See 2.4.1 here:
      // http://pubs.opengroup.org/onlinepubs/009695399/functions/xsh_chap02_04.html
      // To handle the above scenario, one can either:
      //   1. Use sigtimedwait() with a timeout of 0, to ensure we
      //      don't block forever. However, this only works on Linux
      //      and we may still swallow the signal intended for the
      //      process.
      //   2. After seeing the pending signal, signal ourselves with
      //      pthread_kill prior to calling sigwait(). This can still
      //      swallow the signal intended for the process.
      // We chose to use the latter technique as it works on all
      // POSIX systems and is less likely to swallow process signals,
      // provided the thread signal and process signal are not merged.
      pthread_kill(pthread_self(), signal);

      sigset_t mask;
      sigemptyset(&mask);
      sigaddset(&mask, signal);

      int result;
      do {
        int _ignored;
        result = sigwait(&mask, &_ignored);
      } while (result == -1 && errno == EINTR);
    }

    // Unblock the signal (only if we were the ones to block it).
    if (unblock) {
      signals::unblock(signal);
    }

    // Restore errno.
    errno = _errno;
  }

  // Needed for the suppress() macro.
  operator bool () { return true; }

private:
  const int signal;
  bool pending; // Whether the signal is already pending.
  bool unblock; // Whether to unblock the signal on destruction.
};

} // namespace internal {

#define suppress(signal) \
  if (os::signals::internal::Suppressor suppressor ## signal = \
      os::signals::internal::Suppressor(signal))

} // namespace signals {

} // namespace os {

#endif // __STOUT_OS_SIGNALS_HPP__
