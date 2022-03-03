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
// NOTE: The order of these `#include`s is important. This file is structured
// as a series of `#include`s for historical reasons. Before, `stout/error`
// simply contained the definitions of `Error` and `ErrnoError`. The addition
// of Windows required the addition of `WindowsError`. But, we try to avoid
// `#ifdef`'ing code, opting instead to `#ifdef` `#include` statements. Hence,
// we simply move the `error.hpp` code to `errorbase.hpp` and include the
// Windows error code below it.
#include "stout/errorbase.hpp"

#ifdef _WIN32
#include "stout/windows/error.hpp"
#endif // _WIN32

////////////////////////////////////////////////////////////////////////

using SocketError =
#ifdef _WIN32
    WindowsSocketError;
#else
    ErrnoError;
#endif // _WIN32

////////////////////////////////////////////////////////////////////////
