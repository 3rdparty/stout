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

#ifndef __STOUT_PATH_HPP__
#define __STOUT_PATH_HPP__

#include <string>
#include <utility>
#include <vector>

#include <stout/stringify.hpp>
#include <stout/strings.hpp>

#include <stout/os/constants.hpp>


namespace path {

// Converts a fully formed URI to a filename for the platform.
//
// On all platforms, the optional "file://" prefix is removed if it
// exists.
//
// On Windows, this also converts "/" characters to "\" characters.
// The Windows file system APIs don't work with "/" in the filename
// when using long paths (although they do work fine if the file
// path happens to be short).
//
// NOTE: Currently, Mesos uses URIs and files somewhat interchangably.
// For compatibility, lack of "file://" prefix is not considered an
// error.
inline std::string from_uri(const std::string& uri)
{
  // Remove the optional "file://" if it exists.
  // TODO(coffler): Remove the `hostname` component.
  const std::string path = strings::remove(uri, "file://", strings::PREFIX);

#ifndef __WINDOWS__
  return path;
#else
  return strings::replace(path, "/", "\\");
#endif // __WINDOWS__
}


// Normalizes a given pathname and removes redundant separators and up-level
// references.
//
// Pathnames like `A/B/`, `A///B`, `A/./B`, 'A/foobar/../B` are all normalized
// to `A/B`. An empty pathname is normalized to `.`. Up-level entry also cannot
// escape a root path, in which case an error will be returned.
//
// This function follows the rules described in path_resolution(7) for Linux.
// However, it only performs pure lexical processing without touching the
// actual filesystem.
inline Try<std::string> normalize(
    const std::string& path,
    const char _separator = os::PATH_SEPARATOR)
{
  if (path.empty()) {
    return ".";
  }

  std::vector<std::string> components;
  const bool isAbs = (path[0] == _separator);
  const std::string separator(1, _separator);

  // TODO(jasonlai): Handle pathnames (including absolute paths) in Windows.

  foreach (const std::string& component, strings::tokenize(path, separator)) {
    // Skips empty components and "." (current directory).
    if (component == "." || component.empty()) {
      continue;
    }

    if (component == "..") {
      if (components.empty()) {
        if (isAbs) {
          return Error("Absolute path '" + path + "' tries to escape root");
        }
        components.push_back(component);
      } else if (components.back() == "..") {
        components.push_back(component);
      } else {
        components.pop_back();
      }
    } else {
      components.push_back(component);
    }
  }

  if (components.empty()) {
    return isAbs ? separator : ".";
  } else if (isAbs) {
    // Make sure that a separator is prepended if it is an absolute path.
    components.insert(components.begin(), "");
  }

  return strings::join(separator, components);
}


// Base case.
inline std::string join(
    const std::string& path1,
    const std::string& path2,
    const char _separator = os::PATH_SEPARATOR)
{
  const std::string separator = stringify(_separator);
  return strings::remove(path1, separator, strings::SUFFIX) +
         separator +
         strings::remove(path2, separator, strings::PREFIX);
}


template <typename... Paths>
inline std::string join(
    const std::string& path1,
    const std::string& path2,
    Paths&&... paths)
{
  return join(path1, join(path2, std::forward<Paths>(paths)...));
}


inline std::string join(const std::vector<std::string>& paths)
{
  if (paths.empty()) {
    return "";
  }

  std::string result = paths[0];
  for (size_t i = 1; i < paths.size(); ++i) {
    result = join(result, paths[i]);
  }
  return result;
}


/**
 * Returns whether the given path is an absolute path.
 * If an invalid path is given, the return result is also invalid.
 */
inline bool absolute(const std::string& path)
{
#ifndef __WINDOWS__
  return strings::startsWith(path, os::PATH_SEPARATOR);
#else
  // NOTE: We do not use `PathIsRelative` Windows utility function
  // here because it does not support long paths.
  //
  // See https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx
  // for details on paths. In short, an absolute path for files on Windows
  // looks like one of the following:
  //   * "[A-Za-z]:\"
  //   * "[A-Za-z]:/"
  //   * "\\?\..."
  //   * "\\server\..." where "server" is a network host.
  //
  // NOLINT(whitespace/line_length)

  // A uniform naming convention (UNC) name of any format,
  // always starts with two backslash characters.
  if (strings::startsWith(path, "\\\\")) {
    return true;
  }

  // A disk designator with a slash, for example "C:\" or "d:/".
  if (path.length() < 3) {
    return false;
  }

  const char letter = path[0];
  if (!((letter >= 'A' && letter <= 'Z') ||
        (letter >= 'a' && letter <= 'z'))) {
    return false;
  }

  std::string colon = path.substr(1, 2);
  return colon == ":\\" || colon == ":/";
#endif // __WINDOWS__
}

} // namespace path {


/**
 * Represents a POSIX or Windows file system path and offers common path
 * manipulations. When reading the comments below, keep in mind that '/' refers
 * to the path separator character, so read it as "'/' or '\', depending on
 * platform".
 */
class Path
{
public:
  Path() : value(), separator(os::PATH_SEPARATOR) {}

  explicit Path(
      const std::string& path, const char path_separator = os::PATH_SEPARATOR)
    : value(strings::remove(path, "file://", strings::PREFIX)),
      separator(path_separator)
  {}

  // TODO(cmaloney): Add more useful operations such as 'directoryname()',
  // 'filename()', etc.

  /**
   * Extracts the component following the final '/'. Trailing '/'
   * characters are not counted as part of the pathname.
   *
   * Like the standard '::basename()' except it is thread safe.
   *
   * The following list of examples (taken from SUSv2) shows the
   * strings returned by basename() for different paths:
   *
   * path        | basename
   * ----------- | -----------
   * "/usr/lib"  | "lib"
   * "/usr/"     | "usr"
   * "usr"       | "usr"
   * "/"         | "/"
   * "."         | "."
   * ".."        | ".."
   *
   * @return The component following the final '/'. If Path does not
   *   contain a '/', this returns a copy of Path. If Path is the
   *   string "/", then this returns the string "/". If Path is an
   *   empty string, then it returns the string ".".
   */
  inline std::string basename() const
  {
    if (value.empty()) {
      return std::string(".");
    }

    size_t end = value.size() - 1;

    // Remove trailing slashes.
    if (value[end] == separator) {
      end = value.find_last_not_of(separator, end);

      // Paths containing only slashes result into "/".
      if (end == std::string::npos) {
        return stringify(separator);
      }
    }

    // 'start' should point towards the character after the last slash
    // that is non trailing.
    size_t start = value.find_last_of(separator, end);

    if (start == std::string::npos) {
      start = 0;
    } else {
      start++;
    }

    return value.substr(start, end + 1 - start);
  }

  // TODO(hausdorff) Make sure this works on Windows for very short path names,
  // such as "C:\Temp". There is a distinction between "C:" and "C:\", the
  // former means "current directory of the C drive", while the latter means
  // "The root of the C drive". Also make sure that UNC paths are handled.
  // Will probably need to use the Windows path functions for that.
  /**
   * Extracts the component up to, but not including, the final '/'.
   * Trailing '/' characters are not counted as part of the pathname.
   *
   * Like the standard '::dirname()' except it is thread safe.
   *
   * The following list of examples (taken from SUSv2) shows the
   * strings returned by dirname() for different paths:
   *
   * path        | dirname
   * ----------- | -----------
   * "/usr/lib"  | "/usr"
   * "/usr/"     | "/"
   * "usr"       | "."
   * "/"         | "/"
   * "."         | "."
   * ".."        | "."
   *
   * @return The component up to, but not including, the final '/'. If
   *   Path does not contain a '/', then this returns the string ".".
   *   If Path is the string "/", then this returns the string "/".
   *   If Path is an empty string, then this returns the string ".".
   */
  inline std::string dirname() const
  {
    if (value.empty()) {
      return std::string(".");
    }

    size_t end = value.size() - 1;

    // Remove trailing slashes.
    if (value[end] == separator) {
      end = value.find_last_not_of(separator, end);
    }

    // Remove anything trailing the last slash.
    end = value.find_last_of(separator, end);

    // Paths containing no slashes result in ".".
    if (end == std::string::npos) {
      return std::string(".");
    }

    // Paths containing only slashes result in "/".
    if (end == 0) {
      return stringify(separator);
    }

    // 'end' should point towards the last non slash character
    // preceding the last slash.
    end = value.find_last_not_of(separator, end);

    // Paths containing no non slash characters result in "/".
    if (end == std::string::npos) {
      return stringify(separator);
    }

    return value.substr(0, end + 1);
  }

  /**
   * Returns the file extension of the path, including the dot.
   *
   * Returns None if the basename contains no dots, or consists
   * entirely of dots (i.e. '.', '..').
   *
   * Examples:
   *
   *   path         | extension
   *   ----------   | -----------
   *   "a.txt"      |  ".txt"
   *   "a.tar.gz"   |  ".gz"
   *   ".bashrc"    |  ".bashrc"
   *   "a"          |  None
   *   "."          |  None
   *   ".."         |  None
   */
  inline Option<std::string> extension() const
  {
    std::string _basename = basename();
    size_t index = _basename.rfind('.');

    if (_basename == "." || _basename == ".." || index == std::string::npos) {
      return None();
    }

    return _basename.substr(index);
  }

  // Checks whether the path is absolute.
  inline bool absolute() const
  {
    return path::absolute(value);
  }

  // Implicit conversion from Path to string.
  operator std::string() const
  {
    return value;
  }

  const std::string& string() const
  {
    return value;
  }

private:
  std::string value;
  char separator;
};


inline bool operator==(const Path& left, const Path& right)
{
  return left.string() == right.string();
}


inline bool operator!=(const Path& left, const Path& right)
{
  return !(left == right);
}


inline bool operator<(const Path& left, const Path& right)
{
  return left.string() < right.string();
}


inline bool operator>(const Path& left, const Path& right)
{
  return right < left;
}


inline bool operator<=(const Path& left, const Path& right)
{
  return !(left > right);
}


inline bool operator>=(const Path& left, const Path& right)
{
  return !(left < right);
}


inline std::ostream& operator<<(
    std::ostream& stream,
    const Path& path)
{
  return stream << path.string();
}

#endif // __STOUT_PATH_HPP__
