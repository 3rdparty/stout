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

    # Needed because protobuf_deps brings rules_python 0.26.0 which is broken:
    # https://github.com/bazelbuild/rules_python/issues/1543
    http_archive(
        name = "rules_python",
        sha256 = "5868e73107a8e85d8f323806e60cad7283f34b32163ea6ff1020cf27abef6036",
        strip_prefix = "rules_python-0.25.0",
        url = "https://github.com/bazelbuild/rules_python/releases/download/0.25.0/rules_python-0.25.0.tar.gz",
    )

    protobuf_deps()
