"""Adds repositories/archives needed by stout."""

########################################################################
# DO NOT EDIT THIS FILE unless you are inside the
# https://github.com/3rdparty/stout repository. If you encounter it
# anywhere else it is because it has been copied there in order to
# simplify adding transitive dependencies. If you want a different
# version of stout follow the Bazel build instructions at
# https://github.com/3rdparty/stout.
########################################################################

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("//3rdparty/bazel-rules-picojson:repos.bzl", picojson_repos = "repos")
load("//3rdparty/bazel-rules-rapidjson:repos.bzl", rapidjson_repos = "repos")

# buildifier: disable=out-of-order-load
load("@com_github_3rdparty_stout_atomic_backoff//bazel:repos.bzl", stout_atomic_backoff_repos = "repos")
load("@com_github_3rdparty_stout_borrowed_ptr//bazel:repos.bzl", stout_borrowed_ptr_repos = "repos")
load("@com_github_3rdparty_stout_flags//bazel:repos.bzl", stout_flags_repos = "repos")
load("@com_github_3rdparty_stout_notification//bazel:repos.bzl", stout_notification_repos = "repos")
load("@com_github_3rdparty_stout_stateful_tally//bazel:repos.bzl", stout_stateful_tally_repos = "repos")

def repos(external = True, repo_mapping = {}):
    """Adds repositories/archives needed by stout

    Args:
          external: whether or not we're invoking this function as though
            though we're an external dependency
          repo_mapping: passed through to all other functions that expect/use
            repo_mapping, e.g., 'git_repository'
    """
    picojson_repos(
        repo_mapping = repo_mapping,
    )

    rapidjson_repos(
        repo_mapping = repo_mapping,
    )

    stout_atomic_backoff_repos(
        repo_mapping = repo_mapping,
    )

    stout_borrowed_ptr_repos(
        repo_mapping = repo_mapping,
    )

    stout_flags_repos(
        repo_mapping = repo_mapping,
    )

    stout_notification_repos(
        repo_mapping = repo_mapping,
    )

    stout_stateful_tally_repos(
        repo_mapping = repo_mapping,
    )

    if "com_github_nelhage_rules_boost" not in native.existing_rules():
        git_repository(
            name = "com_github_nelhage_rules_boost",
            commit = "32164a62e2472077320f48f52b8077207cd0c9c8",
            remote = "https://github.com/nelhage/rules_boost",
            shallow_since = "1650381330 -0700",
        )

    if "com_github_gflags_gflags" not in native.existing_rules():
        http_archive(
            name = "com_github_gflags_gflags",
            url = "https://github.com/gflags/gflags/archive/v2.2.2.tar.gz",
            sha256 = "34af2f15cf7367513b352bdcd2493ab14ce43692d2dcd9dfc499492966c64dcf",
            strip_prefix = "gflags-2.2.2",
        )

    if "com_github_google_glog" not in native.existing_rules():
        http_archive(
            name = "com_github_google_glog",
            url = "https://github.com/google/glog/archive/refs/tags/v0.5.0.tar.gz",
            sha256 = "eede71f28371bf39aa69b45de23b329d37214016e2055269b3b5e7cfd40b59f5",
            strip_prefix = "glog-0.5.0",
        )

    if external and "com_github_3rdparty_stout" not in native.existing_rules():
        git_repository(
            name = "com_github_3rdparty_stout",
            commit = "67e6b9b08f340e223b741130815d97cf20296c08",
            remote = "https://github.com/3rdparty/stout",
            shallow_since = "1637367065 -0800",
        )
