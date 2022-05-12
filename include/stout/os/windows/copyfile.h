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

#include "stout/error.h"
#include "stout/internal/windows/longpath.h"
#include "stout/nothing.h"
#include "stout/path.h"
#include "stout/try.h"
#include "stout/windows.h"

////////////////////////////////////////////////////////////////////////

namespace os {

////////////////////////////////////////////////////////////////////////

// Uses the `CopyFile` Windows API to perform a file copy.
// Unlike the POSIX implementation, we do not need to check if the
// source or destination are directories, because `CopyFile` only
// works on files.
//
// https://tinyurl.com/mrxcxzeb
//
// NOLINT(whitespace/line_length)
inline Try<Nothing> copyfile(
    const std::string& source,
    const std::string& destination) {
  if (!path::absolute(source)) {
    return Error("`source` was a relative path");
  }

  if (!path::absolute(destination)) {
    return Error("`destination` was a relative path");
  }

  if (!::CopyFileW(
          ::internal::windows::longpath(source).data(),
          ::internal::windows::longpath(destination).data(),
          // NOTE: This allows the destination to be overwritten if the
          // destination already exists, as is the case in the POSIX version of
          // `copyfile`.
          false)) {
    return WindowsError();
  }

  return Nothing();
}

////////////////////////////////////////////////////////////////////////

} // namespace os

////////////////////////////////////////////////////////////////////////
