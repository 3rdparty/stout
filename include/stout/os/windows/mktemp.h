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

#include <string>
#include <vector>

#include "stout/error.h"
#include "stout/internal/windows/longpath.h"
#include "stout/os/close.h"
#include "stout/os/int_fd.h"
#include "stout/os/open.h"
#include "stout/os/temp.h"
#include "stout/path.h"
// Since 'fmt' library doesn't support 'wide' conversion (e.g
// from 'std::wstring' to 'std::string' and vice versa) we use
// API from 'include/stout/stringify.h' (e.g:
// std::string stringify(const std::wstring& wstr) - function).
// Check the issue for fmt conversion on github:
// https://github.com/fmtlib/fmt/issues/1116
#include "stout/stringify.h"
#include "stout/try.h"
#include "stout/windows.h"

////////////////////////////////////////////////////////////////////////

namespace os {

////////////////////////////////////////////////////////////////////////

// Creates a temporary file using the specified path template. The
// template may be any path with _6_ `Xs' appended to it, for example
// /tmp/temp.XXXXXX. The trailing `Xs' are replaced with a unique
// alphanumeric combination.
inline Try<std::string> mktemp(
    const std::string& path = path::join(os::temp(), "XXXXXX")) {
  const std::wstring longpath = ::internal::windows::longpath(path);
  std::vector<wchar_t> buffer(longpath.begin(), longpath.end());

  // The range does not include the null terminator, needed to reconstruct
  // the next string.
  buffer.push_back(L'\0');

  // NOTE: in the POSIX spec, `mkstemp` will generate a random filename from
  // the `path` template, `open` that filename, and return the resulting file
  // descriptor. On Windows, `_mktemp_s` will actually only generate the path,
  // so here we actually have to call `open` ourselves to get a file descriptor
  // we can return as a result.
  if (::_wmktemp_s(buffer.data(), buffer.size()) != 0) {
    return WindowsError();
  }

  const std::string temp_file = stringify(std::wstring(buffer.data()));

  // NOTE: We open the file with read/write access for the given user, an
  // attempt to match POSIX's specification of `mkstemp`. We use `_S_IREAD` and
  // `_S_IWRITE` here instead of the POSIX equivalents. On Windows the file is
  // is not present, we use `_O_CREAT` option when opening the file.
  Try<int_fd> fd =
      os::open(temp_file, O_RDWR | O_CREAT | O_EXCL, _S_IREAD | _S_IWRITE);
  if (fd.isError()) {
    return Error(fd.error());
  }

  // We ignore the return value of close(). This is because users
  // calling this function are interested in the return value of
  // mkstemp(). Also an unsuccessful close() doesn't affect the file.
  os::close(fd.get());

  return strings::remove(
      temp_file,
      os::LONGPATH_PREFIX,
      strings::Mode::PREFIX);
}

////////////////////////////////////////////////////////////////////////

} // namespace os

////////////////////////////////////////////////////////////////////////
