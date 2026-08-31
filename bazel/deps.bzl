"""Dependency specific initialization for stout."""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@com_github_3rdparty_bazel_rules_picojson//bazel:deps.bzl", picojson_deps = "deps")
load("@com_github_3rdparty_bazel_rules_rapidjson//bazel:deps.bzl", rapidjson_deps = "deps")
load("@com_github_nelhage_rules_boost//:boost/boost.bzl", "boost_deps")
load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")

def deps(repo_mapping = {}):
    """Propagate all dependencies.

    Args:
        repo_mapping (str): {}.
    """
    boost_deps()

    picojson_deps(
        repo_mapping = repo_mapping,
    )

    rapidjson_deps(
        repo_mapping = repo_mapping,
    )

    # Needed because `protobuf_deps()` brings `rules_python` 0.28.0,
    # which predates Bazel 8's removal of the native `PyInfo` /
    # `PyCcLinkParamsProvider` globals and so fails to load under
    # Bazel 8's autoloading. 0.40.0 is the version Bazel 8.7.0's own
    # WORKSPACE suffix declares.
    http_archive(
        name = "rules_python",
        sha256 = "690e0141724abb568267e003c7b6d9a54925df40c275a870a4d934161dc9dd53",
        strip_prefix = "rules_python-0.40.0",
        url = "https://github.com/bazelbuild/rules_python/releases/download/0.40.0/rules_python-0.40.0.tar.gz",
    )

    protobuf_deps()
