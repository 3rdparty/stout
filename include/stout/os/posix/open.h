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

#include <sys/stat.h>
#include <sys/types.h>

#include <string>

#include "stout/error.h"
#include "stout/nothing.h"
#include "stout/os/close.h"
#include "stout/os/fcntl.h"
#include "stout/os/int_fd.h"
#include "stout/try.h"

#ifndef O_CLOEXEC
#error "missing O_CLOEXEC support on this platform"
#endif

////////////////////////////////////////////////////////////////////////

namespace os {

////////////////////////////////////////////////////////////////////////

inline Try<int_fd> open(const std::string& path, int oflag, mode_t mode = 0) {
  int_fd fd = ::open(path.c_str(), oflag, mode);
  if (fd < 0) {
    return ErrnoError();
  }

  return fd;
}

////////////////////////////////////////////////////////////////////////

} // namespace os

////////////////////////////////////////////////////////////////////////
