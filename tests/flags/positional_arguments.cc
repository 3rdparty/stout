#include <array>

#include "gtest/gtest.h"
#include "stout/flags/flags.h"
#include "tests/flags/test.pb.h"

TEST(PositionalArguments, RenameSucceed) {
  test::Rename rename;

  auto parser = stout::flags::Parser::Builder(rename).Build();

  std::array arguments = {
      "rename",
      "foo.cc",
      "bar.cc",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  // We expect argc be equal to 1 cause all flags/pos args
  // which were parsed successfully should be consumed and
  // removed from 'argv'.
  EXPECT_EQ(1, argc);
  EXPECT_STREQ("rename", argv[0]);
  EXPECT_EQ("foo.cc", rename.cur_file_name());
  EXPECT_EQ("bar.cc", rename.new_file_name());
}

TEST(PositionalArguments, RenameFailOnMissingRequiredArgument) {
  test::Rename rename;

  auto parser = stout::flags::Parser::Builder(rename).Build();

  std::array arguments = {
      "rename",
      "bar.cc",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  EXPECT_DEATH(
      parser.Parse(&argc, &argv),
      "rename: Failed while parsing and validating flags:"
      "\?\n\?\n"
      ". Positional argument 'new_file' not parsed but required");
}

TEST(PositionalArguments, BuildFileSucceed) {
  test::ProcessFile msg;

  auto parser = stout::flags::Parser::Builder(msg).Build();

  std::array arguments = {
      "program",
      "build",
      "--debug",
      "foo.cc",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_EQ(1, argc);
  EXPECT_STREQ("program", argv[0]);
  EXPECT_FALSE(msg.has_rename());
  EXPECT_TRUE(msg.has_build());
  EXPECT_TRUE(msg.build().debug_mode());
  EXPECT_EQ("foo.cc", msg.build().file());
}

TEST(PositionalArguments, ProcessFileRenameSucceed) {
  test::ProcessFile msg;

  auto parser = stout::flags::Parser::Builder(msg).Build();

  std::array arguments = {
      "program",
      "rename",
      "foo.cc",
      "bar.cc",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_EQ(1, argc);
  EXPECT_STREQ("program", argv[0]);
  EXPECT_TRUE(msg.has_rename());
  EXPECT_FALSE(msg.has_build());
  EXPECT_EQ("foo.cc", msg.rename().cur_file_name());
  EXPECT_EQ("bar.cc", msg.rename().new_file_name());
}

TEST(PositionalArguments, RandomOrderFieldIndexes) {
  test::RandomOrderFieldIndexes msg;

  auto parser = stout::flags::Parser::Builder(msg).Build();

  std::array arguments = {
      "program",
      "first value",
      "second value",
      "third value",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_EQ(1, argc);
  EXPECT_STREQ("program", argv[0]);
  EXPECT_EQ("first value", msg.str1());
  EXPECT_EQ("second value", msg.str2());
  EXPECT_EQ("third value", msg.str3());
}

TEST(PositionalArguments, BuildFileFailOnMissingRequiredArgument) {
  test::ProcessFile msg;

  auto parser = stout::flags::Parser::Builder(msg).Build();

  std::array arguments = {
      "program",
      "build",
      "--debug=true",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  EXPECT_DEATH(
      parser.Parse(&argc, &argv),
      "build: Failed while parsing and validating flags:"
      "\?\n\?\n"
      ". Positional argument 'file_name' not parsed but required");
}

TEST(PositionalArguments, RedundantPositionalArguments) {
  test::ProcessFile msg;

  auto parser = stout::flags::Parser::Builder(msg).Build();

  std::array arguments = {
      "program",
      "build",
      "main.cc",
      "--debug=true",
      "45",
      "redundant",
      "true",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  EXPECT_DEATH(
      parser.Parse(&argc, &argv),
      "build: Failed while parsing and validating flags:"
      "\?\n\?\n"
      ". Encountered unknown flag '45'"
      "\?\n\?\n"
      ". Encountered unknown flag 'redundant'"
      "\?\n\?\n"
      ". Encountered unknown flag 'true'");
}

TEST(PositionalArguments, IllegalPositionalArgument1) {
  test::IllegalPositionalArg1 msg;

  EXPECT_DEATH(
      stout::flags::Parser::Builder(msg).Build(),
      "Field 'test.IllegalPositionalArg1.num' with 'stout::v1::argument'"
      " extension must have string type");
}

TEST(PositionalArguments, IllegalPositionalArgument2) {
  test::IllegalPositionalArg2 msg;

  EXPECT_DEATH(
      stout::flags::Parser::Builder(msg).Build(),
      "Missing name for field 'test.IllegalPositionalArg2.str'");
}

TEST(PositionalArguments, IllegalPositionalArgument3) {
  test::IllegalPositionalArg3 msg;

  EXPECT_DEATH(
      stout::flags::Parser::Builder(msg).Build(),
      "Missing 'help' for field 'test.IllegalPositionalArg3.str'");
}

TEST(PositionalArguments, MoreComplicatedCase1) {
  test::TopLevelArguments msg;

  auto parser = stout::flags::Parser::Builder(msg).Build();

  std::array arguments = {
      "program",
      "--top_level_flag1=42",
      "top_pos_arg1",
      "--top_level_flag2='ciao'",
      "C++",
      "sub1",
      "--sub1_flag=13",
      "hello world",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_EQ(1, argc);
  EXPECT_STREQ("program", argv[0]);
  EXPECT_FALSE(msg.has_sub2());
  EXPECT_TRUE(msg.has_sub1());
  EXPECT_EQ(42, msg.top_level_flag1());
  EXPECT_EQ("'ciao'", msg.top_level_flag2());
  EXPECT_EQ("top_pos_arg1", msg.top_pos_arg1());
  EXPECT_EQ("C++", msg.top_pos_arg2());
  EXPECT_EQ(13, msg.sub1().sub1_flag());
  EXPECT_EQ("hello world", msg.sub1().sub1_pos_arg());
}

TEST(PositionalArguments, MoreComplicatedCase2) {
  test::TopLevelArguments msg;

  auto parser = stout::flags::Parser::Builder(msg).Build();

  std::array arguments = {
      "program",
      "--top_level_flag1=100",
      "--top_level_flag2='ciao'",
      "some",
      "value",
      "sub2",
      "la vita e bella come caramella",
      "--sub2_flag=1994",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_EQ(1, argc);
  EXPECT_STREQ("program", argv[0]);
  EXPECT_FALSE(msg.has_sub1());
  EXPECT_TRUE(msg.has_sub2());
  EXPECT_EQ(100, msg.top_level_flag1());
  EXPECT_EQ("'ciao'", msg.top_level_flag2());
  EXPECT_EQ("some", msg.top_pos_arg1());
  EXPECT_EQ("value", msg.top_pos_arg2());
  EXPECT_EQ(1994, msg.sub2().sub2_flag());
  EXPECT_EQ(
      "la vita e bella come caramella",
      msg.sub2().sub2_pos_arg());
}
