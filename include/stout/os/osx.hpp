#ifndef __STOUT_OS_OSX_HPP__
#define __STOUT_OS_OSX_HPP__

// This file contains OSX-only OS utilities.
#ifndef __APPLE__
#error "stout/os/osx.hpp is only available on OSX systems."
#endif

#include <libproc.h>

#include <sys/sysctl.h>
#include <sys/types.h> // For pid_t.

#include <queue>
#include <set>

#include <stout/error.hpp>
#include <stout/none.hpp>
#include <stout/strings.hpp>

#include <stout/os/process.hpp>
#include <stout/os/sysctl.hpp>

namespace os {

inline Result<Process> process(pid_t pid)
{
  const Try<std::vector<kinfo_proc> >& processes =
    os::sysctl(CTL_KERN, KERN_PROC, KERN_PROC_PID, pid).table(1);

  if (processes.isError()) {
    return Error("Failed to get process via sysctl: " + processes.error());
  } else if (processes.get().size() != 1) {
    return None();
  }

  const kinfo_proc process = processes.get()[0];

  // The command line from 'process.kp_proc.p_comm' only includes the
  // first 16 characters from "arg0" (i.e., the canonical executable
  // name). We can try to get "argv" via some sysctl magic. This first
  // requires determining "argc" via KERN_PROCARGS2 followed by the
  // actual arguments via KERN_PROCARGS. This is still insufficient
  // with insufficient privilege (e.g., not being root). If we were
  // only interested in the "executable path" (i.e., the first
  // argument to 'exec' but none of the arguments) we could use
  // proc_pidpath() instead.
  Option<std::string> command = None();

#ifdef KERN_PROCARGS2
  // Looking at the source code of XNU (the Darwin kernel for OS X:
  // www.opensource.apple.com/source/xnu/xnu-1699.24.23/bsd/kern/kern_sysctl.c),
  // it appears as though KERN_PROCARGS2 writes 'argc' as the first
  // word of the returned bytes.
  Try<std::string> args = os::sysctl(CTL_KERN, KERN_PROCARGS2, pid).string();

  if (args.isSome()) {
    int argc = *((int*) args.get().data());

    if (argc > 0) {
      // Now grab the arguments.
      args = os::sysctl(CTL_KERN, KERN_PROCARGS, pid).string();

      if (args.isSome()) {
        // At this point 'args' contains the parameters to 'exec'
        // delimited by null bytes, i.e., "executable path", then
        // "arg0" (the canonical executable name), then "arg1", then
        // "arg2", etc. Sometimes there are no arguments (argc = 1) so
        // all we care about is the "executable path", but when there
        // are arguments we grab "arg0" and on assuming that "arg0"
        // really is the canonical executable name.

        // Tokenize the args by the null byte ('\0').
        std::vector<std::string> tokens =
          strings::tokenize(args.get(), std::string(1, '\0'));

        if (!tokens.empty()) {
          if (argc == 1) {
            // When there are no arguments, all we care about is the
            // "executable path".
            command = tokens[0];
          } else if (argc > 1) {
            // When there are arguments, we skip the "executable path"
            // and just grab "arg0" -> "argN", assuming "arg0" is the
            // canonical executable name. In the case that we didn't
            // get enough tokens back from KERN_PROCARGS the following
            // code will end up just keeping 'command' None (i.e.,
            // tokens.size() will be <= 0).
            tokens.erase(tokens.begin()); // Remove path.
            tokens.erase(tokens.begin() + argc, tokens.end());
            if (tokens.size() > 0) {
              command = strings::join(" ", tokens);
            }
          }
        }
      }
    }
  }
#endif

  // We also use proc_pidinfo() to get memory and CPU usage.
  // NOTE: There are several pitfalls to using proc_pidinfo().
  // In particular:
  //   -This will not work for many root processes.
  //   -This may not work for processes owned by other users.
  //   -However, this always works for processes owned by the same user.
  // This beats using task_for_pid(), which only works for the same pid.
  // For further discussion around these issues,
  // see: http://code.google.com/p/psutil/issues/detail?id=297
  proc_taskinfo task;
  int size = proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &task, sizeof(task));

  // It appears that zombie processes on OS X do not have sessions and
  // result in ESRCH.
  int session = getsid(pid);

  if (size != sizeof(task)) {
    return Process(process.kp_proc.p_pid,
                   process.kp_eproc.e_ppid,
                   process.kp_eproc.e_pgid,
                   session > 0 ? session : Option<pid_t>::none(),
                   None(),
                   None(),
                   None(),
                   command.get(std::string(process.kp_proc.p_comm)),
                   process.kp_proc.p_stat & SZOMB);
  } else {
    return Process(process.kp_proc.p_pid,
                   process.kp_eproc.e_ppid,
                   process.kp_eproc.e_pgid,
                   session > 0 ? session : Option<pid_t>::none(),
                   Bytes(task.pti_resident_size),
                   Nanoseconds(task.pti_total_user),
                   Nanoseconds(task.pti_total_system),
                   command.get(std::string(process.kp_proc.p_comm)),
                   process.kp_proc.p_stat & SZOMB);
  }
}


inline Try<std::set<pid_t> > pids()
{
  const Try<int>& maxproc = os::sysctl(CTL_KERN, KERN_MAXPROC).integer();

  if (maxproc.isError()) {
    return Error(maxproc.error());
  }

  const Try<std::vector<kinfo_proc> >& processes =
    os::sysctl(CTL_KERN, KERN_PROC, KERN_PROC_ALL).table(maxproc.get());

  if (processes.isError()) {
    return Error(processes.error());
  }

  std::set<pid_t> result;
  foreach (const kinfo_proc& process, processes.get()) {
    result.insert(process.kp_proc.p_pid);
  }
  return result;
}

} // namespace os {

#endif // __STOUT_OS_OSX_HPP__
