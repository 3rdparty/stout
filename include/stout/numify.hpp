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

#ifndef __STOUT_NUMIFY_HPP__
#define __STOUT_NUMIFY_HPP__

#include <sstream>
#include <string>

#include <boost/lexical_cast.hpp>

#include "error.hpp"
#include "none.hpp"
#include "option.hpp"
#include "result.hpp"
#include "strings.hpp"
#include "try.hpp"

template <typename T>
Try<T> numify(const std::string& s)
{
  // Since even with a `0x` prefix `boost::lexical_cast` cannot always
  // cast all hexadecimal numbers on all platforms (parsing of some
  // floating point literals seems to work e.g., on macOS, but fails on
  // some Linuxes), we have to workaround this issue here. We also
  // process negative hexadecimal numbers `-0x` here to keep it
  // consistent with non-hexadecimal numbers.
  bool maybeHex = false;

  if (strings::startsWith(s, "0x") || strings::startsWith(s, "0X") ||
      strings::startsWith(s, "-0x") || strings::startsWith(s, "-0X")) {
    maybeHex = true;

    // We disallow hexadecimal floating point numbers.
    if (strings::contains(s, ".") || strings::contains(s, "p")) {
      return Error("Failed to convert '" + s + "' to number");
    }
  }

  try {
    return boost::lexical_cast<T>(s);
  } catch (const boost::bad_lexical_cast&) {
    // Try to parse hexadecimal numbers by hand if `boost::lexical_cast` failed.
    if (maybeHex) {
      T result;
      std::stringstream ss;
      // Process negative hexadecimal numbers.
      if (strings::startsWith(s, "-")) {
        ss << std::hex << s.substr(1);
        ss >> result;
        // NOTE: Negating `result` is safe even if `T` is an unsigned
        // integer type, because the C++ standard defines the result
        // to be modulo 2^n in that case.
        //
        //     numify<T>("-1") == std::numeric_limits<T>::max();
        //
        // Disabled unary negation warning for all types.
#ifdef __WINDOWS__
#pragma warning(disable:4146)
#endif
        result = -result;
#ifdef __WINDOWS__
#pragma warning(default:4146)
#endif
      } else {
        ss << std::hex << s;
        ss >> result;
      }
      // Make sure we really hit the end of the string.
      if (!ss.fail() && ss.eof()) {
        return result;
      }
    }

    return Error("Failed to convert '" + s + "' to number");
  }
}


template <typename T>
Try<T> numify(const char* s)
{
  return numify<T>(std::string(s));
}


template <typename T>
Result<T> numify(const Option<std::string>& s)
{
  if (s.isSome()) {
    Try<T> t = numify<T>(s.get());
    if (t.isSome()) {
      return t.get();
    } else if (t.isError()) {
      return Error(t.error());
    }
  }

  return None();
}

#endif // __STOUT_NUMIFY_HPP__
