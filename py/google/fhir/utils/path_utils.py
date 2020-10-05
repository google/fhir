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
"""Utilities to make conversions to path(-like) expressions easier."""
import re

_CAMELCASE_BOUNDARY_REGEX = re.compile('(.)([A-Z][a-z0-9]+)')
_TRAILING_CAPITAL_REGEX = re.compile('([a-z0-9])([A-Z])')


def camel_case_to_snake_case(input_str: str) -> str:
  """Transforms a camelCase/UpperCamelCase string into a snake_case string."""
  tmp = _CAMELCASE_BOUNDARY_REGEX.sub(r'\1_\2', input_str)
  return _TRAILING_CAPITAL_REGEX.sub(r'\1_\2', tmp).lower()


def snake_case_to_camel_case(input_str: str, upper: bool = False) -> str:
  """Transforms a snake_case string into a camelCase/UpperCamelCase string.

  Args:
    input_str: The input string to transform.
    upper: If True, returns a representation in Pascal/UpperCamelCase. Defaults
      to False.

  Returns:
    A camelCase/UpperCamelCase string.
  """
  parts = input_str.split('_')
  if len(parts) == 1:
    return input_str
  result = parts[0] + ''.join([w[0].upper() + w[1:] for w in parts[1:]])
  return result[0].upper() + result[1:] if upper else result
