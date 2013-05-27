Stout is a header-only C++ library.

No action is needed if you would like to use this library in your
project. Simply add the include folder to your include path during
compilation.

Depending on which headers you'd like to use, you may require the
following third party libraries:

  - Boost
  - Google's glog (this dependency will be removed in the future)
  - Google's protobuf
  - Google's gmock/gtest

---

## User Guide

There are a handful of primitives and collections that are provided
within the library, as well as some namespaced and miscellaneous
utilities.

It probably makes sense to at least take a look at [Option](#option),
[Try](#try), and [Result](#result) as they are used extensively
throughout the library. Note that the library is designed to
completely avoid exceptions. See [exceptions](#exceptions) for further
discussion.


* <a href="#primitives">Primitives</a>
  - <a href="#duration">Duration</a>
  - <a href="#error">Error</a>
  - <a href="#none">None</a>
  - <a href="#nothing">Nothing</a>
  - <a href="#option">Option</a>
  - <a href="#owned">Owned</a>
  - <a href="#result">Result</a>
  - <a href="#try">Try</a>
  - <a href="#stopwatch">Stopwatch</a>
  - <a href="#uuid">UUID</a>
* <a href="#collections">Collections</a>
  - <a href="#cache">cache</a>
  - <a href="#hashmap">hashmap</a>
  - <a href="#hashset">hashset</a>
  - <a href="#multihashmap">multihashmap</a>
* <a href="#namespaces">Namespaces</a>
  - <a href="#fs">fs</a>
  - <a href="#gzip">gzip</a>
  - <a href="#json">JSON</a>
  - <a href="#lambda">lambda</a>
  - <a href="#net">net</a>
  - <a href="#os">os</a>
  - <a href="#path">path</a>
  - <a href="#protobuf">protobuf</a>
  - <a href="#strings">strings</a>
* <a href="#miscellaneous">Miscellaneous</a>
  - <a href="#copy">copy</a>
  - <a href="#exit">EXIT</a>
  - <a href="#fatal">fatal</a>
  - <a href="#foreach>foreach</a>
  - <a href="#gtest">gtest</a>
  - <a href="#numify">numify</a>
  - <a href="#preprocessor">preprocessor</a>
  - <a href="#stringify">stringify</a>
* <a href="#philosophy">Philosophy</a>
  - <a href="#exceptions">Exceptions</a>


<a name="primitives"></a>

## Primitives

**Note that none of the primitives are namespaced!**


<a name="duration"></a>

### Duration

Used to represent some duration of time. The main way to construct a
`Duration` is to invoke `Duration::parse` which expects a string made
up of a number and a unit, i.e., `42ns`, `42us`, `42ms`, `42secs`,
`42mins`, `42hrs`, `42days`, `42weeks`. For each of the supported
units there are associated types: `Nanoseconds`, `Microseconds`,
`Milliseconds`, `Seconds`, `Minutes`, `Hours`, `Days`, `Weeks`. Each
of these types inherit from `Duration` and can be used anywhere a
`Duration` is expected, for example:

```cpp
        Duration d = Seconds(5);
```

Note that we also provide an overload of the `std::ostream operator
<<` for `Duration` that formats the output (including the unit) based
on the magnitude (e.g., `Seconds(42)` outputs `42secs` but
`Seconds(120)` outputs `2mins`).


<a href="error"></a>

### Error

Primitives such as [Try](#try) and [Result](#result) can represent
errors. You can explicitly construct one of these types as an error,
but it's a bit verbose. The `Error` type acts as "syntactic sugar" for
implicitly constructing one of these types. That is, `Error` is
implicitly convertible to a `Try<T>` or `Result<T>` for any `T`. For
example:

```cpp
        Try<bool> parse(const std::string& s) {
          if (s == "true") return true;
          else if (s == "false") return false;
          else return Error("Failed to parse string as boolean");
        }

        Try<bool> t = parse("false");
```


<a href="none"></a>

### None

Similar to [Error](#error), the `None` type acts as "syntactic sugar"
to make using [Option](#option) less verbose. For example:

```cpp
        Option<bool> o = None();
```


<a href="nothing"></a>

### Nothing

A lot of functions that return `void` can also "return" an
error. Since we don't use exceptions (see [Exceptions](#exceptions))
we capture this pattern using `Try<Nothing>` (see [Try](#try).


<a href="option"></a>

### Option

The `Option` type provides a safe alternative to using `NULL`. There
are implicit constructors provided by `Option` as well:

```cpp
        Option<bool> o = true;
```

Note that the current implementation *copies* the underlying
values. See [Philosophy](#philosophy) for more discussion. Nothing
prevents you from using pointers, however, *the pointer will not be
deleted when the Option is destructed*:

```cpp
        Option<std::string*> o = new std::string("hello world");
```


<a href="owned"></a>

### Owned

The `Owned` type represents a uniquely owned pointer. With C++11 this
will extend `std::unique_ptr`, requiring the user to adhere to move
semantics. Until then, an `Owned` instance inherits from
`std::tr1::shared_ptr` and is used more as a placeholder for where we
want to use move semantics in the future.


<a href="result"></a>

### Result


<a href="try"></a>

### Try


<a href="stopwatch"></a>

### Stopwatch


<a href="uuid"></a>

### UUID


<a href="collections"></a>

## Collections

The library includes a few different collections. Mostly these are
wrappers around existing collections but with modified interfaces to
provide a more monadic apporach (e.g., returning an
[Option](#option)).


<a name="cache"></a>

### cache

A templated implementation of a least-recently used (LRU) cache. Note
that the key type must be compatible with `std::tr1::unordered_map`.
The interface is rather poor right now, only providing 'put' and 'get'
operations.


<a href="hashmap"></a>

### hashmap

*Requires Boost.*


<a href="hashset"></a>

### hashset

*Requires Boost.*


<a href="multihashmap"></a>

### multihashmap

*Requires Boost.*


<a href="namespaces"></a>

## Namespaces

There are a fair number of utilities behind a few namespaces.


<a href="fs"></a>

### fs


<a href="gzip"></a>

### gzip


<a href="json"></a>

### JSON


<a href="lambda"></a>

### lambda


<a href="net"></a>

### net


<a href="os"></a>

### os


<a href="path"></a>

### path


<a href="protobuf"></a>

### protobuf

*Requires protobuf.*


<a href="strings"></a>

### strings


<a href="miscellaneous"></a>

## Miscellaneous

Like the primitives, these miscellaneous utilities are **not**
namespaced.


<a href="copy"></a>

### copy


<a href="exit"></a>

### EXIT


<a href="fatal"></a>

### fatal


<a href="foreach></a>

### foreach

*Requires Boost.*


<a href="gtest"></a>

### gtest

*Requires gtest.*


<a href="numify"></a>

### numify


<a href="preprocessor"></a>

### preprocessor

*Requires Boost.*


<a href="stringify"></a>

### stringify


<a href="philosophy"></a>

## Philosophy

*"Premature optimization is the root of all evil."*

You'll notice that the library is designed in a way that can lead to a
lot of copying. This decision was deliberate. Capturing the semantics
of pointer ownership is hard to enforce programmatically unless you
copy, and in many instances these copies can be elided by an
optimizing compiler. We've choosen safety rather than premature
optimizations.

Note, however, that we plan to liberally augment the library as we add
C++11 support. In particular, we plan to use rvalue references and
std::unique_ptr (although, likely wrapped as Owned) in order to
explicitly express ownership semantics. Until then, it's unlikely that
the performance overhead incurred via any extra copying is your
bottleneck, and if it is we'd love to hear from you!


<a href="exceptions"></a>

### Exceptions

The library WILL NEVER throw exceptions and will attempt to capture
any exceptions thrown by underlying C++ functions and convert them
into an [Error](#error).


## Building Tests

We'll assume you've got a distribution of gmock and have already built
a static archive called libgmock.a (see gmock's README to learn
how). We'll also assume the Boost, glog, and protobuf headers can be
found via the include paths and libglog.* can be found via the library
search paths. You can then build the tests via:

       $ g++ -I${STOUT}/include -I$(GMOCK)/gtest/include -I$(GMOCK)/include \
         ${STOUT}/tests/tests.cpp libgmock.a -lglog -o tests

Note that if you want to test the gzip headers you'll need to define
HAVE_LIBZ and link against libz:

       $ g++ -I${STOUT}/include -I$(GMOCK)/gtest/include -I$(GMOCK)/include \
         -DHAVE_LIBZ ${STOUT}/tests/tests.cpp libgmock.a -lglog -lz -o tests


