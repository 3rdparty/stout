#include <array>

#include "gtest/gtest.h"
#include "stout/flags/flags.h"
#include "tests/flags/test.pb.h"

// On Windows there is no `setenv()` or `unsetenv()`.
// Instead we use `_putenv_s()` function.
#ifdef _WIN32
int setenv(
    const char* env_name,
    const char* env_value,
    int replace) {
  return _putenv_s(env_name, env_value);
}

int unsetenv(const char* env_name) {
  return _putenv_s(env_name, "");
}
#endif

TEST(FlagsTest, EnvironmentVariableString) {
  test::Flags flags;

  auto parser = stout::flags::Parser::Builder(flags)
                    .IncludeEnvironmentVariablesWithPrefix("STOUT_FLAGS_TEST")
                    .Build();

  std::array arguments = {
      "program",
      "--bar",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  const char* env_name = "STOUT_FLAGS_TEST_FOO";
  const char* env_value = "'HELLO world'";
  int replace = 0;

  setenv(env_name, env_value, replace);
  parser.Parse(&argc, &argv);
  unsetenv(env_name);

  EXPECT_TRUE(flags.bar());
  EXPECT_EQ(1, argc);
  EXPECT_EQ("'HELLO world'", flags.foo());
  EXPECT_EQ("program", argv[0]);
}

TEST(FlagsTest, IncludeEnvironmentVariableWithUnderscoreFailure) {
  test::Flags flags;

  auto parser = stout::flags::Parser::Builder(flags)
                    .IncludeEnvironmentVariablesWithPrefix("STOUT_FLAGS_TEST_")
                    .Build();

  std::array arguments = {
      "program",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  const std::string regex =
      "program: Failed while parsing and validating flags:"
      "\?\n\?\n"
      ". Flag 'foo' not parsed but required";

  const char* env_name = "STOUT_FLAGS_TEST_FOO";
  const char* env_value = "'HELLO'";
  int replace = 0;

  setenv(env_name, env_value, replace);

  EXPECT_DEATH(
      parser.Parse(&argc, &argv),
      testing::ContainsRegex(regex));

  unsetenv(env_name);
}

TEST(FlagsTest, EnvironmentVariableWithNoUnderlineInNameFailure) {
  test::Flags flags;

  auto parser = stout::flags::Parser::Builder(flags)
                    .IncludeEnvironmentVariablesWithPrefix("STOUT_FLAGS_TEST")
                    .Build();

  std::array arguments = {
      "program",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  const std::string regex =
      "program: Failed while parsing and validating flags:"
      "\?\n\?\n"
      ". Flag 'foo' not parsed but required";

  const char* env_name = "STOUT_FLAGS_TESTFOO";
  const char* env_value = "'No underline'";
  int replace = 0;

  setenv(env_name, env_value, replace);
  EXPECT_DEATH(
      parser.Parse(&argc, &argv),
      testing::ContainsRegex(regex));
  unsetenv(env_name);
}

TEST(FlagsTest, EnvironmentVariableWith2Underscores) {
  test::Flags flags;

  auto parser = stout::flags::Parser::Builder(flags)
                    .IncludeEnvironmentVariablesWithPrefix("STOUT_FLAGS_TEST_")
                    .Build();

  std::array arguments = {
      "program",
      "--foo='hello'",
  };

  int argc = arguments.size();
  const char** argv = arguments.data();

  const char* env_name = "STOUT_FLAGS_TEST__S";
  const char* env_value = "'HELLO world'";
  int replace = 0;

  setenv(env_name, env_value, replace);
  parser.Parse(&argc, &argv);
  unsetenv(env_name);

  EXPECT_EQ(1, argc);
  EXPECT_EQ("'HELLO world'", flags._s());
  EXPECT_EQ("'hello'", flags.foo());
  EXPECT_EQ("program", argv[0]);
}
