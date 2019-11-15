workspace(name = "com_google_fhir")

load("//bazel:dependencies.bzl", "fhirproto_dependencies")
fhirproto_dependencies()

load("//bazel:workspace.bzl", "fhirproto_workspace")
fhirproto_workspace()

load("@fhir_bazel_pip_dependencies//:requirements.bzl", "pip_install")
pip_install()
