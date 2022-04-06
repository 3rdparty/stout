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

#include <stdlib.h>
#include <string.h>

#include <string>

#include "stout/error.hpp"
#include "stout/os/close.hpp"
#include "stout/os/int_fd.hpp"
#include "stout/os/temp.hpp"
#include "stout/path.hpp"
#include "stout/try.hpp"

////////////////////////////////////////////////////////////////////////

namespace os {

////////////////////////////////////////////////////////////////////////

// Creates a temporary file using the specified path template. The
// template may be any path with _6_ `Xs' appended to it, for example
// /tmp/temp.XXXXXX. The trailing `Xs' are replaced with a unique
// alphanumeric combination.
inline Try<std::string> mktemp(
    const std::string& path = path::join(os::temp(), "XXXXXX")) {
  char* _path = new char[path.size() + 1];
  ::memcpy(_path, path.c_str(), path.size() + 1);

  int_fd fd = ::mkstemp(_path);
  std::string temp(_path);
  delete[] _path;

  if (fd < 0) {
    return ErrnoError();
  }

  Try<Nothing> close = os::close(fd);

  // We propagate `close` failures if creation of the temp file was successful.
  if (close.isError()) {
    return Error("Failed to close '" + stringify(fd) + "':" + close.error());
  }

  return temp;
}

////////////////////////////////////////////////////////////////////////

} // namespace os

////////////////////////////////////////////////////////////////////////
