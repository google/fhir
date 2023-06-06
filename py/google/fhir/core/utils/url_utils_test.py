#
# Copyright 2022 Google LLC
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
"""Test url_utils functionality."""
from typing import Optional, Tuple

from absl.testing import absltest
from absl.testing import parameterized
from google.fhir.core.utils import url_utils


class UrlUtilsTest(parameterized.TestCase):

  @parameterized.named_parameters(
      dict(
          testcase_name='with_version_suffix_succeeds',
          url='http://hl7.org/fhir/ValueSet/value-set|1.0',
          expected_parse=('http://hl7.org/fhir/ValueSet/value-set', '1.0'),
      ),
      dict(
          testcase_name='with_no_version_suffix_succeeds',
          url='http://hl7.org/fhir/ValueSet/value-set',
          expected_parse=('http://hl7.org/fhir/ValueSet/value-set', None),
      ),
  )
  def test_parse_url_version(
      self, url: str, expected_parse: Tuple[str, Optional[str]]
  ):
    self.assertEqual(url_utils.parse_url_version(url), expected_parse)

  def test_filter_urls_to_domains_with_filter_succeeds(self):
    urls = ('http://hl7.org/fhir/ValueSet/financial-taskcode',
            'http://loinc.org/fhir/ValueSet/loing-taskcode')
    result = url_utils.filter_urls_to_domains(urls, ['loinc.org'])
    self.assertListEqual(
        list(result), ['http://loinc.org/fhir/ValueSet/loing-taskcode'])


if __name__ == '__main__':
  absltest.main()
