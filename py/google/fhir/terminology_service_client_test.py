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
"""Test terminology_service_client functionality."""

import unittest.mock

from absl.testing import absltest
from proto.google.fhir.proto.r4.core.resources import value_set_pb2
from google.fhir import terminology_service_client


class TerminologyServiceClientTest(absltest.TestCase):

  @unittest.mock.patch.object(
      terminology_service_client.requests, 'Session', autospec=True)
  def testValueSetExpansion_withValueSet_retrievesCodes(self, mock_session):
    mock_session().headers = {}
    mock_session().__enter__().get('url').json.side_effect = [
        {
            'resourceType': 'ValueSet',
            'id': 'vs-id',
            'url': 'vs-url',
            'status': 'draft',
            'expansion': {
                'timestamp': '2022-02-14T15:51:40-05:00',
                'offset': 0,
                'total': 2,
                'contains': [{
                    'code': 'code-1'
                }]
            }
        },
        {
            'resourceType': 'ValueSet',
            'id': 'vs-id',
            'url': 'vs-url',
            'status': 'draft',
            'expansion': {
                'timestamp': '2022-02-14T15:51:40-05:00',
                'offset': 1,
                'total': 2,
                'contains': [{
                    'code': 'code-2'
                }]
            }
        },
    ]
    # Reset to hide the get('url') call we made above.
    mock_session().__enter__().get.reset_mock()

    value_set = value_set_pb2.ValueSet()
    value_set.id.value = '2.16'
    value_set.url.value = 'http://cts.nlm.nih.gov/fhir/ValueSet/2.16'

    client = terminology_service_client.TerminologyServiceClient(
        {'https://cts.nlm.nih.gov/fhir/': 'the-api-key'})
    result = client.expand_value_set(value_set)

    # Ensure we called requests correctly.
    self.assertEqual(mock_session().__enter__().get.call_args_list, [
        unittest.mock.call(
            'https://cts.nlm.nih.gov/fhir/r4/ValueSet/2.16/$expand',
            params={'offset': 0}),
        unittest.mock.call(
            'https://cts.nlm.nih.gov/fhir/r4/ValueSet/2.16/$expand',
            params={'offset': 1}),
    ])
    self.assertEqual(mock_session().auth, ('apikey', 'the-api-key'))
    self.assertEqual(mock_session().headers['Accept'], 'application/json')

    # Ensure we got the expected protos back.
    expected = [
        value_set_pb2.ValueSet.Expansion.Contains(),
        value_set_pb2.ValueSet.Expansion.Contains(),
    ]
    expected[0].code.value = 'code-1'
    expected[1].code.value = 'code-2'
    self.assertCountEqual(result.expansion.contains, expected)

  @unittest.mock.patch.object(
      terminology_service_client.requests, 'Session', autospec=True)
  def testValueSetExpansionFromTerminologyService_withNoPagination_retrievesCodes(
      self, mock_session):
    mock_session().headers = {}
    mock_session().__enter__().get('url').json.return_value = {
        'resourceType': 'ValueSet',
        'id': 'vs-id',
        'url': 'vs-url',
        'status': 'draft',
        'expansion': {
            'timestamp': '2022-02-14T15:51:40-05:00',
            'contains': [{
                'code': 'code-1'
            }]
        }
    }
    # Reset to hide the get('url') call we made above.
    mock_session().__enter__().get.reset_mock()

    value_set = value_set_pb2.ValueSet()
    value_set.id.value = 'the-id'
    value_set.url.value = 'http://hl7.org/fhir/ValueSet/financial-taskcode'

    client = terminology_service_client.TerminologyServiceClient({})
    result = client.expand_value_set(value_set)

    # Ensure we called requests correctly.
    mock_session().__enter__().get.assert_called_once_with(
        'https://tx.fhir.org/r4/ValueSet/the-id/$expand', params={'offset': 0})
    self.assertEqual(mock_session().headers['Accept'], 'application/json')

    # Ensure we got the expected protos back.
    expected = [
        value_set_pb2.ValueSet.Expansion.Contains(),
    ]
    expected[0].code.value = 'code-1'
    self.assertCountEqual(result.expansion.contains, expected)

  def testValueSetExpansionFromTerminologyService_withMissingId_raisesError(
      self):
    value_set = value_set_pb2.ValueSet()

    with self.assertRaises(ValueError):
      client = terminology_service_client.TerminologyServiceClient({})
      client.expand_value_set(value_set)

  def testValueSetExpansionFromTerminologyService_withBadUrl_raisesError(self):
    value_set = value_set_pb2.ValueSet()
    value_set.id.value = 'an-id'
    value_set.url.value = 'mystery-url'

    with self.assertRaises(ValueError):
      client = terminology_service_client.TerminologyServiceClient({})
      client.expand_value_set(value_set)

  def testInit_withBadServerKey_raisesError(self):
    with self.assertRaises(ValueError):
      terminology_service_client.TerminologyServiceClient(
          {'mystery-server': 'api-key'})

  @unittest.mock.patch.object(
      terminology_service_client, 'requests', autospec=True)
  def testSessionWithBackoff_withRequests_AddsAdapter(self, mock_requests):
    terminology_service_client._session_with_backoff()

    mock_requests.adapters.HTTPAdapter.assert_called_once_with(
        max_retries=mock_requests.packages.urllib3.util.Retry())
    mock_requests.Session().mount.assert_has_calls([
        unittest.mock.call('http://', mock_requests.adapters.HTTPAdapter()),
        unittest.mock.call('https://', mock_requests.adapters.HTTPAdapter()),
    ])


if __name__ == '__main__':
  absltest.main()
