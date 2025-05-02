#pragma once

#include "tl/expected.hpp"

////////////////////////////////////////////////////////////////////////

namespace stout {

////////////////////////////////////////////////////////////////////////

// Define aliases for 'tl::expected', 'tl::unexpected' and
// 'tl::make_unexpected' that are used to represent error-or-value types
// returned by code throughout stout. Errors are std::strings, and the
// error-or-value type is similar to C++23's std::expected.
// You can read more about 'tl::expected' library using the link below:
// https://github.com/TartanLlama/expected

template <typename T, typename Error = std::string>
using expected = tl::expected<T, Error>;

template <typename Error = std::string>
using unexpected = tl::unexpected<Error>;

template <typename ErrorInputType>
unexpected<std::string> make_unexpected(ErrorInputType&& e) {
  return tl::make_unexpected<std::string>(std::move(e));
}

////////////////////////////////////////////////////////////////////////

} // namespace stout

////////////////////////////////////////////////////////////////////////
