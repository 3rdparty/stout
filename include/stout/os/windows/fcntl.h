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

#include <glog/logging.h>

#include "stout/nothing.h"
#include "stout/os/int_fd.h"
#include "stout/os/socket.h"
#include "stout/try.h"
#include "stout/windows.h"

////////////////////////////////////////////////////////////////////////

namespace os {

////////////////////////////////////////////////////////////////////////

inline Try<Nothing> cloexec(const int_fd& fd) {
  return Nothing();
}

////////////////////////////////////////////////////////////////////////

inline Try<Nothing> unsetCloexec(const int_fd& fd) {
  return Nothing();
}

////////////////////////////////////////////////////////////////////////

inline Try<bool> isCloexec(const int_fd& fd) {
  return true;
}

////////////////////////////////////////////////////////////////////////

inline Try<Nothing> nonblock(const int_fd& fd) {
  switch (fd.type()) {
    case WindowsFD::Type::HANDLE: {
      /* Do nothing. */
      return Nothing();
    }
    case WindowsFD::Type::SOCKET: {
      const u_long non_block_mode = 1;
      u_long mode = non_block_mode;

      int result = ::ioctlsocket(fd, FIONBIO, &mode);
      if (result != NO_ERROR) {
        return WindowsSocketError();
      }
      return Nothing();
    }
  }

  UNREACHABLE();
}

////////////////////////////////////////////////////////////////////////

// NOTE: This is not supported on Windows.
inline Try<bool> isNonblock(const int_fd& fd) {
  VLOG(2) << "`os::isNonblock` has been called, but is a stub on Windows";
  return true;
}

////////////////////////////////////////////////////////////////////////

} // namespace os

////////////////////////////////////////////////////////////////////////
