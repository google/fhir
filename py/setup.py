#
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
"""The setuptools script for the Google FHIR Python distribution package."""

# TODO: Explore PEP517 and PEP518 adoption in lieu of `setup.py`.

from distutils import spawn
from distutils.command import clean
import glob
import os
import pathlib
import subprocess
import sys
from typing import List

import setuptools
from setuptools.command import build_py
from setuptools.command import sdist

_HERE = os.path.dirname(os.path.abspath(__file__))
_ROOT = os.path.abspath(os.path.join(_HERE, '..'))
_PROTO = os.path.join(_ROOT, 'proto', 'google', 'fhir', 'proto')
_PROTO_FILES = (
    glob.glob(os.path.join(_PROTO, '*.proto')) +
    glob.glob(os.path.join(_PROTO, 'stu3', '**', '*.proto'), recursive=True) +
    glob.glob(os.path.join(_PROTO, 'r4', '**', '*.proto'), recursive=True))

# Locate the protobuf compiler. If it does not exist, abandon.
_protoc = (
    os.environ['PROTOC'] if 'PROTOC' in os.environ and
    os.path.exists(os.environ['PROTOC']) else spawn.find_executable('protoc'))
if _protoc is None:
  raise FileNotFoundError('protoc was not found.')


def _generate_python_protos(proto_files: List[str], *, python_out: str = '.'):
  """Generates FHIR Python protobuf classes.

  Args:
    proto_files: A list of .proto file paths.
    python_out: Indicates where the generated definitions should be placed.
      Defaults to '.', which indicates to place alongside the .proto definition.
      This directory will be created if it does not exist.
  """
  if not proto_files:
    return

  subpackages = []
  for proto_file in proto_files:
    proto_dir = os.path.dirname(os.path.abspath(proto_file))

    # Preserve subdirectory structure relative to _HERE
    commonpath = os.path.commonpath((_HERE, proto_dir))
    relpath = os.path.relpath(proto_dir, commonpath)

    # Create Python package
    init_py = os.path.join(python_out, relpath, '__init__.py')
    if not os.path.exists(init_py):
      subpackage = os.path.dirname(init_py)
      subpackages.append(subpackage)
      pathlib.Path(subpackage).mkdir(parents=True)
      pathlib.Path(init_py).touch()

    # Generate _py_pb2.py file
    output = proto_file.replace('.proto', '_py_pb2.py')
    if (not os.path.exists(output) or
        (os.path.exists(proto_file) and
         os.path.getmtime(proto_file) > os.path.getmtime(output))):
      sys.stdout.write(f'Generating file: {output}\n')
      protoc_cmd = (
          _protoc,
          f'-I={_ROOT}',
          f'--python_out={python_out}',
          f'--mypy_out={python_out}',
          proto_file,
      )

      if subprocess.call(protoc_cmd) != 0:
        sys.exit(-1)

  # Add py.typed for PEP0561 compliance
  root_package = os.path.commonpath(subpackages)
  pathlib.Path(os.path.join(root_package, 'py.typed')).touch()


def _parse_requirements(path: str) -> List[str]:
  """Parses a requirements.txt file into a list of strings."""
  with open(os.path.join(_HERE, path), 'r') as f:
    return [
        line.rstrip()
        for line in f
        if not (line.isspace() or line.startswith('#'))
    ]


class _FhirBuildPy(build_py.build_py):
  """Generates _py_pb2.py FHIR protobuf classes."""

  def run(self):
    if not self.dry_run:
      _generate_python_protos(_PROTO_FILES, python_out=self.build_lib)
      build_py.build_py.run(self)


class _FhirSdist(sdist.sdist):
  """Generates a Source Archive for the FHIR R4 specification."""

  def make_release_tree(self, base_dir: str, files: List[str]):
    sdist.sdist.make_release_tree(self, base_dir, files)
    _generate_python_protos(_PROTO_FILES, python_out=base_dir)


class _FhirClean(clean.clean):
  """Removes generated distribution files."""

  def run(self):
    clean.clean.run(self)
    targets = (
        os.path.join(_HERE, 'build'),
        os.path.join(_HERE, 'dist'),
        os.path.join(_HERE, '*.egg-info'),
    )
    os.system(f"shopt -s globstar; rm -vrf {' '.join(targets)}")


requirements = _parse_requirements('requirements.txt')
namespace_packages = setuptools.find_namespace_packages(where=_HERE)
long_description = pathlib.Path(_HERE).joinpath('README.md').read_text()

setuptools.setup(
    name='google-fhir',
    version='0.7.1',
    description='Tools for parsing and printing FHIR JSON.',
    long_description=long_description,
    long_description_content_type='text/markdown',
    author='Google LLC',
    author_email='google-fhir-pypi@google.com',
    url='https://github.com/google/fhir',
    download_url='https://github.com/google/fhir/releases',
    packages=namespace_packages,
    include_package_data=True,
    license='Apache 2.0',
    python_requires='>=3.6, <3.10',
    install_requires=requirements,
    zip_safe=False,
    keywords='google fhir python healthcare',
    cmdclass={
        'build_py': _FhirBuildPy,
        'clean': _FhirClean,
        'sdist': _FhirSdist,
    },
    classifiers=[
        'License :: OSI Approved :: Apache Software License',
        'Operating System :: MacOS :: MacOS X',
        'Operating System :: POSIX :: Linux',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
    ],
)
