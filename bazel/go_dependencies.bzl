""" Function for loading go dependencies for the go jsonformat library"""

load("@io_bazel_rules_go//go:deps.bzl", "go_register_toolchains", "go_rules_dependencies")
load("@bazel_gazelle//:deps.bzl", "gazelle_dependencies", "go_repository")

def fhir_go_dependencies():
    """ Loads dependencies of the Go FHIR library"""
    go_rules_dependencies()
    go_register_toolchains()
    gazelle_dependencies()

    go_repository(
        name = "com_github_pkg_errors",
        importpath = "github.com/pkg/errors",
        tag = "v0.9.1",
    )

    go_repository(
        name = "com_github_serenize_snaker",
        commit = "a683aaf2d516deecd70cad0c72e3ca773ecfcef0",
        importpath = "github.com/serenize/snaker",
    )

    go_repository(
        name = "com_github_golang_glog",
        importpath = "github.com/golang/glog",
        tag = "23def4e6c14b4da8ac2ed8007337bc5eb5007998",
    )

    go_repository(
        name = "com_github_json_iterator_go",
        importpath = "github.com/json-iterator/go",
        tag = "v1.1.9",
    )

    go_repository(
        name = "com_github_vitessio",
        importpath = "github.com/vitessio/vitess",
        tag = "vitess-parent-3.0.0",
    )

    go_repository(
        name = "com_bitbucket_creachadair_stringset",
        importpath = "bitbucket.org/creachadair/stringset",
        tag = "v0.0.8",
    )

    go_repository(
        name = "com_github_google_go_cmp",
        importpath = "github.com/google/go-cmp",
        tag = "v0.3.0",
    )

    go_repository(
        name = "org_golang_google_protobuf",
        commit = "d165be301fb1e13390ad453281ded24385fd8ebc",
        importpath = "google.golang.org/protobuf",
    )

    go_repository(
        name = "com_github_modern_go_reflect2",
        importpath = "github.com/modern-go/reflect2",
        commit = "4b7aa43c6742a2c18fdef89dd197aaae7dac7ccd",
    )

    go_repository(
        name = "com_github_modern_go_concurrent",
        importpath = "github.com/modern-go/concurrent",
        commit = "bacd9c7ef1dd9b15be4a9909b8ac7a4e313eec94",
    )
