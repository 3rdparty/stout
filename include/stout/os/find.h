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

#include <list>
#include <string>

#include "stout/error.h"
#include "stout/foreach.h"
#include "stout/os/ls.h"
#include "stout/os/stat.h"
#include "stout/path.h"
#include "stout/try.h"

////////////////////////////////////////////////////////////////////////

namespace os {

////////////////////////////////////////////////////////////////////////

// Return the list of file paths that match the given pattern by recursively
// searching the given directory. A match is successful if the pattern is a
// substring of the file name.
// NOTE: Directory path should not end with '/'.
// NOTE: Symbolic links are not followed.
// TODO(vinod): Support regular expressions for pattern.
// TODO(vinod): Consider using ftw or a non-recursive approach.
inline Try<std::list<std::string>> find(
    const std::string& directory,
    const std::string& pattern) {
  std::list<std::string> results;

  if (!stat::isdir(directory)) {
    return Error("'" + directory + "' is not a directory");
  }

  Try<std::list<std::string>> entries = ls(directory);
  if (entries.isSome()) {
    foreach (const std::string& entry, entries.get()) {
      std::string path = path::join(directory, entry);
      // If it's a directory, recurse.
      if (stat::isdir(path) && !stat::islink(path)) {
        Try<std::list<std::string>> matches = find(path, pattern);
        if (matches.isError()) {
          return matches;
        }
        foreach (const std::string& match, matches.get()) {
          results.push_back(match);
        }
      } else {
        if (entry.find(pattern) != std::string::npos) {
          results.push_back(path); // Matched the file pattern!
        }
      }
    }
  }

  return results;
}

////////////////////////////////////////////////////////////////////////

} // namespace os

////////////////////////////////////////////////////////////////////////
