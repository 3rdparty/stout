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

// For readability, we minimize the number of #ifdef blocks in the code by
// splitting platform specific system calls into separate directories.
#ifdef _WIN32
#include "stout/os/windows/getcwd.hpp"
#else
#include "stout/os/posix/getcwd.hpp"
#endif // _WIN32
