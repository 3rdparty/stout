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

#include <unistd.h>

#include <string>

#include "stout/nothing.h"
#include "stout/os/close.h"
#include "stout/os/int_fd.h"
#include "stout/os/open.h"
#include "stout/try.h"

////////////////////////////////////////////////////////////////////////

namespace os {

////////////////////////////////////////////////////////////////////////

inline Try<Nothing> fsync(int fd) {
  if (::fsync(fd) == -1) {
    return ErrnoError();
  }

  return Nothing();
}

////////////////////////////////////////////////////////////////////////

// A wrapper function for the above `fsync()` with opening and closing the
// file.
// NOTE: This function is POSIX only and can be used to commit changes to a
// directory (e.g., renaming files) to the filesystem.
inline Try<Nothing> fsync(const std::string& path) {
  Try<int_fd> fd = os::open(path, O_RDONLY | O_CLOEXEC);

  if (fd.isError()) {
    return Error(fd.error());
  }

  Try<Nothing> result = fsync(fd.get());

  // We ignore the return value of `close()` since any I/O-related failure
  // scenarios would already have been triggered by `open()` or `fsync()`.
  os::close(fd.get());

  return result;
}

////////////////////////////////////////////////////////////////////////

} // namespace os

////////////////////////////////////////////////////////////////////////
