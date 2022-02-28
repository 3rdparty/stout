#include <gtest/gtest.h>

#include "stout/stringify.hpp"

#ifdef _WIN32
TEST(StringifyTest, WStringToString) {
  EXPECT_EQ(std::string{}, stringify(std::wstring{}));
  EXPECT_EQ(std::string{"hello"}, stringify(std::wstring{L"hello"}));
}

TEST(StringifyTest, StringToWstring) {
  EXPECT_EQ(std::wstring{}, wide_stringify(std::string{}));
  EXPECT_EQ(std::wstring{L"hello"}, wide_stringify(std::string{"hello"}));
}
#endif

TEST(StringifyTest, StringifyInt) {
  EXPECT_EQ(std::string{"123"}, stringify(123));
}

TEST(StringifyTest, StringifyString) {
  EXPECT_EQ(std::string{"hello"}, stringify(std::string{"hello"}));
}

TEST(StringifyTest, StringifyBool) {
  EXPECT_EQ(std::string{"true"}, stringify(true));
  EXPECT_EQ(std::string{"false"}, stringify(false));
}

TEST(StringifyTest, StringifySet) {
  EXPECT_EQ(
      std::string{"{ 1, 2, 3, 4, 5 }"},
      stringify(std::set<int>{1, 2, 3, 4, 5}));
}

TEST(StringifyTest, StringifyList) {
  EXPECT_EQ(
      std::string{"[ 1, 2, 3, 4, 5 ]"},
      stringify(std::list<int>{1, 2, 3, 4, 5}));
}

TEST(StringifyTest, StringifyVector) {
  EXPECT_EQ(
      std::string{"[ Ben, RJ, Gorm, Alex, Riley ]"},
      stringify(std::vector<std::string>{
          "Ben",
          "RJ",
          "Gorm",
          "Alex",
          "Riley"}));
}

TEST(StringifyTest, StringifyMap) {
  EXPECT_EQ(
      std::string{"{ buona sera: good evening, ciao: hi, grazie: thanks }"},
      stringify(std::map<std::string, std::string>{
          {"ciao", "hi"},
          {"buona sera", "good evening"},
          {"grazie", "thanks"}}));
}

TEST(StringifyTest, StringifyHashSet) {
  EXPECT_EQ(
      std::string{"{ 1 }"},
      stringify(hashset<int>{1}));
}

TEST(StringifyTest, StringifyHashMap) {
  EXPECT_EQ(
      std::string{"{ RAM: 100 }"},
      stringify(hashmap<std::string, int>{{"RAM", 100}}));
}

TEST(StringifyTest, StringifyError) {
  EXPECT_EQ(
      std::string{"Failed to ..."},
      stringify(Error{"Failed to ..."}));
}
