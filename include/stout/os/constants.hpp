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

////////////////////////////////////////////////////////////////////////

namespace os {

////////////////////////////////////////////////////////////////////////

constexpr char WINDOWS_PATH_SEPARATOR = '\\';
constexpr char POSIX_PATH_SEPARATOR = '/';

#ifndef _WIN32
constexpr char PATH_SEPARATOR = POSIX_PATH_SEPARATOR;
#else
constexpr char PATH_SEPARATOR = WINDOWS_PATH_SEPARATOR;
#endif // _WIN32

#ifndef _WIN32
constexpr char DEV_NULL[] = "/dev/null";
#else
constexpr char DEV_NULL[] = "NUL";
#endif // _WIN32

#ifdef _WIN32
// This prefix is prepended to absolute paths on Windows to indicate the path
// may be greater than 255 characters.
//
// NOTE: We do not use a R"raw string" here because syntax highlighters do not
// handle mismatched backslashes well.
constexpr char LONGPATH_PREFIX[] = "\\\\?\\";
#endif // _WIN32

////////////////////////////////////////////////////////////////////////

} // namespace os

////////////////////////////////////////////////////////////////////////
