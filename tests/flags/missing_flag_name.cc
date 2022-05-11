#include "gtest/gtest.h"
#include "stout/flags/flags.h"
#include "tests/flags/test.pb.h"

TEST(FlagsTest, MissingFlagName) {
  EXPECT_DEATH(
      []() {
        test::MissingFlagName missing_flag_name;
        auto builder = stout::flags::Parser::Builder(&missing_flag_name);
        builder.Build();
      }(),
      "Missing at least one flag name in 'names' for "
      "field 'test.MissingFlagName.s'");
}

TEST(FlagsTest, MissingFlagHelp) {
  EXPECT_DEATH(
      []() {
        test::MissingFlagHelp missing_flag_help;
        auto builder = stout::flags::Parser::Builder(&missing_flag_help);
        builder.Build();
      }(),
      "Missing flag 'help' for "
      "field 'test.MissingFlagHelp.s'");
}
