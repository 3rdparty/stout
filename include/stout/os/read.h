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

#include <assert.h>
#include <stdio.h>
#ifndef _WIN32
#include <unistd.h>
#endif // _WIN32

#include <string>

#if defined(__sun) || defined(_WIN32)
#include <fstream>
#endif // __sun || _WIN32

#include "stout/error.h"
#include "stout/result.h"
#include "stout/try.h"
#ifdef _WIN32
#include "stout/windows.h"
#endif // _WIN32

#include "stout/os/int_fd.h"
#include "stout/os/socket.h"

#ifdef _WIN32
#include "stout/internal/windows/longpath.h"
#endif // _WIN32

#ifdef _WIN32
#include "stout/os/windows/read.h"
#else
#include "stout/os/posix/read.h"
#endif // _WIN32

////////////////////////////////////////////////////////////////////////

namespace os {

////////////////////////////////////////////////////////////////////////

// Reads 'size' bytes from a file from its current offset.
// If EOF is encountered before reading 'size' bytes then the result
// will contain the bytes read and a subsequent read will return None.
inline Result<std::string> read(int_fd fd, size_t size) {
  char* buffer = new char[size];
  size_t offset = 0;

  while (offset < size) {
    ssize_t length = os::read(fd, buffer + offset, size - offset);

#ifdef _WIN32
    // NOTE: There is no actual difference between `WSAGetLastError` and
    // `GetLastError`, the former is an alias for the latter. As such, there is
    // no difference between `WindowsError` and `WindowsSocketError`, so we can
    // simply use the former here for both `HANDLE` and `SOCKET` types of
    // `int_fd`. See MESOS-8764.
    WindowsError error;
#else
    ErrnoError error;
#endif // _WIN32

    if (length < 0) {
      // TODO(bmahler): Handle a non-blocking fd? (EAGAIN, EWOULDBLOCK)
      if (net::is_restartable_error(error.code)) {
        continue;
      }
      delete[] buffer;
      return error;
    } else if (length == 0) {
      // Reached EOF before expected! Only return as much data as
      // available or None if we haven't read anything yet.
      if (offset > 0) {
        std::string result(buffer, offset);
        delete[] buffer;
        return result;
      }
      delete[] buffer;
      return None();
    }

    offset += length;
  }

  std::string result(buffer, size);
  delete[] buffer;
  return result;
}

////////////////////////////////////////////////////////////////////////

// Returns the contents of the file.
// NOTE: getline is not available on Solaris so we use STL.
#if defined(__sun)
inline Try<std::string> read(const std::string& path) {
  std::ifstream file(path.c_str());
  if (!file.is_open()) {
    // Does ifstream actually set errno?
    return ErrnoError("Failed to open file");
  }
  return std::string(
      (std::istreambuf_iterator<char>(file)),
      (std::istreambuf_iterator<char>()));
}

////////////////////////////////////////////////////////////////////////

// NOTE: Windows needs Unicode long path support.
#elif defined(_WIN32)
inline Try<std::string> read(const std::string& path) {
  const std::wstring longpath = ::internal::windows::longpath(path);
  // NOTE: The `wchar_t` constructor of `ifstream` is an MSVC
  // extension.
  //
  // TODO(andschwa): This might need `io_base::binary` like other
  // streams on Windows.
  std::ifstream file(longpath.data());
  if (!file.is_open()) {
    return Error("Failed to open file");
  }

  return std::string(
      (std::istreambuf_iterator<char>(file)),
      (std::istreambuf_iterator<char>()));
}

////////////////////////////////////////////////////////////////////////

#else
inline Try<std::string> read(const std::string& path) {
  FILE* file = ::fopen(path.c_str(), "r");
  if (file == nullptr) {
    return ErrnoError();
  }

  // Use a buffer to read the file in BUFSIZ
  // chunks and append it to the string we return.
  //
  // NOTE: We aren't able to use fseek() / ftell() here
  // to find the file size because these functions don't
  // work properly for in-memory files like /proc/*/stat.
  char* buffer = new char[BUFSIZ];
  std::string result;

  while (true) {
    size_t read = ::fread(buffer, 1, BUFSIZ, file);

    if (::ferror(file)) {
      // NOTE: ferror() will not modify errno if the stream
      // is valid, which is the case here since it is open.
      ErrnoError error;
      delete[] buffer;
      ::fclose(file);
      return error;
    }

    result.append(buffer, read);

    if (read != BUFSIZ) {
      assert(feof(file));
      break;
    }
  };

  ::fclose(file);
  delete[] buffer;
  return result;
}
#endif // __sun || _WIN32

////////////////////////////////////////////////////////////////////////

} // namespace os

////////////////////////////////////////////////////////////////////////
