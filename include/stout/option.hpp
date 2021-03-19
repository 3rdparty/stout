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

#ifndef __STOUT_OPTION_HPP__
#define __STOUT_OPTION_HPP__

#include <assert.h>

#include <algorithm>
#include <functional>
#include <type_traits>
#include <utility>

#include <boost/functional/hash.hpp>

#include <stout/none.hpp>
#include <stout/some.hpp>

template <typename T>
class Option
{
public:
  static Option<T> none()
  {
    return Option<T>();
  }

  static Option<T> some(const T& t)
  {
    return Option<T>(t);
  }

  Option() : state(NONE) {}

  Option(const T& _t) : state(SOME), t(_t) {}

  Option(T&& _t) : state(SOME), t(std::move(_t)) {}

  template <
    typename U,
    typename = typename std::enable_if<
        std::is_constructible<T, const U&>::value>::type>
  Option(const U& u) : state(SOME), t(u) {}

  Option(const None&) : state(NONE) {}

  template <typename U>
  Option(const _Some<U>& some) : state(SOME), t(some.t) {}

  template <typename U>
  Option(_Some<U>&& some) : state(SOME), t(std::move(some.t)) {}

  Option(const Option<T>& that) : state(that.state)
  {
    if (that.isSome()) {
      new (&t) T(that.t);
    }
  }

  Option(Option<T>&& that)
      noexcept(std::is_nothrow_move_constructible<T>::value)
    : state(std::move(that.state))
  {
    if (that.isSome()) {
      new (&t) T(std::move(that.t));
    }
  }

  ~Option()
  {
    if (isSome()) {
      t.~T();
    }
  }

  Option<T>& operator=(const Option<T>& that)
  {
    if (this != &that) {
      if (isSome()) {
        t.~T();
      }
      state = that.state;
      if (that.isSome()) {
        new (&t) T(that.t);
      }
    }

    return *this;
  }

  Option<T>& operator=(Option<T>&& that)
      noexcept(std::is_nothrow_move_constructible<T>::value)
  {
    if (this != &that) {
      if (isSome()) {
        t.~T();
      }
      state = std::move(that.state);
      if (that.isSome()) {
        new (&t) T(std::move(that.t));
      }
    }

    return *this;
  }

  bool isSome() const { return state == SOME; }
  bool isNone() const { return state == NONE; }

  const T& get() const & { assert(isSome()); return t; }
  T& get() & { assert(isSome()); return t; }
  T&& get() && { assert(isSome()); return std::move(t); }
  const T&& get() const && { assert(isSome()); return std::move(t); }

  const T* operator->() const { return &get(); }
  T* operator->() { return &get(); }

  const T& operator*() const& { return get(); }
  T& operator*() & { return get(); }
  const T&& operator*() const&& { return std::move(*this).get(); }
  T&& operator*() && { return std::move(*this).get(); }

  template <typename U>
  T getOrElse(U&& u) const &
  {
    return isNone() ? static_cast<T>(std::forward<U>(u)) : t;
  }

  template <typename U>
  T getOrElse(U&& u) &&
  {
    return isNone() ? static_cast<T>(std::forward<U>(u)) : std::move(t);
  }

  bool operator==(const Option<T>& that) const
  {
    return (isNone() && that.isNone()) ||
      (isSome() && that.isSome() && t == that.t);
  }

  bool operator!=(const Option<T>& that) const
  {
    return !(*this == that);
  }

  bool operator==(const T& that) const
  {
    return isSome() && t == that;
  }

  bool operator!=(const T& that) const
  {
    return !(*this == that);
  }

private:
  enum State
  {
    SOME,
    NONE,
  };

  State state;

  union {
    // We remove the const qualifier (if there is one) from T so that
    // we can initialize 't' from outside of the initializer list
    // using placement new. This is necessary because sometimes we
    // specialize 'Option' as such: 'Option<const T>'.
    typename std::remove_const<T>::type t;
  };
};


template <typename T>
Option<T> min(const Option<T>& left, const Option<T>& right)
{
  if (left.isSome() && right.isSome()) {
    return std::min(left.get(), right.get());
  } else if (left.isSome()) {
    return left.get();
  } else if (right.isSome()) {
    return right.get();
  } else {
    return Option<T>::none();
  }
}


template <typename T>
Option<T> min(const Option<T>& left, const T& right)
{
  return min(left, Option<T>(right));
}


template <typename T>
Option<T> min(const T& left, const Option<T>& right)
{
  return min(Option<T>(left), right);
}


template <typename T>
Option<T> max(const Option<T>& left, const Option<T>& right)
{
  if (left.isSome() && right.isSome()) {
    return std::max(left.get(), right.get());
  } else if (left.isSome()) {
    return left.get();
  } else if (right.isSome()) {
    return right.get();
  } else {
    return Option<T>::none();
  }
}


template <typename T>
Option<T> max(const Option<T>& left, const T& right)
{
  return max(left, Option<T>(right));
}


template <typename T>
Option<T> max(const T& left, const Option<T>& right)
{
  return max(Option<T>(left), right);
}

namespace std {

template <typename T>
struct hash<Option<T>>
{
  typedef size_t result_type;

  typedef Option<T> argument_type;

  result_type operator()(const argument_type& option) const
  {
    size_t seed = 0;
    if (option.isSome()) {
      boost::hash_combine(seed, hash<T>()(option.get()));
    }
    return seed;
  }
};

} // namespace std {

#endif // __STOUT_OPTION_HPP__
