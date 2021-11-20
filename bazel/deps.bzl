"""Dependency specific initialization for stout."""

load("@com_github_nelhage_rules_boost//:boost/boost.bzl", "boost_deps")
load("@com_github_3rdparty_bazel_rules_picojson//bazel:deps.bzl", picojson_deps="deps")
load("@com_github_3rdparty_bazel_rules_rapidjson//bazel:deps.bzl", rapidjson_deps="deps")

def deps(repo_mapping = {}):
    boost_deps()

    picojson_deps(
        repo_mapping = repo_mapping,
    )

    rapidjson_deps(
        repo_mapping = repo_mapping,
    )
