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
"""Functionality for parsing FHIR JSON into protobuf format."""

import decimal
import json
from typing import Type, TypeVar

from google.protobuf import message
from google.fhir import _primitive_time_utils
from google.fhir import resource_validation
from google.fhir.json_format import _json_parser
from google.fhir.json_format import _json_printer
from google.fhir.r4 import primitive_handler

_T = TypeVar('_T', bound=message.Message)
_PRIMITIVE_HANDLER = primitive_handler.PrimitiveHandler()


def merge_json_fhir_string_into_proto(
    raw_json: str,
    target: message.Message,
    *,
    validate: bool = True,
    default_timezone: str = _primitive_time_utils.SIMPLE_ZULU):
  """Merges the provided raw_json string into a target Message.

  Args:
    raw_json: The JSON to parse and merge into target.
    target: The Message instance to merge raw_json into.
    validate: A Boolean value indicating if validation should be performed on
      the resultant Message. Validation takes the form of ensuring that basic
      checks such as cardinality guarantees, required field adherence, etc. are
      met. Defaults to True.
    default_timezone: A string specifying the timezone string to use for time-
      like FHIR data during parsing. Defaults to 'Z'.

  Raises:
    fhir_errors.InvalidFhirError: In the event that validation fails after
    parsing.
  """
  json_value = json.loads(
      raw_json, parse_float=decimal.Decimal, parse_int=decimal.Decimal)

  parser = _json_parser.JsonParser.json_parser_with_default_timezone(
      _PRIMITIVE_HANDLER, default_timezone=default_timezone)
  parser.merge_value(json_value, target)
  if validate:
    resource_validation.validate_resource(target, _PRIMITIVE_HANDLER)


def json_fhir_string_to_proto(
    raw_json: str,
    proto_cls: Type[_T],
    *,
    validate: bool = True,
    default_timezone: str = _primitive_time_utils.SIMPLE_ZULU) -> _T:
  """Creates a resource of proto_cls and merges contents of raw_json into it.

  Args:
    raw_json: The raw FHIR JSON string to convert.
    proto_cls: A subclass of message.Message to instantiate and return.
    validate: A Boolean value indicating if validation should be performed on
      the resultant Message. Validation takes the form of ensuring that basic
      checks such as cardinality guarantees, required field adherence, etc. are
      met. Defaults to True.
    default_timezone: A string specifying the timezone string to use for time-
      like FHIR data during parsing. Defaults to 'Z'.

  Raises:
    fhir_errors.InvalidFhirError: In the event that raw_json was not valid FHIR.

  Returns:
    An instance of proto_cls with FHIR JSON data from the raw_json
    representation.
  """
  resource = proto_cls()
  merge_json_fhir_string_into_proto(
      raw_json, resource, validate=validate, default_timezone=default_timezone)
  return resource


def pretty_print_fhir_to_json_string(fhir_proto: message.Message,
                                     *,
                                     indent_size: int = 2) -> str:
  """Returns a FHIR JSON representation with spaces and newlines.

  Args:
    fhir_proto: The proto to serialize into a "pretty" JSON string.
    indent_size: An integer denoting the size of space indentation for lexical
      scoping. Defaults to 2.

  Returns:
    A FHIR JSON string representation with spaces and newlines.
  """
  printer = _json_printer.JsonPrinter.pretty_printer(
      _PRIMITIVE_HANDLER, indent_size=indent_size)
  return printer.print(fhir_proto)


def print_fhir_to_json_string(fhir_proto: message.Message) -> str:
  """Returns a FHIR JSON representation with no spaces or newlines.

  Args:
    fhir_proto: The proto to serialize into a JSON string.

  Returns:
    A FHIR JSON representation with no spaces or newlines.
  """
  printer = _json_printer.JsonPrinter.compact_printer(_PRIMITIVE_HANDLER)
  return printer.print(fhir_proto)


def pretty_print_fhir_to_json_string_for_analytics(fhir_proto: message.Message,
                                                   *,
                                                   indent_size: int = 2) -> str:
  """Returns an Analytic FHIR JSON representation with spaces and newlines.

  Args:
    fhir_proto: The proto to serialize into a "pretty" JSON string.
    indent_size: An integer denoting the size of space indentation for lexical
      scoping. Defaults to 2.

  Returns:
    An Analytic FHIR JSON representation with spaces and newlines.
  """
  printer = _json_printer.JsonPrinter.pretty_printer_for_analytics(
      _PRIMITIVE_HANDLER, indent_size=indent_size)
  return printer.print(fhir_proto)


def print_fhir_to_json_string_for_analytics(fhir_proto: message.Message) -> str:
  """Returns an Analytic FHIR JSON representation with no spaces or newlines.

  Args:
    fhir_proto: The proto to serialize into a JSON string.

  Returns:
    An Analytic FHIR JSON representation with no spaces or newlines.
  """
  printer = _json_printer.JsonPrinter.compact_printer_for_analytics(
      _PRIMITIVE_HANDLER)
  return printer.print(fhir_proto)
