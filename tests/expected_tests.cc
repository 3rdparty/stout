#include "gtest/gtest.h"
#include "stout/expected.h"

stout::expected<int> divide(const int dividend, const int divisor) {
  if (divisor == 0) {
    return stout::make_unexpected("divide by zero");
  }

  return dividend / divisor;
}

TEST(Expected, SucceedResult) {
  const stout::expected<int> result = divide(6, 3);

  EXPECT_EQ(result, stout::expected<int>(2));
}

TEST(Expected, DivideByZero) {
  const stout::expected<int> result = divide(6, 0);

  EXPECT_EQ(result, stout::make_unexpected("divide by zero"));
}

TEST(Expected, MakeUnexpected) {
  const stout::unexpected<std::string> result =
      stout::make_unexpected("error");

  EXPECT_EQ("error", result.value());
}

TEST(Expected, Unexpected) {
  stout::unexpected<std::string> unexpected{"error"};

  EXPECT_EQ("error", unexpected.value());
}
