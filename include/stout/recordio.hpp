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

#ifndef __STOUT_RECORDIO_HPP__
#define __STOUT_RECORDIO_HPP__

#include <stdlib.h>

#include <deque>
#include <functional>
#include <string>

#include <stout/check.hpp>
#include <stout/foreach.hpp>
#include <stout/numify.hpp>
#include <stout/option.hpp>
#include <stout/stringify.hpp>
#include <stout/try.hpp>

/**
 * Provides facilities for "Record-IO" encoding of data.
 * "Record-IO" encoding allows one to encode a sequence
 * of variable-length records by prefixing each record
 * with its size in bytes:
 *
 * 5\n
 * hello
 * 6\n
 * world!
 *
 * Note that this currently only supports record lengths
 * encoded as base 10 integer values with newlines as a
 * delimiter. This is to provide better language portability
 * portability: parsing a base 10 integer is simple. Most
 * other "Record-IO" implementations use a fixed-size header
 * of 4 bytes to directly encode an unsigned 32 bit length.
 *
 * TODO(bmahler): Move this to libprocess and support async
 * consumption of data.
 */
namespace recordio {

/**
 * Given an encoding function for individual records, this
 * provides encoding from typed records into "Record-IO" data.
 */
template <typename T>
class Encoder
{
public:
  Encoder(std::function<std::string(const T&)> _serialize)
    : serialize(_serialize) {}

  /**
   * Returns the "Record-IO" encoded record.
   */
  std::string encode(const T& record) const
  {
    std::string s = serialize(record);
    return stringify(s.size()) + "\n" + s;
  }

private:
  std::function<std::string(const T&)> serialize;
};


/**
 * Given a decoding function for individual records, this
 * provides decoding from "Record-IO" data into typed records.
 */
template <typename T>
class Decoder
{
public:
  Decoder(std::function<Try<T>(const std::string&)> _deserialize)
    : state(HEADER), deserialize(_deserialize) {}

  /**
   * Decodes another chunk of data from the "Record-IO" stream
   * and returns the attempted decoding of any additional
   * complete records.
   *
   * Returns an Error if the data contains an invalid length
   * header, at which point the decoder will return Error for
   * all subsequent calls.
   *
   * TODO(bmahler): Allow the caller to signal EOF, this allows
   * detection of invalid partial data at the end of the input.
   */
  Try<std::deque<Try<T>>> decode(const std::string& data)
  {
    if (state == FAILED) {
      return Error("Decoder is in a FAILED state");
    }

    std::deque<Try<T>> records;

    foreach (char c, data) {
      if (state == HEADER) {
        // Keep reading until we have the entire header.
        if (c != '\n') {
          buffer += c;
          continue;
        }

        Try<size_t> numify = ::numify<size_t>(buffer);

        // If we were unable to decode the length header, do not
        // continue decoding since we cannot determine where to
        // pick up the next length header!
        if (numify.isError()) {
          state = FAILED;
          return Error("Failed to decode length '" + buffer + "': " +
                       numify.error());
        }

        length = numify.get();
        buffer.clear();
        state = RECORD;

        // Note that for 0 length records, we immediately decode.
        if (numify.get() <= 0) {
          records.push_back(deserialize(buffer));
          state = HEADER;
        }
      } else if (state == RECORD) {
        CHECK_SOME(length);
        CHECK_LT(buffer.size(), length.get());

        buffer += c;

        if (buffer.size() == length.get()) {
          records.push_back(deserialize(buffer));
          buffer.clear();
          state = HEADER;
        }
      }
    }

    return records;
  }

private:
  enum
  {
    HEADER,
    RECORD,
    FAILED
  } state;

  // TODO(bmahler): Avoid string here as it will not free
  // its underlying memory allocation when we clear it.
  std::string buffer;
  Option<size_t> length;

  std::function<Try<T>(const std::string&)> deserialize;
};

} // namespace recordio {

#endif // __STOUT_RECORDIO_HPP__
