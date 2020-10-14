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
"""Common functionality for version-specific json_format tests."""

import collections
import copy
import decimal
import json
from typing import Any, Callable, Optional, Type, TypeVar

from google.protobuf import message
from absl.testing import parameterized
from google.fhir.testing import testdata_utils

_T = TypeVar('_T', bound=message.Message)


_JsonFormatTestdata = collections.namedtuple('_JsonFormatTestdata',
                                             ['json_strs', 'protos'])


class JsonFormatTest(parameterized.TestCase):
  """A base class for testing FHIR version-specific json_format modules."""

  def _read_json_and_protos(
      self,
      json_path: str,
      proto_path: str,
      proto_cls: Type[message.Message],
      *,
      json_delimiter: Optional[str] = None,
      proto_delimiter: Optional[str] = None) -> _JsonFormatTestdata:
    """Convenience method for returning collections of testdata."""
    json_strs = testdata_utils.read_data(json_path, delimiter=json_delimiter)

    # We filter out 'null' values here, as the proto-parsing has no delimiter/
    # methodology for loading a 'null' .prototxt representation. This value is
    # simply absent in the corresponding .prototxt file.
    nonnull_json_strs = [
        json_str for json_str in json_strs if json_str != 'null'
    ]
    protos = testdata_utils.read_protos(
        proto_path, proto_cls, delimiter=proto_delimiter)
    if len(nonnull_json_strs) != len(protos):
      raise ValueError(
          f'Cannot compare {len(nonnull_json_strs)} .json representations '
          f'against {len(protos)} .prototxt representations.')
    return _JsonFormatTestdata(nonnull_json_strs, protos)

  def assert_print_equals_golden(self,
                                 json_path: str,
                                 proto_path: str,
                                 proto_cls: Type[message.Message],
                                 *,
                                 print_f: Callable[..., str],
                                 json_delimiter: Optional[str] = None,
                                 proto_delimiter: Optional[str] = None,
                                 **print_kwargs: Any):
    """Compare printer output against 'golden' file.

    Note that we perform a comparison between Python native types after calling
    into json.loads(...), as diffing raw strings can have minor differences in
    spaces that are inconsequential to the underlying representations.

    If json_delimiter and proto_delimiter are supplied, the cardinality of the
    resulting sequences must match exactly or an error will be thrown.

    Args:
      json_path: The filepath to the .json file (loaded as a 'golden').
      proto_path: The filepath to the .prototxt file (loaded as a 'test case').
      proto_cls: The type of protobuf message to serialize to and print from
        (type under test).
      print_f: The print function to execute and examine.
      json_delimiter: An optional delimiter for the .json file to load multiple
        representations. Defaults to None.
      proto_delimiter: An optional delimiter for the .prototxt file to load
        multiple representations. Defaults to None.
      **print_kwargs: An optional list of key/value arguments to supply to the
        print function.
    """
    testdata = self._read_json_and_protos(
        json_path,
        proto_path,
        proto_cls,
        json_delimiter=json_delimiter,
        proto_delimiter=proto_delimiter)

    for (json_str, proto) in zip(testdata.json_strs, testdata.protos):
      # Golden (expected)
      from_json = json.loads(
          json_str, parse_int=decimal.Decimal, parse_float=decimal.Decimal)

      # Test case
      orig_proto = copy.deepcopy(proto)
      raw_json_str = print_f(proto, **print_kwargs)
      from_proto = json.loads(
          raw_json_str, parse_int=decimal.Decimal, parse_float=decimal.Decimal)

      self.assertEqual(proto, orig_proto)  # Ensure print did not mutate proto
      self.assertEqual(from_json, from_proto)

  def assert_parse_equals_golden(self,
                                 json_path: str,
                                 proto_path: str,
                                 proto_cls: Type[message.Message],
                                 *,
                                 parse_f: Callable[..., message.Message],
                                 json_delimiter: Optional[str] = None,
                                 proto_delimiter: Optional[str] = None,
                                 **parse_kwargs: Any):
    """Compare parser output against 'golden' file.

    Note that we perform a comparison between protobuf representations.

    If json_delimiter and proto_delimiter are supplied, the cardinality of the
    resulting sequences must match exactly or an error will be thrown.

    Args:
      json_path: The filepath to the .json file (loaded as a 'test case').
      proto_path: The filepath to the .prototxt file (loaded as a 'golden').
      proto_cls: The type of protobuf message to parse into.
      parse_f: The function responsible for parsing FHIR JSON to exmaine.
      json_delimiter: An optional delimiter for the .json file to load multiple
        representations. Defaults to None.
      proto_delimiter: An optional delimiter for the .prototxt file to load
        multiple representations. Defaults to None.
      **parse_kwargs: Optional key/value arguments to supply to parse_f.
    """
    testdata = self._read_json_and_protos(
        json_path,
        proto_path,
        proto_cls,
        json_delimiter=json_delimiter,
        proto_delimiter=proto_delimiter)

    for (json_str, proto) in zip(testdata.json_strs, testdata.protos):
      # Test case
      from_json = parse_f(json_str, proto_cls, **parse_kwargs)

      # Golden (expected)
      self.assertEqual(from_json, proto)
