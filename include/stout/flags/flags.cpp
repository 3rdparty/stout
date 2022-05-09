#include "stout/flags/flags.hpp"

#include "absl/strings/ascii.h"
#include "absl/strings/escaping.h"
#include "absl/strings/match.h"
#include "absl/strings/str_split.h"
#include "glog/logging.h"

////////////////////////////////////////////////////////////////////////

// We need this global variable for parsing environment varables.
// See 'Parser::Parse()' for more details.
extern char** environ;

////////////////////////////////////////////////////////////////////////

namespace stout::flags {

////////////////////////////////////////////////////////////////////////

void Parser::AddAllOrExit(google::protobuf::Message* message) {
  const auto* descriptor = message->GetDescriptor();

  for (int i = 0; i < descriptor->field_count(); i++) {
    const auto* field = descriptor->field(i);

    const auto& flag = field->options().GetExtension(stout::v1::flag);

    if (flag.names().empty()) {
      std::cerr
          << "Missing at least one flag name in 'names' for field '"
          << field->full_name() << "'"
          << std::endl;
      std::exit(1);
    }

    if (flag.help().empty()) {
      std::cerr
          << "Missing flag 'help' for field '" << field->full_name() << "'"
          << std::endl;
      std::exit(1);
    }

    for (const auto& name : flag.names()) {
      AddOrExit(name, field, message);
    }

    for (const auto& name : flag.deprecated_names()) {
      AddOrExit(name, field, message);
    }
  }
}

////////////////////////////////////////////////////////////////////////

void Parser::AddOrExit(
    const std::string& name,
    const google::protobuf::FieldDescriptor* field,
    google::protobuf::Message* message) {
  auto [_, inserted] = fields_.emplace(name, field);

  if (!inserted) {
    std::cerr
        << "Encountered duplicate flag name '" << name << "' "
        << "for field '" << field->full_name() << "'"
        << std::endl;
    std::exit(1);
  }

  messages_.emplace(field, message);
}

////////////////////////////////////////////////////////////////////////

void Parser::Parse(int* argc, const char*** argv) {
  // Grab the program name from argv, without removing it.
  program_name_ = *argc > 0
      ? std::filesystem::path((*argv)[0]).filename().string()
      : "";

  // Keep the arguments that are not being processed as flags.
  std::vector<const char*> args;

  std::multimap<std::string, std::optional<std::string>> values;

  // Read flags from the command line.
  for (int i = 1; i < *argc; i++) {
    std::string arg((*argv)[i]);

    // Strip both leading and trailing whitespace.
    absl::StripAsciiWhitespace(&arg);

    // Stop parsing flags after '--' is encountered.
    if (arg == "--") {
      // Save the rest of the arguments.
      for (int j = i + 1; j < *argc; j++) {
        args.push_back((*argv)[j]);
      }
      break;
    }

    // Skip anything that doesn't look like a flag.
    if (arg.find("--") != 0) {
      args.push_back((*argv)[i]);
      continue;
    }

    std::string name;
    std::optional<std::string> value;

    size_t eq = arg.find_first_of('=');
    if (eq == std::string::npos && arg.find("--no-") == 0) { // --no-name
      name = arg.substr(2);
    } else if (eq == std::string::npos) { // --name
      name = arg.substr(2);
    } else { // --name=value
      name = arg.substr(2, eq - 2);
      value = arg.substr(eq + 1);
    }

    values.emplace(name, value);
  }

  // Parse environment variables if environment_variable_prefix_
  // contains the specific prefix of variables we want to parse.
  if (environment_variable_prefix_) {
    // Run through all environment variables and select those which has
    // prefix (environment_variable_prefix_) in the name. Then we grab
    // correct name and the value and store this pair in `values`.
    // We use global variable `environ` to have access to the environment
    // variables to be able to select all necessary variables for
    // parsing according to the included prefix (see class `Parser`
    // in `stout/flags.h` which has `environment_variable_prefix_`
    // variable of type `std::optional<std::string>`).
    for (char** env = environ; *env != nullptr; ++env) {
      if (!absl::StrContains(*env, *environment_variable_prefix_)) {
        continue;
      }

      // By default all environment variables are as follows:
      //    name=value
      //  Hence we're splitting by '=' to correctly get the name and value.
      const std::vector<std::string> name_value =
          absl::StrSplit(*env, absl::MaxSplits('=', 1));

      CHECK_EQ(name_value.size(), 2u)
          << "Expecting all environment variables to have '=' delimiter";

      // Grab the name and the value.
      absl::string_view name{name_value[0]};
      std::string value{name_value[1]};

      if (absl::ConsumePrefix(
              &name,
              environment_variable_prefix_.value() + "_")) {
        // It's possible that users can set variables with upper cases.
        // So we should be sure that names we pass for parsing have
        // only lower-cases symbols.
        values.emplace(absl::AsciiStrToLower(name), value);
      }
    }
  }

  Parse(values);

  // Update 'argc' and 'argv' if we successfully loaded the flags.
  CHECK_LE(args.size(), (size_t) *argc);
  int i = 1; // Start at '1' to skip argv[0].
  for (const char* arg : args) {
    (*argv)[i++] = arg;
  }

  *argc = i;

  // Now null terminate the array. Note that we'll "leak" the
  // arguments that were processed here but it's not like they would
  // have gotten deleted in normal operations anyway.
  (*argv)[i++] = nullptr;
}

////////////////////////////////////////////////////////////////////////

void Parser::Parse(
    const std::multimap<std::string, std::optional<std::string>>& values) {
  // NOTE: using a set for errors to avoid duplicates which may
  // happen, e.g., when we have duplicate flags that are unknown or
  // duplicate boolean flags that conflict, etc.
  std::set<std::string> errors;

  for (const auto& [name, value] : values) {
    bool is_negated = absl::StartsWith(name, "no-");
    std::string non_negated_name = !is_negated ? name : name.substr(3);

    auto iterator = fields_.find(non_negated_name);

    if (iterator == fields_.end()) {
      errors.insert(
          "Encountered unknown flag '" + non_negated_name + "'"
          + (!is_negated ? "" : " via '" + name + "'"));
      continue;
    }

    const auto* field = iterator->second;

    // Need to normalize 'value' into protobuf text-format which
    // doesn't have a concept of "no-" prefix or non-empty booleans.
    std::optional<std::string> text;

    const bool boolean =
        field->type() == google::protobuf::FieldDescriptor::TYPE_BOOL;

    if (boolean) {
      if (!value) {
        text = is_negated ? "false" : "true";
      } else if (is_negated) {
        // Boolean flags with "no-" prefix must have an empty value.
        errors.insert(
            "Encountered negated boolean flag '" + name
            + "' with an unexpected value '" + value.value() + "'");
        continue;
      } else {
        text = value.value();
      }
    } else if (is_negated) {
      // Non-boolean flags cannot have "no-" prefix.
      errors.insert(
          "Failed to parse non-boolean flag '"
          + non_negated_name + "' via '" + name + "'");
      continue;
    } else if (!value.has_value() || value.value().empty()) {
      // Non-boolean flags must have a non-empty value.
      errors.insert(
          "Failed to parse non-boolean flag '" + non_negated_name
          + "': missing value");
      continue;
    } else {
      if (field->type() != google::protobuf::FieldDescriptor::TYPE_STRING) {
        text = value.value();
      } else {
        text = "'" + absl::CEscape(value.value()) + "'";
      }
    }

    CHECK(text);

    // Check if the field is a duplicate.
    const bool duplicate = parsed_.count(field) > 0;

    if (duplicate) {
      if (boolean) {
        // Only boolean flags can be duplicated and if/when they are
        // they can not conflict with one another.
        if (text != parsed_[field].text) {
          errors.insert(
              "Encountered duplicate boolean flag '" + non_negated_name + "' "
              + (parsed_[field].name != non_negated_name
                     ? "with flag aliased as '" + parsed_[field].name + "' "
                     : "")
              + "that has a conflicting value");
          continue;
        }
      } else {
        // Only boolean flags can be duplicated.
        errors.insert(
            "Encountered duplicate flag '" + non_negated_name + "'"
            + (parsed_[field].name != non_negated_name
                   ? " with flag aliased as '" + parsed_[field].name + "'"
                   : ""));
        continue;
      }
    }

    // Parse the value using an overloaded parser if provided.
    if (overload_parsing_.count(field->message_type()) > 0) {
      CHECK(messages_.count(field) != 0);
      auto* message = messages_[field];
      std::optional<std::string> error =
          overload_parsing_[field->message_type()](
              text.value(),
              message->GetReflection()->MutableMessage(message, field));

      if (error) {
        errors.insert(
            "Failed to parse flag '" + non_negated_name
            + "' from normalized value '" + text.value()
            + "' due to overloaded parsing error: " + error.value());
      } else {
        // Successfully parsed!
        parsed_[field] = {non_negated_name, text.value()};
      }
    } else {
      // Parse the value using an error collector that aggregates the
      // error for us to print out later.
      struct ErrorCollector : public google::protobuf::io::ErrorCollector {
        void AddError(
            int /* line */,
            int /* column */,
            const std::string& message) override {
          error += message;
        }

        void AddWarning(
            int line,
            int column,
            const std::string& message) override {
          // For now we treat all warnings as errors.
          AddError(line, column, message);
        }

        std::string error;
      } error_collector;

      google::protobuf::TextFormat::Parser text_format_parser;
      text_format_parser.RecordErrorsTo(&error_collector);
      if (!text_format_parser.ParseFieldValueFromString(
              text.value(),
              field,
              messages_[field])) {
        errors.insert(
            "Failed to parse flag '" + non_negated_name
            + "' from normalized value '" + text.value()
            + "' due to protobuf text-format parser error(s): "
            + error_collector.error);
      } else {
        // Successfully parsed!
        parsed_[field] = {non_negated_name, text.value()};
      }
    }
  }

  // Print out help if requested.
  if (standard_flags_->help()) {
    PrintHelp();
    std::exit(0);
  }

  // Ensure required flags are present.
  for (const auto& [_, field] : fields_) {
    const auto& flag = field->options().GetExtension(stout::v1::flag);
    if (flag.required() && parsed_.count(field) == 0) {
      CHECK(!flag.names().empty());
      std::string names;
      for (int i = 0; i < flag.names().size(); i++) {
        if (i == 1) {
          names += " (aka ";
        } else if (i > 1) {
          names += ", ";
        }
        names += "'" + flag.names().at(i) + "'";
      }
      if (flag.names().size() > 1) {
        names += ")";
      }
      errors.insert(
          "Flag " + names + " not parsed but required");
    }
  }

  // Perform validations.
  for (auto& [error, f] : validate_) {
    if (!f()) {
      errors.insert(error);
    }
  }

  if (!errors.empty()) {
    std::cerr
        << program_name_ << ": "
        << "Failed while parsing and validating flags:"
        << std::endl
        << std::endl;
    for (const auto& error : errors) {
      std::cerr << "* " << error
                << std::endl
                << std::endl;
    }
    std::exit(1);
  }
}

////////////////////////////////////////////////////////////////////////

void Parser::PrintHelp() {
  const int PAD = 5;

  std::string help = "Usage: " + program_name_ + " [...]\n\n";

  std::map<std::string, std::string> col1; // key -> col 1 string.

  // Construct string for the first column and store width of column.
  size_t width = 0;

  for (const auto& [_, field] : fields_) {
    const auto& flag = field->options().GetExtension(stout::v1::flag);

    const bool boolean =
        field->type() == google::protobuf::FieldDescriptor::TYPE_BOOL;

    std::string id;
    for (const std::string& name : flag.names()) {
      if (id == "") {
        id = name;
        col1[id] += " ";
      } else {
        col1[id] += ", ";
      }

      if (boolean) {
        col1[id] += " --[no-]" + name;
      } else {
        col1[id] += " --" + name + "=...";
      }
    }
    width = std::max(width, col1[id].size());
  }

  // TODO(benh): print the help on the next line instead of on the
  // same line as the names.
  for (const auto& [_, field] : fields_) {
    const auto& flag = field->options().GetExtension(stout::v1::flag);

    CHECK(!flag.names().empty());

    const std::string id = flag.names().at(0);

    std::string line = col1[id];

    std::string pad(PAD + width - line.size(), ' ');
    line += pad;

    size_t pos1 = 0, pos2 = 0;
    pos2 = flag.help().find_first_of("\n\r", pos1);
    line += flag.help().substr(pos1, pos2 - pos1) + "\n";
    help += line;

    while (pos2 != std::string::npos) { // Handle multi-line help strings.
      line = "";
      pos1 = pos2 + 1;
      std::string pad2(PAD + width, ' ');
      line += pad2;
      pos2 = flag.help().find_first_of("\n\r", pos1);
      line += flag.help().substr(pos1, pos2 - pos1) + "\n";
      help += line;
    }
  }

  std::cerr << help << std::endl;
}

////////////////////////////////////////////////////////////////////////

} // namespace stout::flags

////////////////////////////////////////////////////////////////////////
