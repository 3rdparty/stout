#include <gtest/gtest.h>

#include "stout/tests/utils.hpp"

TEST_F(TemporaryDirectoryTest, GetTemporaryDirectory) {
  ASSERT_TRUE(!test_directory_path().empty());
}
