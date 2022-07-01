#include <array>

#include "gtest/gtest.h"
#include "stout/flags/flags.h"
#include "tests/flags/test_subcommands.pb.h"

TEST(SubcommandsTest, MissingSubcommandName) {
  EXPECT_DEATH(
      []() {
        test::FlagsWithSubcommandMissingName flags;
        auto builder = stout::flags::Parser::Builder(flags);
        builder.Build();
      }(),
      "Missing at least one subcommand name in 'names' for "
      "field 'test.FlagsWithSubcommandMissingName.build'");
}

TEST(SubcommandsTest, MissingSubcommandHelp) {
  EXPECT_DEATH(
      []() {
        test::FlagsWithSubcommandMissingHelp flags;
        auto builder = stout::flags::Parser::Builder(flags);
        builder.Build();
      }(),
      "Missing subcommand 'help' for field "
      "'test.FlagsWithSubcommandMissingHelp.info_subcommand'");
}


TEST(SubcommandsTest, MissingSubcommandExtension) {
  EXPECT_DEATH(
      []() {
        test::SubcommandFlagsWithoutExtension flag;
        auto builder = stout::flags::Parser::Builder(flag);
        builder.Build();
      }(),
      "Every field of the 'oneof subcommand' must be"
      " annotated with a stout.v1.subcommand option");
}

TEST(SubcommandsTest, IncorrectSubcommandExtension) {
  EXPECT_DEATH(
      []() {
        test::FlagsWithIncorrectExtension flag;
        auto builder = stout::flags::Parser::Builder(flag);
        builder.Build();
      }(),
      "stout.v1.subcommand option should be annotated on fields"
      " that are only inside 'oneof subcommand'");
}

TEST(SubcommandsTest, IncorrectOneofName) {
  EXPECT_DEATH(
      []() {
        test::IncorrectOneofName flag;
        auto builder = stout::flags::Parser::Builder(flag);
        builder.Build();
      }(),
      "'oneof' field must have 'subcommand' name. "
      "Other names are illegal");
}

TEST(SubcommandsTest, SubcommandFlagExtension) {
  EXPECT_DEATH(
      []() {
        test::SubcommandFlagExtension flag;
        auto builder = stout::flags::Parser::Builder(flag);
        builder.Build();
      }(),
      "Every field of the 'oneof subcommand' must be"
      " annotated with a stout.v1.subcommand option");
}

TEST(SubcommandsTest, DuplicateSubcommandFields) {
  EXPECT_DEATH(
      []() {
        test::DuplicateSubcommandFields flag;
        auto builder = stout::flags::Parser::Builder(flag);
        builder.Build();
      }(),
      "Encountered duplicate subcommand name 'build'"
      " for message 'test.DuplicateSubcommandFields'");
}

TEST(SubcommandsTest, SubcommandAndUnknownArguments) {
  test::SimpleSubcommandSucceed flags;

  auto parser = stout::flags::Parser::Builder(flags).Build();

  std::array arguments = {
      "program",
      "--b",
      "build",
      "--other_flag=13",
      "--",
      "some",
      "unknown",
      "arguments",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_TRUE(flags.has_build());
  EXPECT_EQ(4, argc);
  EXPECT_STREQ("program", argv[0]);
  EXPECT_STREQ("some", argv[1]);
  EXPECT_STREQ("unknown", argv[2]);
  EXPECT_STREQ("arguments", argv[3]);
  EXPECT_TRUE(flags.b());
  EXPECT_EQ(13, flags.build().other_flag());
}

TEST(SubcommandsTest, SimpleSubcommandBuildSucceed) {
  test::SimpleSubcommandSucceed flags;

  auto parser = stout::flags::Parser::Builder(flags).Build();

  std::array arguments = {
      "program",
      "--b",
      "build",
      "--other_flag=13",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_TRUE(flags.has_build());
  EXPECT_EQ(1, argc);
  EXPECT_STREQ("program", argv[0]);
  EXPECT_TRUE(flags.b());
  EXPECT_EQ(13, flags.build().other_flag());
}

TEST(SubcommandsTest, SimpleSubcommandInfoSucceed) {
  test::SimpleSubcommandSucceed flags;

  auto parser = stout::flags::Parser::Builder(flags).Build();

  std::array arguments = {
      "program",
      "--b",
      "info_subcommand",
      "--info=hello world",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_TRUE(flags.has_info_subcommand());
  EXPECT_EQ(1, argc);
  EXPECT_STREQ("program", argv[0]);
  EXPECT_TRUE(flags.b());
  EXPECT_EQ("hello world", flags.info_subcommand().info());
}

TEST(SubcommandsTest, DuplicateSubcommands) {
  test::SimpleSubcommandSucceed flags;

  auto parser = stout::flags::Parser::Builder(flags).Build();

  std::array arguments = {
      "program",
      "--b",
      "info_subcommand",
      "--info=hello world",
      "info_subcommand",
      "--info=oops",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  EXPECT_DEATH(
      parser.Parse(&argc, &argv),
      ". Encountered duplicate flag 'info'"
      "\?\n\?\n"
      ". Encountered unknown flag 'info_subcommand'");
}

TEST(SubcommandsTest, DuplicateSubcommandFlags) {
  test::SimpleSubcommandSucceed flags;

  auto parser = stout::flags::Parser::Builder(flags).Build();

  std::array arguments = {
      "program",
      "--b",
      "build",
      "--other_flag=1",
      "--other_flag=2",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  EXPECT_DEATH(
      parser.Parse(&argc, &argv),
      "Encountered duplicate flag 'other_flag'");
}

TEST(SubcommandsTest, DuplicateSubcommandFlagNameForEnclosingLevel) {
  test::DuplicateEnclosingFlagName flags;

  auto parser = stout::flags::Parser::Builder(flags).Build();

  std::array arguments = {
      "program",
      "--other_flag=1",
      "build",
      "--other_flag=2",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_TRUE(flags.has_build());
  EXPECT_EQ(1, argc);
  EXPECT_STREQ("program", argv[0]);
  EXPECT_EQ(1, flags.other_flag());
  EXPECT_EQ(2, flags.build().other_flag());
}

TEST(SubcommandsTest, SubcommandFailSettingTwoOneofFlagsAtOnce) {
  test::SimpleSubcommandSucceed flags;

  auto parser = stout::flags::Parser::Builder(flags).Build();

  std::array arguments = {
      "program",
      "--b",
      "build",
      "--other_flag=1",
      "info_subcommand",
      "--info=hello",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  EXPECT_DEATH(
      parser.Parse(&argc, &argv),
      ". Encountered unknown flag 'info'"
      "\?\n\?\n"
      ". Encountered unknown flag 'info_subcommand'");
}

TEST(SubcommandsTest, ComplicatedSubcommandSucceed1) {
  test::ComplicatedSubcommandMessage flags;

  auto parser = stout::flags::Parser::Builder(flags).Build();

  std::array arguments = {
      "program",
      "--flag=hello world",
      "--other=Ben",
      "sub1",
      "--another=Artur",
      "--num=13",
      "build",
      "--other_flag=1",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_TRUE(flags.has_sub1());
  EXPECT_TRUE(flags.sub1().has_build());
  EXPECT_FALSE(flags.has_sub2());
  EXPECT_FALSE(flags.sub1().has_info_subcommand());
  EXPECT_EQ(1, argc);
  EXPECT_STREQ("program", argv[0]);
  EXPECT_EQ("hello world", flags.flag());
  EXPECT_EQ("Ben", flags.other());
  EXPECT_EQ("Artur", flags.sub1().another());
  EXPECT_EQ(13, flags.sub1().num());
  EXPECT_EQ(1, flags.sub1().build().other_flag());
}

TEST(SubcommandsTest, ComplicatedSubcommandSucceed2) {
  test::ComplicatedSubcommandMessage flags;

  auto parser = stout::flags::Parser::Builder(flags).Build();

  std::array arguments = {
      "program",
      "--flag=hello world",
      "--other=Ben",
      "sub1",
      "--another=Artur",
      "--num=13",
      "info_subcommand",
      "--info=ciao",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_TRUE(flags.has_sub1());
  EXPECT_FALSE(flags.has_sub2());
  EXPECT_FALSE(flags.sub1().has_build());
  EXPECT_TRUE(flags.sub1().has_info_subcommand());
  EXPECT_EQ(1, argc);
  EXPECT_STREQ("program", argv[0]);
  EXPECT_EQ("hello world", flags.flag());
  EXPECT_EQ("Ben", flags.other());
  EXPECT_EQ("Artur", flags.sub1().another());
  EXPECT_EQ(13, flags.sub1().num());
  EXPECT_EQ("ciao", flags.sub1().info_subcommand().info());
}

TEST(SubcommandsTest, ComplicatedSubcommandSucceed3) {
  test::ComplicatedSubcommandMessage flags;

  auto parser = stout::flags::Parser::Builder(flags).Build();

  std::array arguments = {
      "program",
      "--flag=hello world",
      "--other=Ben",
      "sub2",
      "--s=Artur",
      "info_subcommand",
      "--info=some info",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_TRUE(flags.has_sub2());
  EXPECT_FALSE(flags.has_sub1());
  EXPECT_FALSE(flags.sub2().has_build());
  EXPECT_TRUE(flags.sub2().has_info_subcommand());
  EXPECT_EQ(1, argc);
  EXPECT_STREQ("program", argv[0]);
  EXPECT_EQ("hello world", flags.flag());
  EXPECT_EQ("Ben", flags.other());
  EXPECT_EQ("Artur", flags.sub2().s());
  EXPECT_EQ("some info", flags.sub2().info_subcommand().info());
}

TEST(SubcommandsTest, ComplicatedSubcommandSucceed4) {
  test::ComplicatedSubcommandMessage flags;

  auto parser = stout::flags::Parser::Builder(flags).Build();

  std::array arguments = {
      "program",
      "--flag=hello world",
      "--other=Ben",
      "sub2",
      "--s=Artur",
      "build",
      "--other_flag=13",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  parser.Parse(&argc, &argv);

  EXPECT_TRUE(flags.has_sub2());
  EXPECT_FALSE(flags.has_sub1());
  EXPECT_TRUE(flags.sub2().has_build());
  EXPECT_FALSE(flags.sub2().has_info_subcommand());
  EXPECT_EQ(1, argc);
  EXPECT_STREQ("program", argv[0]);
  EXPECT_EQ("hello world", flags.flag());
  EXPECT_EQ("Ben", flags.other());
  EXPECT_EQ("Artur", flags.sub2().s());
  EXPECT_EQ(13, flags.sub2().build().other_flag());
}
