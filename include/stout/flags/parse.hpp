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

#ifndef __STOUT_FLAGS_PARSE_HPP__
#define __STOUT_FLAGS_PARSE_HPP__

#include <sstream> // For istringstream.
#include <string>

#include <stout/bytes.hpp>
#include <stout/duration.hpp>
#include <stout/error.hpp>
#include <stout/ip.hpp>
#include <stout/json.hpp>
#include <stout/path.hpp>
#include <stout/strings.hpp>
#include <stout/try.hpp>

#include <stout/flags/flag.hpp>

#include <stout/os/read.hpp>

namespace flags {

template <typename T>
Try<T> parse(const std::string& value)
{
  T t;
  std::istringstream in(value);
  in >> t;

  if (in && in.eof()) {
    return t;
  }

  return Error("Failed to convert into required type");
}


template <>
inline Try<std::string> parse(const std::string& value)
{
  return value;
}


template <>
inline Try<bool> parse(const std::string& value)
{
  if (value == "true" || value == "1") {
    return true;
  } else if (value == "false" || value == "0") {
    return false;
  }
  return Error("Expecting a boolean (e.g., true or false)");
}


template <>
inline Try<Duration> parse(const std::string& value)
{
  return Duration::parse(value);
}


template <>
inline Try<Bytes> parse(const std::string& value)
{
  return Bytes::parse(value);
}


template <>
inline Try<net::IP> parse(const std::string& value)
{
  return net::IP::parse(value);
}


template <>
inline Try<net::IPv4> parse(const std::string& value)
{
  return net::IPv4::parse(value);
}


template <>
inline Try<net::IPv6> parse(const std::string& value)
{
  return net::IPv6::parse(value);
}


template <>
inline Try<JSON::Object> parse(const std::string& value)
{
#ifndef __WINDOWS__
  // A value that already starts with 'file://' will properly be
  // loaded from the file and put into 'value' but if it starts with
  // '/' we need to explicitly handle it for backwards compatibility
  // reasons (because we used to handle it before we introduced the
  // 'fetch' mechanism for flags that first fetches the data from URIs
  // such as 'file://').
  //
  // NOTE: Because this code is deprecated, it is not supported on Windows.
  if (strings::startsWith(value, "/")) {
    LOG(WARNING) << "Specifying an absolute filename to read a command line "
                    "option out of without using 'file:// is deprecated and "
                    "will be removed in a future release. Simply adding "
                    "'file://' to the beginning of the path should eliminate "
                    "this warning.";

    Try<std::string> read = os::read(value);
    if (read.isError()) {
      return Error("Error reading file '" + value + "': " + read.error());
    }
    return JSON::parse<JSON::Object>(read.get());
  }
#endif // __WINDOWS__
  return JSON::parse<JSON::Object>(value);
}


template <>
inline Try<JSON::Array> parse(const std::string& value)
{
#ifndef __WINDOWS__
  // A value that already starts with 'file://' will properly be
  // loaded from the file and put into 'value' but if it starts with
  // '/' we need to explicitly handle it for backwards compatibility
  // reasons (because we used to handle it before we introduced the
  // 'fetch' mechanism for flags that first fetches the data from URIs
  // such as 'file://').
  //
  // NOTE: Because this code is deprecated, it is not supported on Windows.
  if (strings::startsWith(value, "/")) {
    LOG(WARNING) << "Specifying an absolute filename to read a command line "
                    "option out of without using 'file:// is deprecated and "
                    "will be removed in a future release. Simply adding "
                    "'file://' to the beginning of the path should eliminate "
                    "this warning.";

    Try<std::string> read = os::read(value);
    if (read.isError()) {
      return Error("Error reading file '" + value + "': " + read.error());
    }
    return JSON::parse<JSON::Array>(read.get());
  }
#endif // __WINDOWS__
  return JSON::parse<JSON::Array>(value);
}


template <>
inline Try<Path> parse(const std::string& value)
{
  return Path(value);
}


template <>
inline Try<SecurePathOrValue> parse(const std::string& value)
{
  SecurePathOrValue result;
  result.value = value;

  if (strings::startsWith(value, "file://")) {
    const std::string path = value.substr(7);

    Try<std::string> read = os::read(path);

    if (read.isError()) {
      return Error("Error reading file '" + path + "': " + read.error());
    }

    result.value = read.get();
    result.path = Path(path);
  }

  return result;
}


#ifdef __WINDOWS__
template <>
inline Try<int_fd> parse(const std::string& value)
{
  // Looks like "WindowsFD::Type::HANDLE=0000000000000000".
  std::vector<std::string> fd = strings::split(value, "=");
  if (fd.size() != 2) {
    return Error("Expected to split string into exactly two parts.");
  }

  if (strings::endsWith(fd[0], "HANDLE")) {
    Try<HANDLE> t = parse<HANDLE>(fd[1]);
    if (t.isError()) {
      return Error(t.error());
    }
    return int_fd(t.get());
  } else if (strings::endsWith(fd[0], "SOCKET")) {
    Try<SOCKET> t = parse<SOCKET>(fd[1]);
    if (t.isError()) {
      return Error(t.error());
    }
    return int_fd(t.get());
  }

  return Error("`int_fd` was neither a `HANDLE` nor a `SOCKET`");
}
#endif // __WINDOWS__


// TODO(klueska): Generalize this parser to take any comma separated
// list and convert it to its appropriate type (i.e., not just for
// unsigned ints). Issues could arise when the generic type is a
// string that contains commas though, so generalizing this is not as
// straightforward as it looks at first glance.
template <>
inline Try<std::vector<unsigned int>> parse(const std::string& value)
{
  std::vector<unsigned int> result;

  foreach (const std::string& token, strings::tokenize(value, ",")) {
    Try<unsigned int> number = numify<unsigned int>(token);

    if (number.isError()) {
      return Error("Failed to numify '" + token + "': " + number.error());
    }

    result.push_back(number.get());
  }

  return result;
}


// NOTE: Strings in the set cannot contain commas, since that
// is the delimiter and we provide no way to escape it.
//
// TODO(klueska): Generalize this parser to take any comma separated
// list and convert it to its appropriate type (i.e., not just for
// strings).
template <>
inline Try<std::set<std::string>> parse(const std::string& value)
{
  std::set<std::string> result;

  foreach (const std::string& token, strings::tokenize(value, ",")) {
    if (result.count(token) > 0) {
      return Error("Duplicate token '" + token + "'");
    }

    result.insert(token);
  }

  return result;
}

} // namespace flags {

#endif // __STOUT_FLAGS_PARSE_HPP__
