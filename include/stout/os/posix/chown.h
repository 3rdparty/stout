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

#include <fts.h>
#include <pwd.h>
#include <sys/types.h>

#include "stout/error.h"
#include "stout/nothing.h"
#include "stout/os/shell.h"
#include "stout/os/stat.h"
#include "stout/try.h"

////////////////////////////////////////////////////////////////////////

namespace os {

////////////////////////////////////////////////////////////////////////

// Set the ownership for a path. This function never follows any symlinks.
inline Try<Nothing> chown(
    uid_t uid,
    gid_t gid,
    const std::string& path,
    bool recursive) {
  char* path_[] = {const_cast<char*>(path.c_str()), nullptr};

  FTS* tree = ::fts_open(
      path_,
      FTS_NOCHDIR | FTS_PHYSICAL,
      nullptr);

  if (tree == nullptr) {
    return ErrnoError();
  }

  FTSENT* node;
  while ((node = ::fts_read(tree)) != nullptr) {
    switch (node->fts_info) {
      // Preorder directory.
      case FTS_D:
      // Regular file.
      case FTS_F:
      // Symbolic link.
      case FTS_SL:
      // Symbolic link without target.
      case FTS_SLNONE: {
        if (::lchown(node->fts_path, uid, gid) < 0) {
          Error error = ErrnoError();
          ::fts_close(tree);
          return error;
        }

        break;
      }

      // Unreadable directory.
      case FTS_DNR:
      // Error; errno is set.
      case FTS_ERR:
      // Directory that causes cycles.
      case FTS_DC:
      // `stat(2)` failed.
      case FTS_NS: {
        Error error = ErrnoError();
        ::fts_close(tree);
        return error;
      }

      default:
        break;
    }

    if (node->fts_level == FTS_ROOTLEVEL && !recursive) {
      break;
    }
  }

  ::fts_close(tree);
  return Nothing();
}

////////////////////////////////////////////////////////////////////////

// Changes the specified path's user and group ownership to that of
// the specified user.
inline Try<Nothing> chown(
    const std::string& user,
    const std::string& path,
    bool recursive = true) {
  passwd* passwd;

  errno = 0;

  if ((passwd = ::getpwnam(user.c_str())) == nullptr) {
    return errno
        ? ErrnoError("Failed to get user information for '" + user + "'")
        : Error("No such user '" + user + "'");
  }

  return chown(passwd->pw_uid, passwd->pw_gid, path, recursive);
}

////////////////////////////////////////////////////////////////////////

} // namespace os

////////////////////////////////////////////////////////////////////////
