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

#include <io.h>

#include "stout/error.hpp"
#include "stout/nothing.hpp"
#include "stout/os/int_fd.hpp"
#include "stout/try.hpp"
#include "stout/windows.hpp"

////////////////////////////////////////////////////////////////////////

namespace os {

////////////////////////////////////////////////////////////////////////

inline Try<Nothing> fsync(const int_fd& fd) {
  if (!::FlushFileBuffers(fd)) {
    return WindowsError(
        "os::fsync: Could not flush file buffers for given file descriptor");
  }

  return Nothing();
}

////////////////////////////////////////////////////////////////////////

} // namespace os

////////////////////////////////////////////////////////////////////////
