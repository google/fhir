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
"""Functions for reading protos to be used in tests."""

import os
from typing import List, Optional, TypeVar, Type

from absl import flags

from google.protobuf import message
from google.protobuf import text_format

FLAGS = flags.FLAGS

_TEST_DATA_DIR_R4 = os.path.join('testdata', 'r4')
_TEST_DATA_DIR_STU3 = os.path.join('testdata', 'stu3')

_T = TypeVar('_T', bound=message.Message)


def _fhir_filepath_from(filename: str) -> str:
  """Returns a filepath from filename relative to the fhir/ root dir."""
  filepath = os.path.join(FLAGS.test_srcdir, 'com_google_fhir/',
                          filename)
  if not os.path.exists(filepath):
    raise FileNotFoundError('File at {} does not exist.'.format(filepath))
  return filepath


def read_data(filename: str, delimiter: Optional[str] = None) -> List[str]:
  """Reads the raw contents of a file and returns as a string."""
  filepath = _fhir_filepath_from(filename)

  raw_text = ''
  with open(filepath, 'r', encoding='utf-8') as f:
    raw_text = f.read()

  if delimiter is None:
    return [raw_text]
  return raw_text.strip(delimiter).split(delimiter)


def read_protos(filename: str,
                proto_cls: Type[_T],
                delimiter: Optional[str] = None) -> List[_T]:
  """Reads protobuf information from filename relative to the fhir/ root dir."""
  raw_protos = read_data(filename, delimiter)

  results = []
  for raw_proto in raw_protos:
    proto = proto_cls()
    text_format.Parse(raw_proto, proto)
    results.append(proto)

  return results


def read_proto(filename: str, proto_cls: Type[_T]) -> _T:
  """Reads protobuf information from filename relative to the fhir/ root dir.

  Data is serialized into an instance of `proto_cls`.

  Args:
    filename: The file to read from.
    proto_cls: The type of protobuf message to look for and return.

  Returns:
    The protobuf message in the file.
  """
  filepath = _fhir_filepath_from(filename)
  proto = proto_cls()
  raw_proto = ''
  with open(filepath, 'r', encoding='utf-8') as f:
    raw_proto = f.read()
    text_format.Parse(raw_proto, proto)

  return proto


def read_stu3_json(filename: str, delimiter: Optional[str] = None) -> List[str]:
  """Reads STU3 JSON information relative to the testdata/stu3 dir."""
  return read_data(os.path.join(_TEST_DATA_DIR_STU3, filename), delimiter)


def read_r4_json(filename: str, delimiter: Optional[str] = None) -> List[str]:
  """Reads R4 JSON information relative to the testdata/r4 dir."""
  return read_data(os.path.join(_TEST_DATA_DIR_R4, filename), delimiter)


def read_stu3_protos(filename: str,
                     proto_cls: Type[_T],
                     delimiter: Optional[str] = None) -> List[_T]:
  """Reads STU3 protobuf information relative to the testdata/stu3 dir."""
  return read_protos(
      os.path.join(_TEST_DATA_DIR_STU3, filename), proto_cls, delimiter)


def read_r4_protos(filename: str,
                   proto_cls: Type[_T],
                   delimiter: Optional[str] = None) -> List[_T]:
  """Reads R4 protobuf information relative to testdata/r4 dir."""
  return read_protos(
      os.path.join(_TEST_DATA_DIR_R4, filename), proto_cls, delimiter)
