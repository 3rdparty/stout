"""Dependency specific initialization for stout."""

load("@com_github_nelhage_rules_boost//:boost/boost.bzl", "boost_deps")

# buildifier: disable=out-of-order-load
load("@com_github_3rdparty_bazel_rules_picojson//bazel:deps.bzl", picojson_deps = "deps")
load("@com_github_3rdparty_bazel_rules_rapidjson//bazel:deps.bzl", rapidjson_deps = "deps")
load("@com_github_3rdparty_stout_atomic_backoff//bazel:deps.bzl", stout_atomic_backoff_deps = "deps")
load("@com_github_3rdparty_stout_borrowed_ptr//bazel:deps.bzl", stout_borrowed_ptr_deps = "deps")
load("@com_github_3rdparty_stout_flags//bazel:deps.bzl", stout_flags_deps = "deps")
load("@com_github_3rdparty_stout_notification//bazel:deps.bzl", stout_notification_deps = "deps")
load("@com_github_3rdparty_stout_stateful_tally//bazel:deps.bzl", stout_stateful_tally_deps = "deps")

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

    stout_atomic_backoff_deps(
        repo_mapping = repo_mapping,
    )

    stout_borrowed_ptr_deps(
        repo_mapping = repo_mapping,
    )

    stout_flags_deps(
        repo_mapping = repo_mapping,
    )

    stout_notification_deps(
        repo_mapping = repo_mapping,
    )

    stout_stateful_tally_deps(
        repo_mapping = repo_mapping,
    )
