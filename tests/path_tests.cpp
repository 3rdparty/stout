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

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <stout/path.hpp>

#include <stout/os/exists.hpp>
#include <stout/os/getcwd.hpp>
#include <stout/os/int_fd.hpp>
#include <stout/os/open.hpp>
#include <stout/os/rm.hpp>
#include <stout/os/touch.hpp>

#include <stout/tests/utils.hpp>

using std::string;
using std::vector;


// Test many corner cases of Path::basename.
TEST(PathTest, Basename)
{
  // Empty path check.
  EXPECT_EQ(".", Path("").basename());

  // Check common path patterns.
#ifdef __WINDOWS__
  EXPECT_EQ("\\", Path("\\").basename());
#else
  EXPECT_EQ("/", Path("/").basename());
#endif // __WINDOWS__
  EXPECT_EQ(".", Path(".").basename());
  EXPECT_EQ("..", Path("..").basename());

  EXPECT_EQ("a", Path("a").basename());
#ifdef __WINDOWS__
  EXPECT_EQ("b", Path("a\\b").basename());
  EXPECT_EQ("c", Path("a\\b\\c").basename());
#else
  EXPECT_EQ("b", Path("a/b").basename());
  EXPECT_EQ("c", Path("a/b/c").basename());
#endif // __WINDOWS__

  // Check leading slashes get cleaned up properly.
#ifdef __WINDOWS__
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
#endif // __WINDOWS__

  // Check trailing slashes get cleaned up properly.
#ifdef __WINDOWS__
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
#endif // __WINDOWS__
}


// Test many corner cases of Path::dirname.
TEST(PathTest, Dirname)
{
  // Empty path check.
  EXPECT_EQ(".", Path("").dirname());

  // Check common path patterns.
#ifdef __WINDOWS__
  EXPECT_EQ("\\", Path("\\").dirname());
#else
  EXPECT_EQ("/", Path("/").dirname());
#endif // __WINDOWS__
  EXPECT_EQ(".", Path(".").dirname());
  EXPECT_EQ(".", Path("..").dirname());

  EXPECT_EQ(".", Path("a").dirname());
#ifdef __WINDOWS__
  EXPECT_EQ("a", Path("a\\b").dirname());
  EXPECT_EQ("a\\b", Path("a\\b\\c\\").dirname());
#else
  EXPECT_EQ("a", Path("a/b").dirname());
  EXPECT_EQ("a/b", Path("a/b/c/").dirname());
#endif // __WINDOWS__

  // Check leading slashes get cleaned up properly.
#ifdef __WINDOWS__
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
#endif // __WINDOWS__

  // Check intermittent slashes get handled just like ::dirname does.
#ifdef __WINDOWS__
  EXPECT_EQ("\\a\\\\b", Path("\\a\\\\b\\\\c\\\\").dirname());
  EXPECT_EQ("\\\\a\\b", Path("\\\\a\\b\\\\c").dirname());
#else
  EXPECT_EQ("/a//b", Path("/a//b//c//").dirname());
  EXPECT_EQ("//a/b", Path("//a/b//c").dirname());
#endif // __WINDOWS__

  // Check trailing slashes get cleaned up properly.
#ifdef __WINDOWS__
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
#endif // __WINDOWS__
}


TEST(PathTest, Extension)
{
  EXPECT_NONE(Path(".").extension());
  EXPECT_NONE(Path("..").extension());

  EXPECT_NONE(Path("a").extension());
#ifdef __WINDOWS__
  EXPECT_NONE(Path("\\a").extension());
  EXPECT_NONE(Path("\\").extension());
#else
  EXPECT_NONE(Path("/a").extension());
  EXPECT_NONE(Path("/").extension());
#endif // __WINDOWS__

#ifdef __WINDOWS__
  EXPECT_NONE(Path("\\a.b\\c").extension());
#else
  EXPECT_NONE(Path("/a.b/c").extension());
#endif // __WINDOWS__

  EXPECT_SOME_EQ(".txt", Path("a.txt").extension());
#ifdef __WINDOWS__
  EXPECT_SOME_EQ(".txt", Path("\\a\\b.txt").extension());
  EXPECT_SOME_EQ(".txt", Path("\\a.b\\c.txt").extension());
#else
  EXPECT_SOME_EQ(".txt", Path("/a/b.txt").extension());
  EXPECT_SOME_EQ(".txt", Path("/a.b/c.txt").extension());
#endif // __WINDOWS__

  EXPECT_SOME_EQ(".gz", Path("a.tar.gz").extension());
  EXPECT_SOME_EQ(".bashrc", Path(".bashrc").extension());
#ifdef __WINDOWS__
  EXPECT_SOME_EQ(".gz", Path("\\a.tar.gz").extension());
  EXPECT_SOME_EQ(".bashrc", Path("\\.bashrc").extension());
#else
  EXPECT_SOME_EQ(".gz", Path("/a.tar.gz").extension());
  EXPECT_SOME_EQ(".bashrc", Path("/.bashrc").extension());
#endif // __WINDOWS__
}


TEST(PathTest, Normalize)
{
  EXPECT_SOME_EQ(".", path::normalize(""));

#ifndef __WINDOWS__
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
#endif // __WINDOWS__
}


TEST(PathTest, Join)
{
  EXPECT_EQ("a%b", path::join("a", "b", '%'));

#ifndef __WINDOWS__
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
#endif // __WINDOWS__

#ifdef __WINDOWS__
  EXPECT_EQ("a\\b\\c", path::join("a", "b", "c"));
  EXPECT_EQ("\\a\\b\\c", path::join("\\a", "b", "c"));
#else
  EXPECT_EQ("a/b/c", path::join("a", "b", "c"));
  EXPECT_EQ("/a/b/c", path::join("/a", "b", "c"));
#endif // __WINDOWS__

  EXPECT_EQ("", path::join(vector<string>()));
#ifdef __WINDOWS__
  EXPECT_EQ("a\\b\\c", path::join(vector<string>({"a", "b", "c"})));
#else
  EXPECT_EQ("a/b/c", path::join(vector<string>({"a", "b", "c"})));
#endif // __WINDOWS__

  // TODO(cmaloney): This should join to ""
#ifdef __WINDOWS__
  EXPECT_EQ("\\", path::join(vector<string>({"", "", ""})));
#else
  EXPECT_EQ("/", path::join(vector<string>({"", "", ""})));
#endif // __WINDOWS__

  // Interesting corner cases around being the first, middle, last.
#ifdef __WINDOWS__
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
#endif // __WINDOWS__

  // Check trailing and leading slashes get cleaned up.
#ifdef __WINDOWS__
  EXPECT_EQ("a\\b\\c\\", path::join("a\\", "b\\", "c\\"));
  EXPECT_EQ("\\a\\b\\c", path::join("\\a", "\\b", "\\c"));
  EXPECT_EQ("\\a\\b\\c\\", path::join("\\a\\", "\\b\\", "\\c\\"));
  EXPECT_EQ("a\\b\\c\\", path::join("a\\", "\\b\\", "\\c\\"));
#else
  EXPECT_EQ("a/b/c/", path::join("a/", "b/", "c/"));
  EXPECT_EQ("/a/b/c", path::join("/a", "/b", "/c"));
  EXPECT_EQ("/a/b/c/", path::join("/a/", "/b/", "/c/"));
  EXPECT_EQ("a/b/c/", path::join("a/", "/b/", "/c/"));
#endif // __WINDOWS__
}


TEST(PathTest, Absolute)
{
#ifdef __WINDOWS__
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
#endif // __WINDOWS__
}


TEST(PathTest, Comparison)
{
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


TEST(PathTest, FromURI)
{
#ifdef __WINDOWS__
  const std::string absolute_path = "C:\\somedir\\somefile";
#else
  const std::string absolute_path = "/somedir/somefile";
#endif // __WINDOWS__

  EXPECT_EQ("", path::from_uri(""));
  EXPECT_EQ(absolute_path, path::from_uri(absolute_path));
  EXPECT_EQ(absolute_path, path::from_uri("file://" + absolute_path));

#ifdef __WINDOWS__
  EXPECT_EQ(absolute_path, path::from_uri("file://C:/somedir/somefile"));
  EXPECT_EQ(absolute_path, path::from_uri("C:/somedir/somefile"));
  EXPECT_EQ(absolute_path, path::from_uri("C:\\somedir\\somefile"));
#endif // __WINDOWS__
}


class PathFileTest : public TemporaryDirectoryTest {};


TEST_F(PathFileTest, ImplicitConversion)
{
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
