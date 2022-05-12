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

#include "stout/nothing.h"
#include "stout/os/close.h"
#include "stout/os/exists.h"
#include "stout/os/int_fd.h"
#include "stout/os/open.h"
#include "stout/os/utime.h"
#include "stout/try.h"

#ifdef _WIN32
#include "stout/windows.h"
#endif // _WIN32

////////////////////////////////////////////////////////////////////////

namespace os {

////////////////////////////////////////////////////////////////////////

inline Try<Nothing> touch(const std::string& path) {
  if (!os::exists(path)) {
    Try<int_fd> fd = os::open(
        path,
        O_RDWR | O_CREAT,
        S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    if (fd.isError()) {
      return Error("Failed to open file: " + fd.error());
    }

    return os::close(fd.get());
  }

  // Update the access and modification times.
  return os::utime(path);
}

////////////////////////////////////////////////////////////////////////

} // namespace os

////////////////////////////////////////////////////////////////////////
