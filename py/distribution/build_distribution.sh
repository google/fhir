#!/bin/bash
# Copyright 2020 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# This script creates an sdist and bdist_wheel of google-fhir for Debian/Darwin
# platforms. Both sdist and bdist_wheel artifacts are installed into a local
# virtualenv and tested against the full suite of tests (although the test data
# is not actually shipped with the artifacts themselves).
#
# This script should be executed from //py/ and, while not required, is best if
# executed inside a Docker container. See README.md for more information.

set -e
set -E
set -u

readonly FHIR_ROOT="${PWD}/.."
readonly FHIR_WORKSPACE='com_google_fhir'
readonly MYPY_PROTOBUF_VERSION='1.23.0'
readonly PROTOBUF_URL='https://github.com/protocolbuffers/protobuf/releases/download'
readonly PROTOC_SHA='4a3b26d1ebb9c1d23e933694a6669295f6a39ddc64c3db2adf671f0a6026f82e'
readonly PROTOC_VERSION='3.13.0'
readonly PYTHON_BUILD_VERSION='3.9.0'
readonly PYTHON_VERSIONS=('3.6.13' '3.7.9' '3.8.6' '3.9.0')

# Helper around print statements.
function print_info() {

  function print_sep() {
    echo "$(seq -s'=' 0 "$(( $(tput cols) - 1))" | tr -d '[:digit:]')"
  }

  print_sep
  echo "$1"
  print_sep
}

# Download and install prebuilt Linux binary from URL. URL should point to a
# compressed (.zip) resource.
#
# Globals:
#   None
# Arguments:
#   url: The URL to fetch.
#   resource: The resource at the basename of the URL (a .zip file).
#   sha: The SHA256 hash of the resource for verification.
#   destination: The destination directory to install to. The user must have
#   sufficient read/write privileges or an error will be raised.
function install_prebuilt_binary() {
  pushd /tmp

  local -r url="$1"
  local -r resource="$2"
  local -r sha="$3"
  local -r destination="$4"

  if [[ -f "${destination}" ]]; then
    echo "File exists at: ${destination}."
    exit 1
  fi

  print_info "Installing ${resource}..."
  curl -OL "${url}"
  echo "${sha} ${resource}" | sha256sum --check
  unzip -o "${resource}" -d "${destination}"

  popd
}

# Helper method to execute all test scripts. The google-fhir package should be
# installed in an active virtual environment prior to calling.
function test_google_fhir() {
  local -r workspace="$1"
  pushd "${workspace}"
  find -L . -type f -name '*_test.py' -not -path '*/build/*' -print0 | \
  xargs -0 -n1 python3 -I
  popd
}

# Sets up a google-fhir "workspace" with appropriate system binaries and
# subdirectory structure for building+executing the full test suite against an
# installed google-fhir package.
#
# Globals:
#   FHIR_ROOT: The parent directory of //py/; necessary for testdata/ and spec/.
#   FHIR_WORKSPACE: The Bazel workspace identifier of google-fhir.
# Arguments:
#   workspace: An ephemeral directory for staging+testing distributions.
function initialize_workspace() {
  local -r workspace="$1"

  # protoc
  local -r protoc_resource="protoc-${PROTOC_VERSION}-linux-x86_64.zip"
  local -r protoc_url="${PROTOBUF_URL}/v${PROTOC_VERSION}/${protoc_resource}"
  install_prebuilt_binary "${protoc_url}" \
                          "${protoc_resource}" \
                          "${PROTOC_SHA}" \
                          "${workspace}/.local"
  export PATH="${workspace}/.local/bin:${PATH}"
  export PROTOC="${workspace}/.local/bin/protoc"

  # pyenv
  print_info 'Installing pyenv...'
  local -r pyenv_root="${workspace}/.pyenv"
  git clone 'https://github.com/pyenv/pyenv.git' "${pyenv_root}"
  export PATH="${pyenv_root}/shims:${pyenv_root}/bin:${PATH}"
  eval "$(pyenv init -)"

  # Create FHIR test environment
  mkdir -p "${workspace}/${FHIR_WORKSPACE}"
  ln -s "${FHIR_ROOT}/testdata" "${workspace}/${FHIR_WORKSPACE}/testdata"
  ln -s "${FHIR_ROOT}/spec" "${workspace}/${FHIR_WORKSPACE}/spec"
  ln -s "${FHIR_ROOT}/py" "${workspace}/py"
}

# Removes the ephemeral workspace and pypi artifacts.
#
# Globals:
#   None
# Arguments:
#   workspace: An ephemeral directory for staging+testing distributions.
function cleanup() {
  local -r workspace="$1"
  python3 setup.py clean && \
  rm -rf "${workspace}"
}

# Installs (if necessary) and activates the requested Python interpreter.
#
# Globals:
#   None
# Arguments:
#   python_version: The version of the Python interpreter to install/activate.
function activate_interpreter() {
  local -r python_version="$1"
  pyenv install -s "${python_version}"
  pyenv global "${python_version}"
  pyenv versions

  # Upgrade build utilities
  pip3 install --upgrade pip
  pip3 install --upgrade setuptools
  pip3 install wheel
}

function main() {
  local -r workspace="$(mktemp -d -t fhir-XXXXXXXXXX)"
  print_info "Initializing workspace ${workspace}..."
  initialize_workspace "${workspace}"
  trap "cleanup ${workspace}" EXIT

  # Build artifacts under one of the supported versions, this should be
  # inconsequential for google-fhir.
  activate_interpreter "${PYTHON_BUILD_VERSION}"

  # `setuptools.setup` "setup_requires" kwarg does not currently install the
  # mypy-protobuf plugin as an executable on the host's `PATH`.
  #
  # See more at: https://github.com/dropbox/mypy-protobuf/issues/137.
  pip3 install "mypy-protobuf==${MYPY_PROTOBUF_VERSION}"

  print_info "Building distribution ($(python --version))..."
  python3 setup.py sdist bdist_wheel

  # Install and test under all supported major/minor Python interpreters
  for python_version in "${PYTHON_VERSIONS[@]}"; do
    activate_interpreter "${python_version}"

    print_info "Testing sdist ($(python --version))..."
    pip3 install dist/*.tar.gz && test_google_fhir "${workspace}" && \
    pip3 uninstall -y google-fhir

    print_info "Testing bdist_wheel ($(python --version))..."
    pip3 install dist/*.whl && test_google_fhir "${workspace}" && \
    pip3 uninstall -y google-fhir
  done

  print_info 'Staging artifacts at /tmp/google/fhir/release/...'
  mkdir -p /tmp/google/fhir/release
  cp dist/* /tmp/google/fhir/release/
}

main "$@"
