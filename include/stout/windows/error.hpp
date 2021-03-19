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

#ifndef __STOUT_WINDOWS_ERROR_HPP__
#define __STOUT_WINDOWS_ERROR_HPP__

#include <stout/errorbase.hpp>
#include <stout/windows.hpp>

// A useful type that can be used to represent a Try that has failed. This is a
// lot like `ErrnoError`, except instead of wrapping an error coming from the C
// standard libraries, it wraps an error coming from the Windows APIs.
class WindowsErrorBase : public Error
{
public:
  const DWORD code;

protected:
  explicit WindowsErrorBase(DWORD _code)
    : Error(get_last_error_as_string(_code)), code(_code) {}

  WindowsErrorBase(DWORD _code, const std::string& message)
    : Error(message + ": " + get_last_error_as_string(_code)), code(_code) {}

private:
  static std::string get_last_error_as_string(const DWORD error_code)
  {
    // NOTE: While Windows does have a corresponding message string
    // for an error code of zero, we want the default construction to
    // have as low a cost as possible, so we short circuit here.
    if (error_code == 0) {
      return std::string();
    }

    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS;

    const DWORD default_language = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);

    // This following function `FormatMessage` is a lot like `strerror`, except
    // it pretty-prints errors from the Windows API instead of from the C
    // standard library. Basically, the semantics are: we pass in `error_code`,
    // and it allocates room for a pretty-printed error message at
    // `message_buffer`, and then dumps said pretty-printed error message at
    // that address, in our `default_language`.
    //
    // The parameter `reinterpret_cast<char*>(&message_buffer)` may
    // look strange to readers of this code. It is copied directly out
    // of the documentation[1], and is unfortunately required to get
    // the pretty-printed error message. The short story is:
    //
    //   * The flag `FORMAT_MESSAGE_ALLOCATE_BUFFER` tells `FormatMessage` to
    //     point `message_buffer` (which is an `LPSTR` a.k.a. `char*`) at a
    //     string error message. But, `message_buffer` to point a `char*` at a
    //     different place, `FormatMessage` would need the address
    //     `&message_buffer` so that it could change where it is pointing.
    //   * So, to solve this problem, the API writers decided that when you
    //     pass that flag in, `FormatMessage` will treat the fifth parameter not
    //     as `LPSTR` (which is what the type is in the function signagure),
    //     but as `LPSTR*` a.k.a. `char**`, which (assuming you've casted the
    //     parameter correctly) allows it to allocate the message on your
    //     behalf, and change `message_buffer` to point at this new error
    //     string.
    //   * This is why we need this strange cast that you see below.
    //
    // Finally, and this is important: it is up to the user to free the memory
    // with `LocalFree`! The line below where we do this comes directly from
    // the documentation as well.
    //
    // [1] https://msdn.microsoft.com/en-us/library/windows/desktop/ms679351(v=vs.85).aspx
    char* message_buffer = nullptr;
    // NOTE: This explicitly uses the non-Unicode version of the API
    // so we can avoid needing Unicode conversion logic in the
    // `WindowsErrorBase` class.
    size_t size = ::FormatMessageA(
      flags,
      nullptr,  // Ignored.
      error_code,
      default_language,
      reinterpret_cast<char*>(&message_buffer),
      0,        // Ignored.
      nullptr); // Ignored.

    std::string message(message_buffer, size);

    // Required per documentation above.
    ::LocalFree(message_buffer);

    return message;
  }
};


class WindowsError : public WindowsErrorBase
{
public:
  WindowsError() : WindowsErrorBase(::GetLastError()) {}

  explicit WindowsError(DWORD _code) : WindowsErrorBase(_code) {}

  WindowsError(const std::string& message)
    : WindowsErrorBase(::GetLastError(), message) {}

  WindowsError(DWORD _code, const std::string& message)
    : WindowsErrorBase(_code, message) {}
};


class WindowsSocketError : public WindowsErrorBase
{
public:
  WindowsSocketError() : WindowsErrorBase(::WSAGetLastError()) {}

  explicit WindowsSocketError(DWORD _code) : WindowsErrorBase(_code) {}

  WindowsSocketError(const std::string& message)
    : WindowsErrorBase(::WSAGetLastError(), message) {}

  WindowsSocketError(DWORD _code, const std::string& message)
    : WindowsErrorBase(_code, message) {}
};

#endif // __STOUT_WINDOWS_ERROR_HPP__
