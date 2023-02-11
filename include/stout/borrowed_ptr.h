#pragma once

#include <functional>

#include "glog/logging.h"
#include "stout/stateful-tally.h"

////////////////////////////////////////////////////////////////////////

namespace stout {

////////////////////////////////////////////////////////////////////////

// Forward dependencies.
template <typename T>
class borrowed_ref;

template <typename T>
class borrowed_ptr;

template <typename F>
class borrowed_callable;

////////////////////////////////////////////////////////////////////////

// NOTE: currently this implementation of Borrowable does an atomic
// backoff instead of blocking the thread when the destructor waits
// for all borrows to be relinquished. This will be much less
// efficient (and hold up a CPU) if the borrowers take a while to
// relinquish. However, since Borrowable will mostly be used in
// cirumstances where the tally is definitely back to 0 when we wait
// no backoff will occur. For circumstances where Borrowable is being
// used to wait until work is completed consider using a Notification
// to be notified when the work is complete and then Borrowable should
// destruct without any atomic backoff (because any workers/threads
// will have relinquished).
class TypeErasedBorrowable {
 public:
  template <typename F>
  bool Watch(F&& f) {
    auto [state, count] = tally_.Wait([](auto, size_t) { return true; });

    do {
      if (state == State::Watching) {
        return false;
      } else if (count == 0) {
        f();
        return true;
      }

      CHECK_EQ(state, State::Borrowing);

    } while (!tally_.Update(state, count, State::Watching, count + 1));

    watch_ = std::move(f);

    Relinquish(__FILE__, __LINE__);

    return true;
  }

  void WaitUntilBorrowsEquals(size_t borrows) {
    tally_.Wait([&](auto /* state */, size_t count) {
      return count == borrows;
    });
  }

  size_t borrows() {
    return tally_.count();
  }

  void Relinquish(const char* file, int line) {
    auto [state, count] = tally_.Decrement();

    DLOG(INFO)
        << this << " relinquished from " << file << ":" << std::to_string(line)
        << " (borrows = " << tally_.count() << ")";

    if (state == State::Watching && count == 0) {
      // Move out 'watch_' in case it gets reset either in the
      // callback or because a concurrent call to 'borrow()' occurs
      // after we've updated the tally below.
      auto f = std::move(watch_);
      watch_ = std::function<void()>();

      tally_.Update(state, State::Borrowing);

      // At this point a call to 'borrow()' may mean that there are
      // outstanding 'borrowed_ref/ptr' when the watch callback gets
      // invoked and thus it's up to the users of this abstraction to
      // avoid making calls to 'borrow()' until after the watch
      // callback gets invoked if they want to guarantee that there
      // are no outstanding 'borrowed_ref/ptr'.

      f();
    }
  }

 protected:
  TypeErasedBorrowable()
    : tally_(State::Borrowing) {}

  TypeErasedBorrowable(const TypeErasedBorrowable& that)
    : tally_(State::Borrowing) {}

  TypeErasedBorrowable(TypeErasedBorrowable&& that)
    : tally_(State::Borrowing) {
    // We need to wait until all borrows have been relinquished so
    // any memory associated with 'that' can be safely released.
    that.WaitUntilBorrowsEquals(0);
  }

  virtual ~TypeErasedBorrowable() {
    auto state = State::Borrowing;
    if (!tally_.Update(state, State::Destructing)) {
      LOG(FATAL) << "Unable to transition to Destructing from state " << state;
    } else {
      // NOTE: it's possible that we'll block forever if exceptions
      // were thrown and destruction was not successful.
      // if (!std::uncaught_exceptions() > 0) {
      WaitUntilBorrowsEquals(0);
      // }
    }
  }

  enum class State : uint8_t {
    Borrowing,
    Watching,
    Destructing,
  };

  // We need to overload '<<' operator for 'State' enum class in
  // order to use 'CHECK_*' family macros.
  friend std::ostream& operator<<(
      std::ostream& os,
      const TypeErasedBorrowable::State& state) {
    switch (state) {
      case TypeErasedBorrowable::State::Borrowing:
        return os << "Borrowing";
      case TypeErasedBorrowable::State::Watching:
        return os << "Watching";
      case TypeErasedBorrowable::State::Destructing:
        return os << "Destructing";
      default:
        LOG(FATAL) << "Unreachable";
    }
  };

  // NOTE: 'stateful_tally' ensures this is non-moveable (but still
  // copyable). What would it mean to be able to borrow a pointer to
  // something that might move!? If an implemenetation ever replaces
  // 'stateful_tally' with something else care will need to be taken
  // to ensure that 'Borrowable' doesn't become moveable.
  StatefulTally<State> tally_;

  std::function<void()> watch_;

 private:
  // Only 'borrowed_ref/ptr' can reborrow!
  template <typename>
  friend class borrowed_ref;

  template <typename>
  friend class borrowed_ptr;

  template <typename>
  friend class borrowed_callable;

  void Reborrow(const char* file, int line) {
    auto [state, count] = tally_.Wait([](auto, size_t) { return true; });

    CHECK_GT(count, 0u);

    do {
      CHECK_NE(state, State::Destructing);
    } while (!tally_.Increment(state));

    DLOG(INFO)
        << this << " reborrowed at " << file << ":" << std::to_string(line)
        << " (borrows = " << tally_.count() << ")";
  }
};

////////////////////////////////////////////////////////////////////////

template <typename T>
class Borrowable : public TypeErasedBorrowable {
 public:
  template <
      typename... Args,
      std::enable_if_t<std::is_constructible_v<T, Args...>, int> = 0>
  Borrowable(Args&&... args)
    : TypeErasedBorrowable(),
      t_(std::forward<Args>(args)...) {}

  Borrowable(const Borrowable& that)
    : TypeErasedBorrowable(that),
      t_(that.t_) {}

  Borrowable(Borrowable&& that)
    : TypeErasedBorrowable(std::move(that)),
      t_(std::move(that.t_)) {}

  borrowed_ref<T> Borrow(const char* file, int line) {
    auto state = State::Borrowing;
    if (tally_.Increment(state)) {
      DLOG(INFO)
          << static_cast<TypeErasedBorrowable*>(this)
          << " borrowed at " << file << ":" << std::to_string(line)
          << " (borrows = " << tally_.count() << ")";
      return borrowed_ref<T>(*this, t_, file, line);
    } else {
      // Why are you borrowing when you shouldn't be?
      LOG(FATAL) << "Attempting to borrow in state " << state;
    }
  }

  template <typename F>
  borrowed_callable<F> Borrow(const char* file, int line, F&& f) {
    auto state = State::Borrowing;
    if (tally_.Increment(state)) {
      DLOG(INFO)
          << static_cast<TypeErasedBorrowable*>(this)
          << " borrowed at " << file << ":" << std::to_string(line)
          << " (borrows = " << tally_.count() << ")";
      return borrowed_callable<F>(std::forward<F>(f), this, file, line);
    } else {
      // Why are you borrowing when you shouldn't be?
      LOG(FATAL) << "Attempting to borrow in state " << state;
    }
  }

  T* get() {
    return &t_;
  }

  const T* get() const {
    return &t_;
  }

  T* operator->() {
    return get();
  }

  const T* operator->() const {
    return get();
  }

  T& operator*() {
    return t_;
  }

  const T& operator*() const {
    return t_;
  }

 private:
  T t_;
};

////////////////////////////////////////////////////////////////////////

template <typename T>
class enable_borrowable_from_this : public TypeErasedBorrowable {
 public:
  borrowed_ref<T> Borrow(const char* file, int line) {
    static_assert(
        std::is_base_of_v<enable_borrowable_from_this<T>, T>,
        "Type 'T' must derive from 'stout::enable_borrowable_from_this<T>'");

    auto state = State::Borrowing;
    if (tally_.Increment(state)) {
      DLOG(INFO)
          << static_cast<TypeErasedBorrowable*>(this)
          << " borrowed at " << file << ":" << std::to_string(line)
          << " (borrows = " << tally_.count() << ")";
      return borrowed_ref<T>(*this, *static_cast<T*>(this), file, line);
    } else {
      // Why are you borrowing when you shouldn't be?
      LOG(FATAL) << "Attempting to borrow in state " << state;
    }
  }

  template <typename F>
  borrowed_callable<F> Borrow(const char* file, int line, F&& f) {
    static_assert(
        std::is_base_of_v<enable_borrowable_from_this<T>, T>,
        "Type 'T' must derive from 'stout::enable_borrowable_from_this<T>'");

    auto state = State::Borrowing;
    if (tally_.Increment(state)) {
      DLOG(INFO)
          << static_cast<TypeErasedBorrowable*>(this)
          << " borrowed at " << file << ":" << std::to_string(line)
          << " (borrows = " << tally_.count() << ")";
      return borrowed_callable<F>(std::forward<F>(f), this, file, line);
    } else {
      // Why are you borrowing when you shouldn't be?
      LOG(FATAL) << "Attempting to borrow in state " << state;
    }
  }
};

////////////////////////////////////////////////////////////////////////

// Represents a borrowed reference to some borrowable of type
// 'T'. Unlike 'borrowed_ptr' a 'borrowed_ref' acts like a raw
// reference which is always non-null. Of course, if you move a
// 'borrowed_ref' then you'll get a runtime error if you attempt to
// "use after move" which is strictly safer than if you only used raw
// references, however, it also means that there is a set of possible
// patterns that are not expressible, in particular, you might be able
// to move a raw reference and still use that reference to point to
// allocated memory, but your mileage may vary depending on whether or
// not that is safe, and hence most of the time you always want to
// treat "use after move" as an error (which is what the clang-tidy
// check does as well).
template <typename T>
class borrowed_ref final {
 public:
  // Deleted copy constructor to force use of 'reborrow()' which makes
  // the copying more explicit!
  borrowed_ref(const borrowed_ref& that) = delete;

  borrowed_ref(borrowed_ref&& that) {
    std::swap(borrowable_, CHECK_NOTNULL(that.borrowable_));
    std::swap(t_, CHECK_NOTNULL(that.t_));
    std::swap(file_, CHECK_NOTNULL(that.file_));
    std::swap(line_, that.line_);
  }

  ~borrowed_ref() {
    // May have been moved!
    if (borrowable_ != nullptr) {
      borrowable_->Relinquish(file_, line_);
    }
  }

  borrowed_ref& operator=(borrowed_ref&& that) {
    std::swap(borrowable_, CHECK_NOTNULL(that.borrowable_));
    std::swap(t_, CHECK_NOTNULL(that.t_));
    std::swap(file_, CHECK_NOTNULL(that.file_));
    std::swap(line_, that.line_);
    return *this;
  }

  // template <
  //     typename U,
  //     std::enable_if_t<
  //         std::conjunction_v<
  //             std::negation<std::is_pointer<U>>,
  //             std::negation<std::is_reference<U>>,
  //             std::is_convertible<T*, U*>>,
  //         int> = 0>
  // operator borrowed_ref<U>() const& {
  //   CHECK_NOTNULL(borrowable_)->Reborrow("[T -> U const&]");
  //   return borrowed_ref<U>(*CHECK_NOTNULL(borrowable_), *CHECK_NOTNULL(t_));
  // }

  // template <
  //     typename U,
  //     std::enable_if_t<
  //         std::conjunction_v<
  //             std::negation<std::is_pointer<U>>,
  //             std::negation<std::is_reference<U>>,
  //             std::is_convertible<T*, U*>>,
  //         int> = 0>
  // operator borrowed_ref<U>() & {
  //   CHECK_NOTNULL(borrowable_)->Reborrow("[T -> U &]");
  //   return borrowed_ref<U>(*CHECK_NOTNULL(borrowable_), *CHECK_NOTNULL(t_));
  // }

  template <
      typename U,
      std::enable_if_t<
          std::conjunction_v<
              std::negation<std::is_pointer<U>>,
              std::negation<std::is_reference<U>>,
              std::is_convertible<T*, U*>>,
          int> = 0>
  operator borrowed_ref<U>() && {
    // Don't reborrow since we're being moved!
    TypeErasedBorrowable* borrowable = nullptr;
    T* t = nullptr;
    const char* file = nullptr;
    int line = -1;
    std::swap(borrowable, borrowable_);
    std::swap(t, t_);
    std::swap(file, file_);
    std::swap(line, line_);
    return borrowed_ref<U>(
        *CHECK_NOTNULL(borrowable),
        *CHECK_NOTNULL(t),
        file,
        line);
  }

  // template <
  //     typename U,
  //     std::enable_if_t<
  //         std::conjunction_v<
  //             std::negation<std::is_pointer<U>>,
  //             std::negation<std::is_reference<U>>,
  //             std::is_convertible<T*, U*>>,
  //         int> = 0>
  // operator borrowed_ptr<U>() const& {
  //   CHECK_NOTNULL(borrowable_)->Reborrow();
  //   return borrowed_ptr<U>(CHECK_NOTNULL(borrowable_), CHECK_NOTNULL(t_));
  // }

  // template <
  //     typename U,
  //     std::enable_if_t<
  //         std::conjunction_v<
  //             std::negation<std::is_pointer<U>>,
  //             std::negation<std::is_reference<U>>,
  //             std::is_convertible<T*, U*>>,
  //         int> = 0>
  // operator borrowed_ptr<U>() & {
  //   CHECK_NOTNULL(borrowable_)->Reborrow();
  //   return borrowed_ptr<U>(CHECK_NOTNULL(borrowable_), CHECK_NOTNULL(t_));
  // }

  template <
      typename U,
      std::enable_if_t<
          std::conjunction_v<
              std::negation<std::is_pointer<U>>,
              std::negation<std::is_reference<U>>,
              std::is_convertible<T*, U*>>,
          int> = 0>
  operator borrowed_ptr<U>() && {
    // Don't reborrow since we're being moved!
    TypeErasedBorrowable* borrowable = nullptr;
    T* t = nullptr;
    const char* file = nullptr;
    int line = -1;
    std::swap(borrowable, borrowable_);
    std::swap(t, t_);
    std::swap(file, file_);
    std::swap(line, line_);
    return borrowed_ptr<U>(borrowable, t, file, line);
  }

  borrowed_ref reborrow(const char* file, int line) const& {
    CHECK_NOTNULL(borrowable_)->Reborrow(file, line);
    return borrowed_ref<T>(
        *CHECK_NOTNULL(borrowable_),
        *CHECK_NOTNULL(t_),
        CHECK_NOTNULL(file),
        line);
  }

  borrowed_ref reborrow(const char* file, int line) && {
    DLOG(INFO)
        << borrowable_ << " relinquished from "
        << file_ << ":" << std::to_string(line_);
    DLOG(INFO)
        << borrowable_ << " reborrowed at " << file << ":" << line;

    // Don't reborrow since we're being moved!
    TypeErasedBorrowable* borrowable = nullptr;
    T* t = nullptr;
    std::swap(borrowable, borrowable_);
    std::swap(t, t_);
    file_ = nullptr;
    line_ = -1;
    return borrowed_ref<T>(
        *CHECK_NOTNULL(borrowable),
        *CHECK_NOTNULL(t),
        file,
        line);
  }

  T* get() const {
    return CHECK_NOTNULL(t_);
  }

  T* operator->() const {
    return get();
  }

  T& operator*() const {
    return *get();
  }

  // TODO(benh): operator[]

  template <typename H>
  friend H AbslHashValue(H h, const borrowed_ref& that) {
    return H::combine(std::move(h), &that.t_);
  }

 private:
  template <typename>
  friend class borrowed_ref;

  template <typename>
  friend class borrowed_ptr;

  template <typename>
  friend class Borrowable;

  template <typename>
  friend class enable_borrowable_from_this;

  borrowed_ref(
      TypeErasedBorrowable& borrowable,
      T& t,
      const char* file,
      int line)
    : borrowable_(&borrowable),
      t_(&t),
      file_(file),
      line_(line) {}

  TypeErasedBorrowable* borrowable_ = nullptr;
  T* t_ = nullptr;
  const char* file_ = nullptr;
  int line_ = -1;
};

////////////////////////////////////////////////////////////////////////

// Like 'borrowed_ref' except similar to a raw pointer (and
// 'std::unique_ptr') it can be a 'nullptr', for example, by
// constructing a 'borrowed_ptr' with the default constructor or after
// calling 'relinquish()'.
template <typename T>
class borrowed_ptr final {
 public:
  borrowed_ptr() {}

  // Deleted copy constructor to force use of 'reborrow()' which makes
  // the copying more explicit!
  borrowed_ptr(const borrowed_ptr& that) = delete;

  borrowed_ptr(borrowed_ptr&& that) {
    std::swap(borrowable_, that.borrowable_);
    std::swap(t_, that.t_);
    std::swap(file_, that.file_);
    std::swap(line_, that.line_);
  }

  ~borrowed_ptr() {
    relinquish();
  }

  borrowed_ptr& operator=(borrowed_ptr&& that) {
    std::swap(borrowable_, that.borrowable_);
    std::swap(t_, that.t_);
    std::swap(file_, that.file_);
    std::swap(line_, that.line_);
    return *this;
  }

  explicit operator bool() const {
    return borrowable_ != nullptr;
  }

  // template <
  //     typename U,
  //     std::enable_if_t<
  //         std::conjunction_v<
  //             std::negation<std::is_pointer<U>>,
  //             std::negation<std::is_reference<U>>,
  //             std::is_convertible<T*, U*>>,
  //         int> = 0>
  // operator borrowed_ptr<U>() const& {
  //   if (borrowable_ != nullptr) {
  //     borrowable_->Reborrow(file_, line_);
  //     return borrowed_ptr<U>(borrowable_, t_, file_, line_);
  //   } else {
  //     return borrowed_ptr<U>();
  //   }
  // }

  // template <
  //     typename U,
  //     std::enable_if_t<
  //         std::conjunction_v<
  //             std::negation<std::is_pointer<U>>,
  //             std::negation<std::is_reference<U>>,
  //             std::is_convertible<T*, U*>>,
  //         int> = 0>
  // operator borrowed_ptr<U>() & {
  //   if (borrowable_ != nullptr) {
  //     borrowable_->Reborrow(file_, line_);
  //     return borrowed_ptr<U>(borrowable_, t_, file_, line_);
  //   } else {
  //     return borrowed_ptr<U>();
  //   }
  // }

  template <
      typename U,
      std::enable_if_t<
          std::conjunction_v<
              std::negation<std::is_pointer<U>>,
              std::negation<std::is_reference<U>>,
              std::is_convertible<T*, U*>>,
          int> = 0>
  operator borrowed_ptr<U>() && {
    // Don't reborrow since we're being moved!
    TypeErasedBorrowable* borrowable = nullptr;
    T* t = nullptr;
    const char* file = nullptr;
    int line = -1;
    std::swap(borrowable, borrowable_);
    std::swap(t, t_);
    std::swap(file, file_);
    std::swap(line, line_);
    return borrowed_ptr<U>(borrowable, t, file, line);
  }

  borrowed_ptr reborrow(const char* file, int line) const& {
    if (borrowable_ != nullptr) {
      borrowable_->Reborrow(file, line);
      return borrowed_ptr<T>(borrowable_, t_, file, line);
    } else {
      return borrowed_ptr<T>();
    }
  }

  borrowed_ptr reborrow(const char* file, int line) && {
    DLOG(INFO)
        << borrowable_ << " transferred from "
        << file_ << ":" << std::to_string(line_)
        << " to " << file << ":" << line;

    // Don't reborrow since we're being moved!
    TypeErasedBorrowable* borrowable = nullptr;
    T* t = nullptr;
    std::swap(borrowable, borrowable_);
    std::swap(t, t_);
    file_ = nullptr;
    line_ = -1;
    return borrowed_ptr<T>(borrowable, t, file, line);
  }

  void relinquish() {
    if (borrowable_ != nullptr) {
      borrowable_->Relinquish(file_, line_);
      borrowable_ = nullptr;
      t_ = nullptr;
    }
  }

  T* get() const {
    return t_;
  }

  T* operator->() const {
    return get();
  }

  T& operator*() const {
    // NOTE: just like with 'std::unique_ptr' the behavior is
    // undefined if 'get() == nullptr'.
    return *get();
  }

  // TODO(benh): operator[]

  template <typename H>
  friend H AbslHashValue(H h, const borrowed_ptr& that) {
    return H::combine(std::move(h), that.t_);
  }

 private:
  template <typename>
  friend class borrowed_ptr;

  template <typename>
  friend class borrowed_ref;

  template <typename>
  friend class Borrowable;

  borrowed_ptr(
      TypeErasedBorrowable* borrowable,
      T* t,
      const char* file,
      int line)
    : borrowable_(borrowable),
      t_(t),
      file_(file),
      line_(line) {}

  TypeErasedBorrowable* borrowable_ = nullptr;
  T* t_ = nullptr;
  const char* file_ = nullptr;
  int line_ = -1;
};

////////////////////////////////////////////////////////////////////////

// Helper type that is callable and handles ensuring a 'borrowed_ptr'
// is borrowed until the callable is destructed.
template <typename F>
class borrowed_callable final {
 public:
  borrowed_callable(
      F f,
      TypeErasedBorrowable* borrowable,
      const char* file,
      int line)
    : f_(std::move(f)),
      borrowable_(CHECK_NOTNULL(borrowable)),
      file_(file),
      line_(line) {}

  // borrowed_callable(const borrowed_callable& that)
  //   : f_(that.f_),
  //     borrowable_([&]() -> TypeErasedBorrowable* {
  //       if (that.borrowable_ != nullptr) {
  //         that.borrowable_->Reborrow(that.file_, that.line_);
  //         return that.borrowable_;
  //       } else {
  //         return nullptr;
  //       }
  //     }()) {}

  borrowed_callable(borrowed_callable&& that)
    : f_(std::move(that.f_)) {
    std::swap(borrowable_, that.borrowable_);
    std::swap(file_, that.file_);
    std::swap(line_, that.line_);
  }

  ~borrowed_callable() {
    if (borrowable_ != nullptr) {
      borrowable_->Relinquish(file_, line_);
    }
  }

  template <typename... Args>
  decltype(auto) operator()(Args&&... args) const& {
    return f_(std::forward<Args>(args)...);
  }

  template <typename... Args>
  decltype(auto) operator()(Args&&... args) & {
    return f_(std::forward<Args>(args)...);
  }

  template <typename... Args>
  decltype(auto) operator()(Args&&... args) && {
    return std::move(f_)(std::forward<Args>(args)...);
  }

 private:
  F f_;
  TypeErasedBorrowable* borrowable_ = nullptr;
  const char* file_;
  int line_ = -1;
};

////////////////////////////////////////////////////////////////////////

template <typename T>
auto Borrow(
    const char* file,
    int line,
    Borrowable<T>& borrowable) {
  return borrowable.Borrow(file, line);
}

template <typename T, typename F>
auto Borrow(
    const char* file,
    int line,
    Borrowable<T>& borrowable,
    F&& f) {
  return borrowable.Borrow(file, line, std::forward<F>(f));
}

template <typename T>
auto Borrow(
    const char* file,
    int line,
    enable_borrowable_from_this<T>& borrowable) {
  return borrowable.Borrow(file, line);
}

template <typename T, typename F>
auto Borrow(
    const char* file,
    int line,
    enable_borrowable_from_this<T>& borrowable,
    F&& f) {
  return borrowable.Borrow(file, line, std::forward<F>(f));
}

template <typename T>
auto Reborrow(
    const char* file,
    int line,
    const borrowed_ref<T>& borrowed) {
  return borrowed.reborrow(file, line);
}

template <typename T>
auto Reborrow(
    const char* file,
    int line,
    borrowed_ref<T>&& borrowed) {
  return std::move(borrowed).reborrow(file, line);
}

template <typename T>
auto Reborrow(
    const char* file,
    int line,
    const borrowed_ptr<T>& borrowed) {
  return borrowed.reborrow(file, line);
}

template <typename T>
auto Reborrow(
    const char* file,
    int line,
    borrowed_ptr<T>&& borrowed) {
  return std::move(borrowed).reborrow(file, line);
}

////////////////////////////////////////////////////////////////////////

} // namespace stout

////////////////////////////////////////////////////////////////////////

// NEED VERSION OF BORROW THAT IF IT'S a std::borrowed_ptr we do the
// null check and then return a std::borrowed_ref

#define Borrow(...) \
  stout::Borrow(__FILE__, __LINE__, __VA_ARGS__)

////////////////////////////////////////////////////////////////////////

#define BorrowThis(...) Borrow(*this, __VA_ARGS__)

////////////////////////////////////////////////////////////////////////

#define Reborrow(borrowed) \
  stout::Reborrow(__FILE__, __LINE__, (borrowed))

////////////////////////////////////////////////////////////////////////
