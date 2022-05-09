#include <array>

#include "gtest/gtest.h"
#include "stout/flags/flags.hpp"
#include "tests/flags/test.pb.h"

TEST(FlagsTest, ParseRequired) {
  test::Flags flags;

  auto parser = stout::flags::Parser::Builder(&flags).Build();

  std::array arguments = {
      "/path/to/program",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  const std::string regex =
      "program: Failed while parsing "
      "and validating flags:"
      "\?\n\?\n"
      ". Flag 'foo' not parsed but required";

  EXPECT_DEATH(
      parser.Parse(&argc, &argv),
      testing::ContainsRegex(regex));
}

TEST(FlagsTest, ParseStringWithSingleQuotes) {
  test::Flags flags;

  auto parser = stout::flags::Parser::Builder(&flags).Build();

  std::array arguments = {
      "/path/to/program",
      "--foo='hello world'",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_EQ("'hello world'", flags.foo());
}

TEST(FlagsTest, ParseStringWithDoubleQuotes) {
  test::Flags flags;

  auto parser = stout::flags::Parser::Builder(&flags).Build();

  std::array arguments = {
      "/path/to/program",
      "--foo=\"hello world\"",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_EQ("\"hello world\"", flags.foo());
}

TEST(FlagsTest, ParseStringWithoutSingleOrDoubleQuotes) {
  test::Flags flags;

  auto parser = stout::flags::Parser::Builder(&flags).Build();

  std::array arguments = {
      "/path/to/program",
      "--foo=hello",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_EQ("hello", flags.foo());
}

TEST(FlagsTest, ParseSpaceStringWithoutAnyQuotes) {
  test::Flags flags;

  auto parser = stout::flags::Parser::Builder(&flags).Build();

  std::array arguments = {
      "/path/to/program",
      "--foo=hello world",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_EQ("hello world", flags.foo());
}

TEST(FlagsTest, ParseImplicitBoolean) {
  test::Flags flags;

  auto parser = stout::flags::Parser::Builder(&flags).Build();

  std::array arguments = {
      "/path/to/program",
      "--foo='hello world'",
      "--bar",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_TRUE(flags.bar());
}

TEST(FlagsTest, ParseExplicitBoolean) {
  test::Flags flags;

  auto parser = stout::flags::Parser::Builder(&flags).Build();

  std::array arguments = {
      "/path/to/program",
      "--foo='hello world'",
      "--bar=true",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_TRUE(flags.bar());
}

TEST(FlagsTest, ParseNegatedBoolean) {
  test::Flags flags;

  auto parser = stout::flags::Parser::Builder(&flags).Build();

  std::array arguments = {
      "/path/to/program",
      "--foo='hello world'",
      "--no-bar",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_FALSE(flags.bar());
}

TEST(FlagsTest, ModifiedArgcArgv) {
  test::Flags flags;

  auto parser = stout::flags::Parser::Builder(&flags).Build();

  std::array arguments = {
      "/path/to/program",
      "--foo='hello world'",
      "one",
      "--bar",
      "two",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_TRUE(flags.bar());
  EXPECT_EQ("'hello world'", flags.foo());

  EXPECT_EQ(3, argc);

  EXPECT_STREQ("/path/to/program", argv[0]);
  EXPECT_STREQ("one", argv[1]);
  EXPECT_STREQ("two", argv[2]);
}

TEST(FlagsTest, UnknownNonNegatedFlag) {
  test::Flags flags;

  auto parser = stout::flags::Parser::Builder(&flags).Build();

  std::array arguments = {
      "/path/to/program",
      "--foo='hello world'",
      "--unknown",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  const std::string regex =
      "program: Failed while parsing "
      "and validating flags:"
      "\?\n\?\n"
      ". Encountered unknown flag 'unknown'";

  EXPECT_DEATH(
      parser.Parse(&argc, &argv),
      testing::ContainsRegex(regex));
}

TEST(FlagsTest, UnknownNegatedFlag) {
  test::Flags flags;

  auto parser = stout::flags::Parser::Builder(&flags).Build();

  std::array arguments = {
      "/path/to/program",
      "--foo='hello world'",
      "--no-unknown",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  const std::string regex =
      "program: Failed while parsing "
      "and validating flags:"
      "\?\n\?\n"
      ". Encountered unknown flag 'unknown' via 'no-unknown'";

  EXPECT_DEATH(
      parser.Parse(&argc, &argv),
      testing::ContainsRegex(regex));
}

TEST(FlagsTest, IncorrectNegatedBooleanFlag) {
  test::Flags flags;

  auto parser = stout::flags::Parser::Builder(&flags).Build();

  std::array arguments = {
      "/path/to/program",
      "--foo='hello world'",
      "--no-bar=some_value",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  const std::string regex =
      "program: Failed while parsing "
      "and validating flags:"
      "\?\n\?\n"
      ". Encountered negated boolean flag 'no-bar'"
      " with an unexpected value 'some_value'";

  EXPECT_DEATH(
      parser.Parse(&argc, &argv),
      testing::ContainsRegex(regex));
}

TEST(FlagsTest, NonBooleanFlagWithPrefix) {
  test::Flags flags;

  auto parser = stout::flags::Parser::Builder(&flags).Build();

  std::array arguments = {
      "/path/to/program",
      "--foo='hello world'",
      "--no-baz=some_value",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  const std::string regex =
      "program: Failed while parsing "
      "and validating flags:"
      "\?\n\?\n"
      ". Failed to parse non-boolean flag 'baz'"
      " via 'no-baz'";

  EXPECT_DEATH(
      parser.Parse(&argc, &argv),
      testing::ContainsRegex(regex));
}

TEST(FlagsTest, NonBooleanFlagWithEmptyValue) {
  test::Flags flags;

  auto parser = stout::flags::Parser::Builder(&flags).Build();

  std::array arguments = {
      "/path/to/program",
      "--foo='hello world'",
      "--baz=",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  const std::string regex =
      "program: Failed while parsing "
      "and validating flags:"
      "\?\n\?\n"
      ". Failed to parse non-boolean flag 'baz':"
      " missing value";

  EXPECT_DEATH(
      parser.Parse(&argc, &argv),
      testing::ContainsRegex(regex));
}

TEST(FlagsTest, ProtobufTextFormatParserError) {
  test::Flags flags;

  auto parser = stout::flags::Parser::Builder(&flags).Build();

  std::array arguments = {
      "/path/to/program",
      "--foo='hello'",
      "--baz=s",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  const std::string regex =
      "program: Failed while parsing "
      "and validating flags:"
      "\?\n\?\n"
      ". Failed to parse flag 'baz' "
      "from normalized value 's' "
      "due to protobuf text-format parser error.s.: "
      "Expected integer, got: s";

  EXPECT_DEATH(
      parser.Parse(&argc, &argv),
      testing::ContainsRegex(regex));
}
