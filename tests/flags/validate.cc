#include <array>

#include "gtest/gtest.h"
#include "stout/flags/flags.hpp"
#include "tests/flags/test.pb.h"

TEST(FlagsTest, Validate) {
  test::Flags flags;

  auto parser = stout::flags::Parser::Builder(&flags)
                    .Validate(
                        "'bar' must be true",
                        [](const auto& flags) {
                          return flags.bar();
                        })
                    .Validate(
                        "'baz' must be greater than 42",
                        [](const auto& flags) {
                          return flags.baz() > 42;
                        })
                    .Build();

  std::array arguments = {
      "/path/to/program",
      "--foo='hello world'",
      "--no-bar",
      "--baz=42",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  const std::string regex =
      "program: Failed while parsing and validating flags:"
      "\?\n\?\n"
      ". 'bar' must be true"
      "\?\n\?\n"
      ". 'baz' must be greater than 42";

  EXPECT_DEATH(
      parser.Parse(&argc, &argv),
      testing::ContainsRegex(regex));
}
