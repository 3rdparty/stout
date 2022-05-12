// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <sys/types.h>

#include "stout/windows.h"

////////////////////////////////////////////////////////////////////////

// Dummy struct for POSIX compliance.
// The name of the group - `gr_name`.
// Numerical group ID - `gr_gid`.
// Pointer to a null-terminated array of character pointers to
// member names - `gr_mem`.
struct group {
  char* gr_name;
  gid_t gr_gid;
  char** gr_mem;
};

////////////////////////////////////////////////////////////////////////

// Dummy implementation of `getgrgid` for POSIX compliance. Per the POSIX
// specification[1], we are to return `nullptr` if an entry matching the GID is
// not found. On Windows, we will never find such an entry, so we always return
// `nullptr`. Just to be safe, we also set `errno` to `ENOSYS` which indicates
// the function is not implemented.
//
// [1] http://pubs.opengroup.org/onlinepubs/009695399/functions/getgrgid.html
inline struct group* getgrgid(gid_t) {
  errno = ENOSYS;
  return nullptr;
}

////////////////////////////////////////////////////////////////////////
