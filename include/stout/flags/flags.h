#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>

#include "absl/time/time.h"
#include "glog/logging.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/duration.pb.h"
#include "google/protobuf/io/tokenizer.h"
#include "google/protobuf/text_format.h"
#include "stout/flags/v1/flag.pb.h"
#include "stout/flags/v1/positional_argument.pb.h"
#include "stout/flags/v1/subcommand.pb.h"

////////////////////////////////////////////////////////////////////////

namespace stout::flags {

////////////////////////////////////////////////////////////////////////

// Forward declaration.
template <typename Flags>
class ParserBuilder;


class Parser {
 public:
  // Returns a builder for a parser based on the specifid flags; see
  // 'ParserBuilder' for more details on what can be modified from the
  // default parser.
  template <typename Flags>
  static ParserBuilder<Flags> Builder(Flags& flags, bool nested = false);

  // Parse flags from 'argc' and 'argv' and modify 'argc' and 'argv'
  // with what ever flags or arguments were not parsed.
  void Parse(int* argc, const char*** argv);

 private:
  template <typename Flags>
  friend class ParserBuilder;

  Parser()
    : standard_flags_(new stout::v1::StandardFlags()) {
    // Add all the "standard flags" first, e.g., --help.
    AddFieldsAndSubcommandsOrExit(*standard_flags_);
  }

  // Helper that fill fields_ and messages_ helpers.
  void AddFieldsAndSubcommandsOrExit(google::protobuf::Message& message);

  Parser* TryLookupNestedParserForSubcommand(const std::string& name);

  // Helper that gets 'google::protobuf::Message*' that we want to
  // populate for the specified field descriptor.
  google::protobuf::Message& GetMessageForField(
      const google::protobuf::FieldDescriptor& field);

  // Helper that gets 'google::protobuf::Message*' that we want to
  // populate for the specified subcommand.
  google::protobuf::Message& GetMessageForSubcommand(
      const std::string& name);

  // Helper struct which stores the argument name and value (if present)
  // from the command line.
  struct ArgumentInfo {
    std::string name;
    std::optional<std::string> value;
  };

  // Helper for parsing a normalized form of flags.
  void Parse(const std::vector<ArgumentInfo>& values);

  // Helper that populates the specific protobuf message's field
  // with some value or just aggregates errors on any failure.
  void SetFieldMessageOrAggregateErrors(
      const std::string& value,
      const std::string& name,
      const google::protobuf::FieldDescriptor* field,
      google::protobuf::Message* message,
      std::set<std::string>& errors);

  // Helper that normalizes default field value if the type is 'string'.
  std::string GetNormalizedDefaultValue(
      const std::string& value,
      const google::protobuf::FieldDescriptor::Type& type);

  // Helper that prints out help for the flags for this parser.
  void PrintHelp();

  // NOTE: need to heap allocate with a 'std::unique_ptr' here because
  // we 'std::move()' the parser but also store a pointer to
  // 'standard_flags_' so that pointer must be stable. This allocation
  // will not likely be in the fast path but if it is this design
  // should be reconsidered.
  std::unique_ptr<stout::v1::StandardFlags> standard_flags_;

  // Map from flag name to the field descriptor for the flag.
  std::map<std::string, const google::protobuf::FieldDescriptor*> fields_;

  // Map from subcommand name to the field descriptor for the flag.
  //
  // NOTE: we need to separate 'fields_' and 'subcommand_fields_' to
  // support flags and subcommands having the same name.
  std::map<std::string, const google::protobuf::FieldDescriptor*>
      subcommand_fields_;

  // Struct which represents a positional argument for
  // a specific 'protobuf' message field with
  // stout::v1::argument option.
  struct PositionalArgument {
    std::string name;
    const google::protobuf::FieldDescriptor* field = nullptr;
  };

  // Vector for positional arguments. Will be populated at the
  // build phase (AddFieldsAndSubcommandsOrExit()). We expect
  // that users will define positional arguments in the command
  // line at the same order in which they are defined in
  // 'protobuf' message. That's why we use 'vector' instead of
  // 'map'.
  std::vector<PositionalArgument> positional_args_;

  // Map from message descriptors to functions that overload the
  // default parsing for that descriptor.
  std::map<
      const google::protobuf::Descriptor*,
      std::function<
          std::optional<std::string>(
              const std::string&,
              google::protobuf::Message&)>>
      overload_parsing_;

  // Map from help string to function that we use to validate the
  // parsed flags.
  std::map<
      std::string,
      std::function<bool(google::protobuf::Message&)>>
      validate_;

  // Command "basename" extracted from 'argv[0]'.
  std::string command_;

  // Helper struct for storing the parsed "name" and normalized
  // protobuf "text" for a flag. This is used for handling possible
  // duplicates.
  struct Parsed {
    std::string name;
    std::string text;
  };

  // Map from field descriptor to the helper struct 'Parsed' for
  // capturing what flags have already been parsed.
  std::map<const google::protobuf::FieldDescriptor*, Parsed> parsed_;

  // Optional for including all environment variables for parsing
  // with specific prefix.
  std::optional<std::string> environment_variable_prefix_;

  // Map from nested parser to it's all possible names. Depending on the
  // subcommand defined by the users we can grab specific parser and do
  // parsing for the next arguments in the command line.
  std::map<const google::protobuf::FieldDescriptor*, std::unique_ptr<Parser>>
      nested_parsers_;

  // Message to populate when parsing. May be nullptr until parsing
  // for nested parsers because we won't know the pointer until
  // parsing in the event the parent parse.
  google::protobuf::Message* message_ = nullptr;
};

////////////////////////////////////////////////////////////////////////


template <typename Flags>
class ParserBuilder {
 public:
  ParserBuilder(Flags& flags, bool nested) {
    parser_.AddFieldsAndSubcommandsOrExit(flags);

    // When constructing a top-level parser we save the 'flags'
    // protobuf message to later populate when we call
    // 'Parse(...)'. We can't save 'flags' for nested parsers because
    // 'flags' might not be valid if it (or it's "parent") is a oneof
    // field and thus we need to compute it during the parse.
    if (!nested) {
      parser_.message_ = &flags;
    }
  }

  // Overloads the parsing of the specified type 'T' with the
  // specified function. Note that 'T' must be a protobuf.
  template <typename T, typename F>
  ParserBuilder& OverloadParsing(F&& f) {
    if (!TryOverloadParsing<T>(std::forward<F>(f))) {
      std::cerr
          << "Encountered more than one overload parsing for "
          << T().GetDescriptor()->full_name()
          << std::endl;
      std::exit(1);
    }

    return *this;
  }

  // Adds a validation function to be invoked that will print out the
  // specified help message if validation fails.
  template <typename F>
  ParserBuilder& Validate(std::string&& help, F&& f) {
    parser_.validate_.emplace(
        std::move(help),
        [f = std::forward<F>(f)](google::protobuf::Message& message) {
          return f(
              *google::protobuf::DynamicCastToGenerated<Flags>(&message));
        });

    return *this;
  }

  // Returns a parser.
  Parser Build() {
    // Try to overload parsing of 'google.protobuf.Duration' flag
    // fields and ignore if already overloaded.
    TryOverloadParsing<google::protobuf::Duration>(
        [](const std::string& value, auto& duration) {
          absl::Duration d;
          std::string error;
          if (!absl::AbslParseFlag(value, &d, &error)) {
            return std::optional<std::string>(error);
          } else {
            duration.set_seconds(
                absl::IDivDuration(d, absl::Seconds(1), &d));
            duration.set_nanos(
                absl::IDivDuration(d, absl::Nanoseconds(1), &d));
            return std::optional<std::string>();
          }
        });

    return std::move(parser_);
  }

  // Enable parsing environment variable.
  ParserBuilder& IncludeEnvironmentVariablesWithPrefix(
      const std::string& prefix) {
    if (parser_.environment_variable_prefix_) {
      std::cerr << "Redundant environment variable prefix '"
                << prefix << "'; already have '"
                << parser_.environment_variable_prefix_.value() << "'"
                << std::endl;
      std::exit(1);
    }
    parser_.environment_variable_prefix_ = prefix;
    return *this;
  }

 private:
  template <typename, typename = void>
  struct HasGetDescriptor : std::false_type {};

  template <typename T>
  struct HasGetDescriptor<
      T,
      std::void_t<decltype(std::declval<T>().GetDescriptor())>>
    : std::true_type {};

  template <typename T, typename F>
  bool TryOverloadParsing(F&& f) {
    static_assert(
        HasGetDescriptor<T>::value,
        "can only overload parsing of message types "
        "(not primitives or 'string')");

    const auto* descriptor = T().GetDescriptor();

    if (parser_.overload_parsing_.count(descriptor) == 0) {
      parser_.overload_parsing_[descriptor] =
          [f = std::forward<F>(f)](
              const std::string& value,
              google::protobuf::Message& message) {
            return f(
                value,
                *google::protobuf::DynamicCastToGenerated<T>(&message));
          };
      return true;
    } else {
      return false;
    }
  }

 private:
  Parser parser_;
};

////////////////////////////////////////////////////////////////////////

// Defined after 'ParserBuilder' to deal with circular dependency.
template <typename Flags>
ParserBuilder<Flags> Parser::Builder(Flags& flags, bool nested) {
  return ParserBuilder<Flags>(flags, nested);
}

////////////////////////////////////////////////////////////////////////

} // namespace stout::flags

////////////////////////////////////////////////////////////////////////
