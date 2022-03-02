// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "stout/os/exists.hpp"
#include "stout/os/getcwd.hpp"
#include "stout/os/int_fd.hpp"
#include "stout/os/open.hpp"
#include "stout/os/rm.hpp"
#include "stout/os/touch.hpp"
#include "stout/path.hpp"
#include "stout/tests/utils.hpp"

using std::string;
using std::vector;


// Test many corner cases of Path::basename.
TEST(PathTest, Basename) {
  // Empty path check.
  EXPECT_EQ(".", Path("").basename());

  // Check common path patterns.
#ifdef _WIN32
  EXPECT_EQ("\\", Path("\\").basename());
#else
  EXPECT_EQ("/", Path("/").basename());
#endif // _WIN32
  EXPECT_EQ(".", Path(".").basename());
  EXPECT_EQ("..", Path("..").basename());

  EXPECT_EQ("a", Path("a").basename());
#ifdef _WIN32
  EXPECT_EQ("b", Path("a\\b").basename());
  EXPECT_EQ("c", Path("a\\b\\c").basename());
#else
  EXPECT_EQ("b", Path("a/b").basename());
  EXPECT_EQ("c", Path("a/b/c").basename());
#endif // _WIN32

  // Check leading slashes get cleaned up properly.
#ifdef _WIN32
  EXPECT_EQ("a", Path("\\a").basename());
  EXPECT_EQ("a", Path("\\\\a").basename());
  EXPECT_EQ("a", Path("\\a\\").basename());
  EXPECT_EQ("c", Path("\\a\\b\\c").basename());
  EXPECT_EQ("b", Path("\\a\\b").basename());
  EXPECT_EQ("b", Path("\\\\a\\\\b").basename());
#else
  EXPECT_EQ("a", Path("/a").basename());
  EXPECT_EQ("a", Path("//a").basename());
  EXPECT_EQ("a", Path("/a/").basename());
  EXPECT_EQ("c", Path("/a/b/c").basename());
  EXPECT_EQ("b", Path("/a/b").basename());
  EXPECT_EQ("b", Path("//a//b").basename());
#endif // _WIN32

  // Check trailing slashes get cleaned up properly.
#ifdef _WIN32
  EXPECT_EQ("a", Path("a\\").basename());
  EXPECT_EQ("c", Path("\\a\\b\\c\\\\").basename());
  EXPECT_EQ("c", Path("\\a\\b\\c\\\\\\").basename());
  EXPECT_EQ("\\", Path("\\\\").basename());
  EXPECT_EQ("\\", Path("\\\\\\").basename());
#else
  EXPECT_EQ("a", Path("a/").basename());
  EXPECT_EQ("c", Path("/a/b/c//").basename());
  EXPECT_EQ("c", Path("/a/b/c///").basename());
  EXPECT_EQ("/", Path("//").basename());
  EXPECT_EQ("/", Path("///").basename());
#endif // _WIN32
}


// Test many corner cases of Path::dirname.
TEST(PathTest, Dirname) {
  // Empty path check.
  EXPECT_EQ(".", Path("").dirname());

  // Check common path patterns.
#ifdef _WIN32
  EXPECT_EQ("\\", Path("\\").dirname());
#else
  EXPECT_EQ("/", Path("/").dirname());
#endif // _WIN32
  EXPECT_EQ(".", Path(".").dirname());
  EXPECT_EQ(".", Path("..").dirname());

  EXPECT_EQ(".", Path("a").dirname());
#ifdef _WIN32
  EXPECT_EQ("a", Path("a\\b").dirname());
  EXPECT_EQ("a\\b", Path("a\\b\\c\\").dirname());
#else
  EXPECT_EQ("a", Path("a/b").dirname());
  EXPECT_EQ("a/b", Path("a/b/c/").dirname());
#endif // _WIN32

  // Check leading slashes get cleaned up properly.
#ifdef _WIN32
  EXPECT_EQ("\\", Path("\\a").dirname());
  EXPECT_EQ("\\", Path("\\\\a").dirname());
  EXPECT_EQ("\\", Path("\\a\\").dirname());
  EXPECT_EQ("\\a", Path("\\a\\b").dirname());
  EXPECT_EQ("\\\\a", Path("\\\\a\\\\b").dirname());
  EXPECT_EQ("\\a\\b", Path("\\a\\b\\c").dirname());
#else
  EXPECT_EQ("/", Path("/a").dirname());
  EXPECT_EQ("/", Path("//a").dirname());
  EXPECT_EQ("/", Path("/a/").dirname());
  EXPECT_EQ("/a", Path("/a/b").dirname());
  EXPECT_EQ("//a", Path("//a//b").dirname());
  EXPECT_EQ("/a/b", Path("/a/b/c").dirname());
#endif // _WIN32

  // Check intermittent slashes get handled just like ::dirname does.
#ifdef _WIN32
  EXPECT_EQ("\\a\\\\b", Path("\\a\\\\b\\\\c\\\\").dirname());
  EXPECT_EQ("\\\\a\\b", Path("\\\\a\\b\\\\c").dirname());
#else
  EXPECT_EQ("/a//b", Path("/a//b//c//").dirname());
  EXPECT_EQ("//a/b", Path("//a/b//c").dirname());
#endif // _WIN32

  // Check trailing slashes get cleaned up properly.
#ifdef _WIN32
  EXPECT_EQ(".", Path("a\\").dirname());
  EXPECT_EQ("a\\b", Path("a\\b\\c").dirname());
  EXPECT_EQ("\\a\\b", Path("\\a\\b\\c\\").dirname());
  EXPECT_EQ("\\a\\b", Path("\\a\\b\\c\\\\").dirname());
  EXPECT_EQ("\\a\\b", Path("\\a\\b\\c\\\\\\").dirname());
  EXPECT_EQ("\\", Path("\\\\").dirname());
  EXPECT_EQ("\\", Path("\\\\\\").dirname());
#else
  EXPECT_EQ(".", Path("a/").dirname());
  EXPECT_EQ("a/b", Path("a/b/c").dirname());
  EXPECT_EQ("/a/b", Path("/a/b/c/").dirname());
  EXPECT_EQ("/a/b", Path("/a/b/c//").dirname());
  EXPECT_EQ("/a/b", Path("/a/b/c///").dirname());
  EXPECT_EQ("/", Path("//").dirname());
  EXPECT_EQ("/", Path("///").dirname());
#endif // _WIN32
}


TEST(PathTest, Extension) {
  EXPECT_NONE(Path(".").extension());
  EXPECT_NONE(Path("..").extension());

  EXPECT_NONE(Path("a").extension());
#ifdef _WIN32
  EXPECT_NONE(Path("\\a").extension());
  EXPECT_NONE(Path("\\").extension());
#else
  EXPECT_NONE(Path("/a").extension());
  EXPECT_NONE(Path("/").extension());
#endif // _WIN32

#ifdef _WIN32
  EXPECT_NONE(Path("\\a.b\\c").extension());
#else
  EXPECT_NONE(Path("/a.b/c").extension());
#endif // _WIN32

  EXPECT_SOME_EQ(".txt", Path("a.txt").extension());
#ifdef _WIN32
  EXPECT_SOME_EQ(".txt", Path("\\a\\b.txt").extension());
  EXPECT_SOME_EQ(".txt", Path("\\a.b\\c.txt").extension());
#else
  EXPECT_SOME_EQ(".txt", Path("/a/b.txt").extension());
  EXPECT_SOME_EQ(".txt", Path("/a.b/c.txt").extension());
#endif // _WIN32

  EXPECT_SOME_EQ(".gz", Path("a.tar.gz").extension());
  EXPECT_SOME_EQ(".bashrc", Path(".bashrc").extension());
#ifdef _WIN32
  EXPECT_SOME_EQ(".gz", Path("\\a.tar.gz").extension());
  EXPECT_SOME_EQ(".bashrc", Path("\\.bashrc").extension());
#else
  EXPECT_SOME_EQ(".gz", Path("/a.tar.gz").extension());
  EXPECT_SOME_EQ(".bashrc", Path("/.bashrc").extension());
#endif // _WIN32
}


TEST(PathTest, Normalize) {
  EXPECT_SOME_EQ(".", path::normalize(""));

#ifndef _WIN32
  EXPECT_SOME_EQ("a/b/c", path::normalize("a/b/c/"));
  EXPECT_SOME_EQ("a/b/c", path::normalize("a///b//c"));
  EXPECT_SOME_EQ("a/b/c", path::normalize("a/foobar/../b//c/"));
  EXPECT_SOME_EQ("a/b/c/.d", path::normalize("a/b/c/./.d/"));

  EXPECT_SOME_EQ(".", path::normalize("a/b/../c/../.."));
  EXPECT_SOME_EQ(".", path::normalize("a/b/../c/../../"));

  EXPECT_SOME_EQ("..", path::normalize("a/../b/c/../../.."));
  EXPECT_SOME_EQ("../..", path::normalize("a/../../.."));
  EXPECT_SOME_EQ("../../a", path::normalize("../.././a/"));
  EXPECT_SOME_EQ("../../b", path::normalize("../../a///../b"));
  EXPECT_SOME_EQ("../../c", path::normalize("a/../b/.././../../c"));

  EXPECT_SOME_EQ("/a/b/c", path::normalize("/a/b/c"));
  EXPECT_SOME_EQ("/a/b/c", path::normalize("//a///b/c"));
  EXPECT_SOME_EQ("/a/b/c", path::normalize("/a/foobar/../b//c/"));
  EXPECT_SOME_EQ("/a/b/c/.d", path::normalize("/a/b/c/./.d/"));

  EXPECT_SOME_EQ("/", path::normalize("/a/b/../c/../.."));
  EXPECT_SOME_EQ("/", path::normalize("/a/b/../c/../../"));

  EXPECT_ERROR(path::normalize("/a/../b/c/../../.."));
  EXPECT_ERROR(path::normalize("/a/../../.."));
  EXPECT_ERROR(path::normalize("/../.././a/"));
  EXPECT_ERROR(path::normalize("/../../a///../b"));
  EXPECT_ERROR(path::normalize("//a/../b/.././../../c"));
#endif // _WIN32
}


TEST(PathTest, Join) {
  EXPECT_EQ("a%b", path::join("a", "b", '%'));

#ifndef _WIN32
  EXPECT_EQ("/", path::join("", ""));
  EXPECT_EQ("/", path::join("", "", ""));
  EXPECT_EQ("/a", path::join("", "a"));
  EXPECT_EQ("a/", path::join("a", ""));
  EXPECT_EQ("a/b", path::join("a", "b"));
#else
  EXPECT_EQ("\\", path::join("", ""));
  EXPECT_EQ("\\", path::join("", "", ""));
  EXPECT_EQ("\\a", path::join("", "a"));
  EXPECT_EQ("a\\", path::join("a", ""));
  EXPECT_EQ("a\\b", path::join("a", "b"));
#endif // _WIN32

#ifdef _WIN32
  EXPECT_EQ("a\\b\\c", path::join("a", "b", "c"));
  EXPECT_EQ("\\a\\b\\c", path::join("\\a", "b", "c"));
#else
  EXPECT_EQ("a/b/c", path::join("a", "b", "c"));
  EXPECT_EQ("/a/b/c", path::join("/a", "b", "c"));
#endif // _WIN32

  EXPECT_EQ("", path::join(vector<string>()));
#ifdef _WIN32
  EXPECT_EQ("a\\b\\c", path::join(vector<string>({"a", "b", "c"})));
#else
  EXPECT_EQ("a/b/c", path::join(vector<string>({"a", "b", "c"})));
#endif // _WIN32

  // TODO(cmaloney): This should join to ""
#ifdef _WIN32
  EXPECT_EQ("\\", path::join(vector<string>({"", "", ""})));
#else
  EXPECT_EQ("/", path::join(vector<string>({"", "", ""})));
#endif // _WIN32

  // Interesting corner cases around being the first, middle, last.
#ifdef _WIN32
  EXPECT_EQ("\\asdf", path::join("\\", "asdf"));
  EXPECT_EQ("\\", path::join("", "\\", ""));
  EXPECT_EQ("ab\\", path::join("ab\\", "", "\\"));
  EXPECT_EQ("\\ab", path::join("\\", "\\", "ab"));
  EXPECT_EQ("ab\\", path::join("ab", "\\", "\\"));
  EXPECT_EQ("\\ab", path::join("\\", "", "\\ab"));
#else
  EXPECT_EQ("/asdf", path::join("/", "asdf"));
  EXPECT_EQ("/", path::join("", "/", ""));
  EXPECT_EQ("ab/", path::join("ab/", "", "/"));
  EXPECT_EQ("/ab", path::join("/", "/", "ab"));
  EXPECT_EQ("ab/", path::join("ab", "/", "/"));
  EXPECT_EQ("/ab", path::join("/", "", "/ab"));
#endif // _WIN32

  // Check trailing and leading slashes get cleaned up.
#ifdef _WIN32
  EXPECT_EQ("a\\b\\c\\", path::join("a\\", "b\\", "c\\"));
  EXPECT_EQ("\\a\\b\\c", path::join("\\a", "\\b", "\\c"));
  EXPECT_EQ("\\a\\b\\c\\", path::join("\\a\\", "\\b\\", "\\c\\"));
  EXPECT_EQ("a\\b\\c\\", path::join("a\\", "\\b\\", "\\c\\"));
#else
  EXPECT_EQ("a/b/c/", path::join("a/", "b/", "c/"));
  EXPECT_EQ("/a/b/c", path::join("/a", "/b", "/c"));
  EXPECT_EQ("/a/b/c/", path::join("/a/", "/b/", "/c/"));
  EXPECT_EQ("a/b/c/", path::join("a/", "/b/", "/c/"));
#endif // _WIN32
}


TEST(PathTest, Absolute) {
#ifdef _WIN32
  // Check absolute paths.
  EXPECT_TRUE(path::absolute("C:\\foo\\bar\\baz"));
  EXPECT_TRUE(path::absolute("c:\\"));
  EXPECT_TRUE(path::absolute("C:/"));
  EXPECT_TRUE(path::absolute("c:/"));
  EXPECT_TRUE(path::absolute("X:\\foo"));
  EXPECT_TRUE(path::absolute("X:\\foo"));
  EXPECT_TRUE(path::absolute("y:\\bar"));
  EXPECT_TRUE(path::absolute("y:/bar"));
  EXPECT_TRUE(path::absolute("\\\\?\\"));
  EXPECT_TRUE(path::absolute("\\\\?\\C:\\Program Files"));
  EXPECT_TRUE(path::absolute("\\\\?\\C:/Program Files"));
  EXPECT_TRUE(path::absolute("\\\\?\\C:\\Path"));
  EXPECT_TRUE(path::absolute("\\\\server\\share"));

  // Check invalid paths.
  EXPECT_FALSE(path::absolute("abc:/"));
  EXPECT_FALSE(path::absolute("1:/"));
  EXPECT_TRUE(path::absolute("\\\\?\\relative"));

  // Check relative paths.
  EXPECT_FALSE(path::absolute("relative"));
  EXPECT_FALSE(path::absolute("\\file-without-disk"));
  EXPECT_FALSE(path::absolute("/file-without-disk"));
  EXPECT_FALSE(path::absolute("N:file-without-dir"));
#else
  // Check absolute paths.
  EXPECT_TRUE(path::absolute("/"));
  EXPECT_TRUE(path::absolute("/foo"));
  EXPECT_TRUE(path::absolute("/foo/bar"));
  EXPECT_TRUE(path::absolute("/foo/bar/../baz"));

  // Check relative paths.
  EXPECT_FALSE(path::absolute(""));
  EXPECT_FALSE(path::absolute("."));
  EXPECT_FALSE(path::absolute(".."));
  EXPECT_FALSE(path::absolute("../"));
  EXPECT_FALSE(path::absolute("./foo"));
  EXPECT_FALSE(path::absolute("../foo"));
#endif // _WIN32
}


TEST(PathTest, Comparison) {
  EXPECT_TRUE(Path("a") == Path("a"));
  EXPECT_FALSE(Path("a") == Path("b"));

  EXPECT_TRUE(Path("a") != Path("b"));
  EXPECT_FALSE(Path("a") != Path("a"));

  EXPECT_TRUE(Path("a") < Path("b"));
  EXPECT_FALSE(Path("b") < Path("a"));

  EXPECT_TRUE(Path("a") <= Path("b"));
  EXPECT_TRUE(Path("a") <= Path("a"));
  EXPECT_FALSE(Path("b") <= Path("a"));

  EXPECT_TRUE(Path("b") > Path("a"));
  EXPECT_FALSE(Path("a") > Path("a"));

  EXPECT_TRUE(Path("b") >= Path("a"));
  EXPECT_TRUE(Path("b") >= Path("b"));
  EXPECT_FALSE(Path("a") >= Path("b"));
}


TEST(PathTest, FromURI) {
#ifdef _WIN32
  const std::string absolute_path = "C:\\somedir\\somefile";
#else
  const std::string absolute_path = "/somedir/somefile";
#endif // _WIN32

  EXPECT_EQ("", path::from_uri(""));
  EXPECT_EQ(absolute_path, path::from_uri(absolute_path));
  EXPECT_EQ(absolute_path, path::from_uri("file://" + absolute_path));

#ifdef _WIN32
  EXPECT_EQ(absolute_path, path::from_uri("file://C:/somedir/somefile"));
  EXPECT_EQ(absolute_path, path::from_uri("C:/somedir/somefile"));
  EXPECT_EQ(absolute_path, path::from_uri("C:\\somedir\\somefile"));
#endif // _WIN32
}


class PathFileTest : public TemporaryDirectoryTest {};


TEST_F(PathFileTest, ImplicitConversion) {
  // Should be implicitly converted to string for the various os::_ calls.
  const Path testfile(path::join(os::getcwd(), "file.txt"));

  // Create the test file.
  ASSERT_SOME(os::touch(testfile));
  ASSERT_TRUE(os::exists(testfile));

  // Open and close the file.
  Try<int_fd> fd = os::open(
      testfile,
      O_RDONLY,
      S_IRUSR | S_IRGRP | S_IROTH);
  ASSERT_SOME(fd);
  close(fd.get());

  // Delete the file.
  EXPECT_SOME(os::rm(testfile));
}
