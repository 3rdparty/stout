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
#include "stout/stringify.h"
#include "stout/try.h"
#include "stout/windows.h"

////////////////////////////////////////////////////////////////////////

namespace internal {

////////////////////////////////////////////////////////////////////////

namespace windows {

////////////////////////////////////////////////////////////////////////

inline Try<DWORD> get_file_attributes(const std::wstring& path) {
  const DWORD attributes = ::GetFileAttributesW(path.data());
  if (attributes == INVALID_FILE_ATTRIBUTES) {
    return WindowsError(
        "Failed to get attributes for file '" + stringify(path) + "'");
  }
  return attributes;
}

////////////////////////////////////////////////////////////////////////

} // namespace windows

////////////////////////////////////////////////////////////////////////

} // namespace internal

////////////////////////////////////////////////////////////////////////
