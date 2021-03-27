"""Dependency specific initialization for stout."""

load("@com_github_nelhage_rules_boost//:boost/boost.bzl", boost="boost_deps")
load("@com_github_3rdparty_bazel_rules_picojson//bazel:deps.bzl", picojson="deps")
load("@com_github_3rdparty_bazel_rules_rapidjson//bazel:deps.bzl", rapidjson="deps")

def deps(**kwargs):
    boost(**kwargs)
    picojson(**kwargs)
    rapidjson(**kwargs)
