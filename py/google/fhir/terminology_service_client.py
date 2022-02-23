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
"""Provides a client for interacting with terminology servers."""

import copy
from typing import Dict, List, Optional, Sequence
import urllib.parse

import requests
import requests.adapters
import requests.packages

from proto.google.fhir.proto.r4.core.resources import value_set_pb2
from google.fhir.r4 import json_format

TERMINOLOGY_BASE_URL_PER_DOMAIN = {
    'hl7.org': 'https://tx.fhir.org',
    'terminology.hl7.org': 'https://tx.fhir.org',
    'cts.nlm.nih.gov': 'https://cts.nlm.nih.gov/fhir/',
}


class TerminologyServiceClient:
  """Client for interacting with terminology servers.

  Attributes:
    api_keys_per_terminology_server: The API key to use for each terminology
      server. The keys of this dictionary should be the values of the
      TERMINOLOGY_BASE_URL_PER_DOMAIN dictionary. If the terminology server does
      not require an API key to access, the entry for that server may be omitted
      from api_keys_per_terminology_server or given a value of None.
  """

  def __init__(self, api_keys_per_terminology_server: Dict[str, str]) -> None:
    allowed_servers = set(TERMINOLOGY_BASE_URL_PER_DOMAIN.values())
    unknown_servers = [
        key for key in api_keys_per_terminology_server.keys()
        if key not in allowed_servers
    ]
    if unknown_servers:
      raise ValueError(
          'Unexpected server(s) in api_keys_per_terminology_server: %s. '
          'Must be one of %s' %
          (', '.join(unknown_servers), ', '.join(allowed_servers)))

    self.api_keys_per_terminology_server = api_keys_per_terminology_server

  def expand_value_set(
      self, value_set: value_set_pb2.ValueSet) -> value_set_pb2.ValueSet:
    """Expands the value set using a terminology server.

    Builds a copy of the given value set with its expansion field expanded to
    contain all the codes composing the value set. Appends new codes from
    expansion to any codes currently in the value set's expansion field.

    Args:
      value_set: The value set for which to retrieve expanded codes.

    Returns:
      A copy of the value set with its code expansion fields set.
    """
    value_set_id = value_set.id.value
    value_set_url = value_set.url.value
    if not value_set_id or not value_set_url:
      raise ValueError('Value set must have both an ID and a URL. '
                       'Unable to make API calls for value set.')

    value_set_domain = urllib.parse.urlparse(value_set_url).netloc
    base_url = TERMINOLOGY_BASE_URL_PER_DOMAIN.get(value_set_domain)
    if base_url is None:
      raise ValueError(
          'Unknown domain %s. Can not find appropriate terminology server.' %
          value_set_domain)

    api_key = self.api_keys_per_terminology_server.get(base_url)

    codes = _get_expansion_contains(value_set_id, base_url, api_key)
    expanded_value_set = copy.deepcopy(value_set)
    expanded_value_set.expansion.contains.extend(codes)
    return expanded_value_set


def _get_expansion_contains(
    value_set_id: str,
    base_url: str,
    api_key: Optional[str] = None
) -> Sequence[value_set_pb2.ValueSet.Expansion.Contains]:
  """Expands codes for the value set using the given terminology server."""
  offset = 0
  codes: List[value_set_pb2.ValueSet.Expansion.Contains] = []
  request_url = urllib.parse.urljoin(base_url,
                                     f'r4/ValueSet/{value_set_id}/$expand')

  session_ = _session_with_backoff()
  session_.headers.update({'Accept': 'application/json'})
  if api_key is not None:
    session_.auth = ('apikey', api_key)

  with session_ as session:
    while True:
      resp = session.get(request_url, params={'offset': offset})
      resp.raise_for_status()
      resp_json = resp.json()

      response_value_set = json_format.json_fhir_object_to_proto(
          resp_json, value_set_pb2.ValueSet)
      codes.extend(response_value_set.expansion.contains)

      # See if we need to paginate through more results. The 'total' attribute
      # may be absent if pagination is not being used. If it is present, see if
      # we need to retrieve more results.
      offset += len(resp_json['expansion']['contains'])
      if 'total' not in resp_json['expansion'] or (
          offset >= resp_json['expansion']['total']):
        return codes


def _session_with_backoff():
  """Builds a request session with exponential back-off retries."""
  session = requests.Session()
  retry_policy = requests.packages.urllib3.util.Retry(backoff_factor=2)
  adapter = requests.adapters.HTTPAdapter(max_retries=retry_policy)
  session.mount('http://', adapter)
  session.mount('https://', adapter)
  return session
