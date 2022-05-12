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

#include <iostream> // For 'std::cerr' and 'std::endl'.
#include <list>
#include <map>
#include <set>
#include <sstream> // For 'std::ostringstream'.
#include <string>
#include <vector>

#include "abort.h"
#include "error.h"
#include "hashmap.h"
#include "set.h"

////////////////////////////////////////////////////////////////////////

template <typename T>
std::string stringify(const T& t) {
  std::ostringstream out;
  out << t;
  if (!out.good()) {
    ABORT("Failed to stringify!");
  }
  return out.str();
}

////////////////////////////////////////////////////////////////////////

// We provide an explicit overload for strings so we do not incur the overhead
// of a stringstream in generic code (e.g., when stringifying containers of
// strings below).
inline std::string stringify(const std::string& str) {
  return str;
}

////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
// WideCharToMultiByte and MultiByteToWideChar are needed, cause
// std::wbuffer_convert, std::wstring_convert, and the <codecvt>
// header (containing std::codecvt_mode, std::codecvt_utf8,
// std::codecvt_utf16, and std::codecvt_utf8_utf16) are deprecated in
// C++17. The C++ Standard doesn't provide equivalent non-deprecated
// functionality.
static std::string stringify(const std::wstring& wstr) {
  if (wstr.empty()) {
    return std::string{};
  }
  int size_needed = WideCharToMultiByte(
      CP_UTF8,
      0,
      &wstr[0],
      static_cast<int>(wstr.size()),
      nullptr,
      0,
      nullptr,
      nullptr);
  std::string str(size_needed, 0);
  WideCharToMultiByte(
      CP_UTF8,
      0,
      &wstr[0],
      static_cast<int>(wstr.size()),
      &str[0],
      size_needed,
      nullptr,
      nullptr);
  return str;
}

////////////////////////////////////////////////////////////////////////

static std::wstring wide_stringify(const std::string& str) {
  if (str.empty()) {
    return std::wstring();
  }
  int size_needed = MultiByteToWideChar(
      CP_UTF8,
      0,
      &str[0],
      static_cast<int>(str.size()),
      nullptr,
      0);
  std::wstring wstr(size_needed, 0);
  MultiByteToWideChar(
      CP_UTF8,
      0,
      &str[0],
      static_cast<int>(str.size()),
      &wstr[0],
      size_needed);
  return wstr;
}
#endif // _WIN32

////////////////////////////////////////////////////////////////////////

inline std::string stringify(bool b) {
  return b ? "true" : "false";
}

////////////////////////////////////////////////////////////////////////

template <typename T>
std::string stringify(const std::set<T>& set) {
  std::ostringstream out;
  out << "{ ";
  typename std::set<T>::const_iterator iterator = set.begin();
  while (iterator != set.end()) {
    out << stringify(*iterator);
    if (++iterator != set.end()) {
      out << ", ";
    }
  }
  out << " }";
  return out.str();
}

////////////////////////////////////////////////////////////////////////

template <typename T>
std::string stringify(const std::list<T>& list) {
  std::ostringstream out;
  out << "[ ";
  typename std::list<T>::const_iterator iterator = list.begin();
  while (iterator != list.end()) {
    out << stringify(*iterator);
    if (++iterator != list.end()) {
      out << ", ";
    }
  }
  out << " ]";
  return out.str();
}

////////////////////////////////////////////////////////////////////////

template <typename T>
std::string stringify(const std::vector<T>& vector) {
  std::ostringstream out;
  out << "[ ";
  typename std::vector<T>::const_iterator iterator = vector.begin();
  while (iterator != vector.end()) {
    out << stringify(*iterator);
    if (++iterator != vector.end()) {
      out << ", ";
    }
  }
  out << " ]";
  return out.str();
}

////////////////////////////////////////////////////////////////////////

template <typename K, typename V>
std::string stringify(const std::map<K, V>& map) {
  std::ostringstream out;
  out << "{ ";
  typename std::map<K, V>::const_iterator iterator = map.begin();
  while (iterator != map.end()) {
    out << stringify(iterator->first);
    out << ": ";
    out << stringify(iterator->second);
    if (++iterator != map.end()) {
      out << ", ";
    }
  }
  out << " }";
  return out.str();
}

////////////////////////////////////////////////////////////////////////

template <typename T>
std::string stringify(const hashset<T>& set) {
  std::ostringstream out;
  out << "{ ";
  typename hashset<T>::const_iterator iterator = set.begin();
  while (iterator != set.end()) {
    out << stringify(*iterator);
    if (++iterator != set.end()) {
      out << ", ";
    }
  }
  out << " }";
  return out.str();
}

////////////////////////////////////////////////////////////////////////

template <typename K, typename V>
std::string stringify(const hashmap<K, V>& map) {
  std::ostringstream out;
  out << "{ ";
  typename hashmap<K, V>::const_iterator iterator = map.begin();
  while (iterator != map.end()) {
    out << stringify(iterator->first);
    out << ": ";
    out << stringify(iterator->second);
    if (++iterator != map.end()) {
      out << ", ";
    }
  }
  out << " }";
  return out.str();
}

////////////////////////////////////////////////////////////////////////

// TODO(chhsiao): This overload returns a non-const rvalue for consistency.
// Consider the following overloads instead for better performance:
//   const std::string& stringify(const Error&);
//   std::string stringify(Error&&);
inline std::string stringify(const Error& error) {
  return error.message;
}

////////////////////////////////////////////////////////////////////////
