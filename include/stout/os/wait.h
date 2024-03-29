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

#ifdef _WIN32
// TODO(klueska): Move all `WAIT` related functions out of
// `windows.hpp` and into this file.
#include "stout/windows.h"
#else
#include <sys/wait.h>
#endif // _WIN32

////////////////////////////////////////////////////////////////////////

#ifndef W_EXITCODE
#define W_EXITCODE(ret, sig) ((ret) << 8 | (sig))
#endif

////////////////////////////////////////////////////////////////////////

namespace os {

////////////////////////////////////////////////////////////////////////

// TODO(klueska): Add helper functions for common wait related
// operations in this header file.

} // namespace os

////////////////////////////////////////////////////////////////////////
