// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <fcntl.h>
#include <glog/logging.h>
#include <limits.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include <list>
#include <queue>
#include <set>
#include <string>

#include "stout/bytes.h"
#include "stout/duration.h"
#include "stout/error.h"
#include "stout/exit.h"
#include "stout/foreach.h"
#include "stout/none.h"
#include "stout/nothing.h"
#include "stout/option.h"
#include "stout/os/access.h"
#include "stout/os/bootid.h"
#include "stout/os/chdir.h"
#include "stout/os/chroot.h"
#include "stout/os/dup.h"
#include "stout/os/exists.h"
#include "stout/os/fcntl.h"
#include "stout/os/getenv.h"
#include "stout/os/int_fd.h"
#include "stout/os/kill.h"
#include "stout/os/ls.h"
#include "stout/os/lseek.h"
#include "stout/os/lsof.h"
#include "stout/os/mkdir.h"
#include "stout/os/mkdtemp.h"
#include "stout/os/mktemp.h"
#include "stout/os/os.h"
#include "stout/os/pagesize.h"
#include "stout/os/pipe.h"
#include "stout/os/process.h"
#include "stout/os/raw/argv.h"
#include "stout/os/raw/environment.h"
#include "stout/os/rename.h"
#include "stout/os/rm.h"
#include "stout/os/rmdir.h"
#include "stout/os/shell.h"
#include "stout/os/stat.h"
#include "stout/os/su.h"
#include "stout/os/temp.h"
#include "stout/os/touch.h"
#include "stout/os/utime.h"
#include "stout/os/wait.h"
#include "stout/os/xattr.h"
#include "stout/path.h"
#include "stout/result.h"
#include "stout/strings.h"
#include "stout/try.h"
#include "stout/version.h"

// For readability, we minimize the number of #ifdef blocks in the code by
// splitting platform specific system calls into separate directories.
#ifdef _WIN32
#include "stout/windows/os.h"
#else
#include "stout/posix/os.h"
#endif // _WIN32

////////////////////////////////////////////////////////////////////////

namespace os {

////////////////////////////////////////////////////////////////////////

namespace libraries {

////////////////////////////////////////////////////////////////////////

namespace Library {

////////////////////////////////////////////////////////////////////////

// Library prefix; e.g., the `lib` in `libprocess`. NOTE: there is no prefix
// on Windows; `libprocess.a` would be `process.lib`.
constexpr const char* prefix =
#ifdef _WIN32
    "";
#else
    "lib";
#endif // _WIN32


// The suffix for a shared library; e.g., `.so` on Linux.
constexpr const char* extension =
#ifdef __APPLE__
    ".dylib";
#elif defined(_WIN32)
    ".dll";
#else
        ".so";
#endif // __APPLE__


// The name of the environment variable that contains paths on which the
// linker should search for libraries. NOTE: Windows does not have an
// environment variable that controls the paths the linker searches through.
constexpr const char* ldPathEnvironmentVariable =
#ifdef __APPLE__
    "DYLD_LIBRARY_PATH";
#elif defined(_WIN32)
    "";
#else
    "LD_LIBRARY_PATH";
#endif

////////////////////////////////////////////////////////////////////////

} // namespace Library

////////////////////////////////////////////////////////////////////////

// Returns the full library name by adding prefix and extension to
// library name.
inline std::string expandName(const std::string& libraryName) {
  return Library::prefix + libraryName + Library::extension;
}

////////////////////////////////////////////////////////////////////////

// Returns the current value of LD_LIBRARY_PATH environment variable.
inline std::string paths() {
  const Option<std::string> path = getenv(Library::ldPathEnvironmentVariable);
  return path.isSome() ? path.get() : std::string();
}

////////////////////////////////////////////////////////////////////////

// Updates the value of LD_LIBRARY_PATH environment variable.
// Note that setPaths has an effect only for child processes
// launched after calling it.
inline void setPaths(const std::string& newPaths) {
  os::setenv(Library::ldPathEnvironmentVariable, newPaths);
}

////////////////////////////////////////////////////////////////////////

// Append newPath to the current value of LD_LIBRARY_PATH environment
// variable.
inline void appendPaths(const std::string& newPaths) {
  if (paths().empty()) {
    setPaths(newPaths);
  } else {
    setPaths(paths() + ":" + newPaths);
  }
}

////////////////////////////////////////////////////////////////////////

} // namespace libraries

////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
inline Try<std::string> sysname() = delete;
#else
// Return the operating system name (e.g. Linux).
inline Try<std::string> sysname() {
  Try<UTSInfo> info = uname();
  if (info.isError()) {
    return Error(info.error());
  }

  return info->sysname;
}
#endif // _WIN32

////////////////////////////////////////////////////////////////////////

inline Try<std::list<Process>> processes() {
  const Try<std::set<pid_t>> pids = os::pids();
  if (pids.isError()) {
    return Error(pids.error());
  }

  std::list<Process> result;
  foreach (pid_t pid, pids.get()) {
    const Result<Process> process = os::process(pid);

    // Ignore any processes that disappear between enumeration and now.
    if (process.isSome()) {
      result.push_back(process.get());
    }
  }
  return result;
}

////////////////////////////////////////////////////////////////////////

inline Option<Process> process(
    pid_t pid,
    const std::list<Process>& processes) {
  foreach (const Process& process, processes) {
    if (process.pid == pid) {
      return process;
    }
  }
  return None();
}

////////////////////////////////////////////////////////////////////////

inline std::set<pid_t> children(
    pid_t pid,
    const std::list<Process>& processes,
    bool recursive = true) {
  // Perform a breadth first search for descendants.
  std::set<pid_t> descendants;
  std::queue<pid_t> parents;
  parents.push(pid);

  do {
    pid_t parent = parents.front();
    parents.pop();

    // Search for children of parent.
    foreach (const Process& process, processes) {
      if (process.parent == parent) {
        // Have we seen this child yet?
        if (descendants.insert(process.pid).second) {
          parents.push(process.pid);
        }
      }
    }
  } while (recursive && !parents.empty());

  return descendants;
}

////////////////////////////////////////////////////////////////////////

inline Try<std::set<pid_t>> children(pid_t pid, bool recursive = true) {
  const Try<std::list<Process>> processes = os::processes();

  if (processes.isError()) {
    return Error(processes.error());
  }

  return children(pid, processes.get(), recursive);
}

////////////////////////////////////////////////////////////////////////

} // namespace os

////////////////////////////////////////////////////////////////////////
