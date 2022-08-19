#include <array>

#include "google/protobuf/util/time_util.h"
#include "gtest/gtest.h"
#include "stout/flags/flags.h"
#include "tests/flags/test_default_values.pb.h"

TEST(FlagsTest, FlagsWithDefaultValues) {
  test::default_values::Flags flags;

  auto parser = stout::flags::Parser::Builder(flags).Build();

  std::array arguments = {
      "/path/to/program",
      "no-bam",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_EQ(1, argc);
  EXPECT_STREQ("/path/to/program", argv[0]);
  EXPECT_EQ("'some default value'", flags.str1());
  EXPECT_EQ("some default value", flags.str2());
  EXPECT_EQ("\"some default value\"", flags.str3());
  EXPECT_TRUE(flags.bar());
  EXPECT_EQ(1994, flags.baz());
  EXPECT_FALSE(flags.bam());
  EXPECT_EQ(
      std::chrono::nanoseconds(
          ::google::protobuf::util::TimeUtil::DurationToNanoseconds(
              flags.duration())),
      std::chrono::nanoseconds(
          std::chrono::seconds(42)));
}

TEST(FlagsTest, PositionalArgsWithDefaultValues) {
  test::default_values::PositionalArgs args;

  auto parser = stout::flags::Parser::Builder(args).Build();

  std::array arguments = {
      "/path/to/program",
      "13",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_EQ(1, argc);
  EXPECT_STREQ("/path/to/program", argv[0]);
  EXPECT_EQ("13", args.arg1());
  EXPECT_EQ("value2", args.arg2());
  EXPECT_EQ("value3", args.arg3());
}

TEST(FlagsTest, IllegalSetup) {
  test::default_values::IllegalSetup flags;

  EXPECT_DEATH(
      stout::flags::Parser::Builder(flags).Build(),
      "Error: you can't have default value for required field "
      "'test.default_values.IllegalSetup.flag1'");
}

TEST(FlagsTest, IncorrectTypeOfDefaultValue) {
  test::default_values::IncorrectTypeOfDefaultValue flags;

  auto parser = stout::flags::Parser::Builder(flags).Build();

  std::array arguments = {
      "/path/to/program",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  const std::string regex =
      "program: Failed while parsing and validating flags:"
      "\?\n\?\n"
      ". Failed to parse flag 'flag1' from normalized value"
      " 'string' due to protobuf text-format parser error.s.:"
      " Expected integer, got: string";

  EXPECT_DEATH(
      parser.Parse(&argc, &argv),
      testing::ContainsRegex(regex));
}

TEST(FlagsTest, PositionalArgumentWithoutDefault) {
  test::default_values::PositionalArgumentWithoutDefault args;

  auto parser = stout::flags::Parser::Builder(args).Build();

  std::array arguments = {
      "/path/to/program",
      "1000",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  const std::string regex =
      "program: Failed while parsing "
      "and validating flags:"
      "\?\n\?\n"
      ". Positional argument 'arg2' not parsed but required";

  EXPECT_DEATH(
      parser.Parse(&argc, &argv),
      testing::ContainsRegex(regex));
}

TEST(FlagsTest, MoreComplicatedCaseForFlagsAndPosArgsSucceed1) {
  test::default_values::TopLevelFlagsAndPositionalArgs flags;

  auto parser = stout::flags::Parser::Builder(flags).Build();

  std::array arguments = {
      "/path/to/program",
      "--top_level_flag1=1000",
      "some value",
      "sub1",
      "Ben",
      "--sub1_flag=13",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_EQ(1, argc);
  EXPECT_STREQ("/path/to/program", argv[0]);
  EXPECT_TRUE(flags.has_sub1());
  EXPECT_FALSE(flags.has_sub2());
  EXPECT_EQ(1000, flags.top_level_flag1());
  EXPECT_EQ("value", flags.top_level_flag2());
  EXPECT_EQ("some value", flags.top_pos_arg1());
  EXPECT_EQ("world", flags.top_pos_arg2());
  EXPECT_EQ(13, flags.sub1().sub1_flag());
  EXPECT_EQ("Ben", flags.sub1().sub1_pos_arg());
}

TEST(FlagsTest, MoreComplicatedCaseForFlagsAndPosArgsSucceed2) {
  test::default_values::TopLevelFlagsAndPositionalArgs flags;

  auto parser = stout::flags::Parser::Builder(flags).Build();

  std::array arguments = {
      "/path/to/program",
      "--top_level_flag1=1993",
      "--top_level_flag2=1994",
      "sub2",
      "--sub2_flag=2022",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_EQ(1, argc);
  EXPECT_STREQ("/path/to/program", argv[0]);
  EXPECT_TRUE(flags.has_sub2());
  EXPECT_FALSE(flags.has_sub1());
  EXPECT_EQ(1993, flags.top_level_flag1());
  EXPECT_EQ("1994", flags.top_level_flag2());
  EXPECT_EQ("hello", flags.top_pos_arg1());
  EXPECT_EQ("world", flags.top_pos_arg2());
  EXPECT_EQ(2022, flags.sub2().sub2_flag());
  EXPECT_EQ("ciao", flags.sub2().sub2_pos_arg());
}
