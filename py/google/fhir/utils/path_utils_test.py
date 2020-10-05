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
"""Tests path_utils functionality."""

from google.fhir.utils import path_utils
from absl.testing import absltest


class PathUtilsTest(absltest.TestCase):
  """Unit tests for methods in path_util.py."""

  def testCamelCaseToSnakeCase(self):
    """Tests camelCase to snake_case conversion."""
    self.assertEqual("", path_utils.camel_case_to_snake_case(""))
    self.assertEqual("apple", path_utils.camel_case_to_snake_case("apple"))
    self.assertEqual("orange", path_utils.camel_case_to_snake_case("Orange"))
    self.assertEqual("apple_orange",
                     path_utils.camel_case_to_snake_case("appleOrange"))
    self.assertEqual("apple_orange",
                     path_utils.camel_case_to_snake_case("AppleOrange"))

  def testSnakeCaseToCamelCase(self):
    """Tests snake_case to camelCase conversion."""
    self.assertEqual("", path_utils.snake_case_to_camel_case(""))
    self.assertEqual("apple", path_utils.snake_case_to_camel_case("apple"))
    self.assertEqual("applePie",
                     path_utils.snake_case_to_camel_case("apple_pie"))


if __name__ == "__main__":
  absltest.main()
