"""Dependency specific initialization for stout."""

load("@com_github_nelhage_rules_boost//:boost/boost.bzl", "boost_deps")
load("@com_github_3rdparty_bazel_rules_picojson//:bazel/picojson.bzl", "picojson_deps")
load("@com_github_3rdparty_bazel_rules_rapidjson//:bazel/rapidjson.bzl", "rapidjson_deps")

def stout_deps():
    if "boost" not in native.existing_rules():
        boost_deps()

    if "picojson" not in native.existing_rules():
        picojson_deps()

    if "com_github_tencent_rapidjson" not in native.existing_rules():
        rapidjson_deps()
