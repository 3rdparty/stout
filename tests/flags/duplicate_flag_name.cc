#include <array>

#include "gtest/gtest.h"
#include "stout/flags/flags.h"
#include "tests/flags/test.pb.h"

TEST(FlagsTest, DuplicateFlagName) {
  EXPECT_DEATH(
      []() {
        test::DuplicateFlagName duplicate_flag_name;
        auto builder = stout::flags::Parser::Builder(&duplicate_flag_name);
        builder.Build();
      }(),
      "Encountered duplicate flag name 'same' for "
      "field 'test.DuplicateFlagName.s2'");
}

TEST(FlagsTest, DuplicateSucceed) {
  test::Flags flags;

  auto parser = stout::flags::Parser::Builder(&flags).Build();

  std::array arguments = {
      "/path/to/program",
      "--foo='hello world'",
      "--bar=true",
      "--bar=true",
      "--bar",
      "--bar",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_TRUE(flags.bar());
}

TEST(FlagsTest, DuplicateConflict) {
  test::DuplicateFlagsDeath duplicates;

  auto parser = stout::flags::Parser::Builder(&duplicates).Build();

  std::array arguments = {
      "/path/to/program",
      "--duplicate",
      "--other=false",
      "--no-b",
      "--b",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  const std::string regex =
      "program: Failed while parsing "
      "and validating flags:"
      "\?\n\?\n"
      ". Encountered duplicate boolean flag 'b' "
      "that has a conflicting value"
      "\?\n\?\n"
      ". Encountered duplicate boolean flag 'other' "
      "with flag aliased as 'duplicate' "
      "that has a conflicting value";

  EXPECT_DEATH(
      parser.Parse(&argc, &argv),
      testing::ContainsRegex(regex));
}

TEST(FlagsTest, DuplicateNonBooleanFlags) {
  test::DuplicateFlagsDeath duplicates;

  auto parser = stout::flags::Parser::Builder(&duplicates).Build();

  std::array arguments = {
      "/path/to/program",
      "--s='hello'",
      "--s='world'",
      "--ss='hey'",
      "--ss_alias='Ben'",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  const std::string regex =
      "program: Failed while parsing "
      "and validating flags:"
      "\?\n\?\n"
      ". Encountered duplicate flag 's'"
      "\?\n\?\n"
      ". Encountered duplicate flag 'ss_alias' "
      "with flag aliased as 'ss'";

  EXPECT_DEATH(
      parser.Parse(&argc, &argv),
      testing::ContainsRegex(regex));
}
