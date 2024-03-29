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

#include "stout/strings.h"

////////////////////////////////////////////////////////////////////////

namespace uri {

////////////////////////////////////////////////////////////////////////

const std::string FILE_PREFIX = "file://";

////////////////////////////////////////////////////////////////////////

// Returns a valid URI containing a filename.
//
// On Windows, the / character is replaced with \ since that's the path
// separator. Note that / will often work, but will absolutely not work if the
// path is a long path.
inline std::string from_path(const std::string& filepath) {
#ifdef _WIN32
  return FILE_PREFIX + strings::replace(filepath, "\\", "/");
#else
  return FILE_PREFIX + filepath;
#endif // _WIN32
}

////////////////////////////////////////////////////////////////////////

} // namespace uri

////////////////////////////////////////////////////////////////////////
