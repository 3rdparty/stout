#include <array>

#include "gtest/gtest.h"
#include "stout/flags/flags.h"
#include "tests/flags/test.pb.h"

TEST(FlagsTest, Help) {
  test::Flags flags;

  auto parser = stout::flags::Parser::Builder(&flags).Build();

  std::array arguments = {
      "/path/to/program",
      "--help",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  const std::string regex =
      "Usage: program \\[\\.\\.\\.\\]"
      "\?\n\?\n"
      "  --\\[no-\\]bar         help\?\n"
      "  --baz=\\.\\.\\.          help\?\n"
      "  --duration=\\.\\.\\.     help\?\n"
      "  --foo=\\.\\.\\.          help\?\n"
      "  --\\[no-\\]help        whether or not to display this help message";

  EXPECT_EXIT(
      parser.Parse(&argc, &argv),
      testing::ExitedWithCode(0),
      testing::ContainsRegex(regex));
}
