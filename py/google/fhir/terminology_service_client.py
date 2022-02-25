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

from typing import Dict, List
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

    Requests an expansion of the value set from the appropriate terminology
    server for the value set's URL and version if present on the value set.
    Retrieves the current definition of the value set from the terminology
    service as well as its expansion.

    Args:
      value_set: The value set to expand.

    Returns:
      The current definition of the value set from the server with its expanded
      codes present.
    """
    if not value_set.url.value:
      raise ValueError('Value set must have a URL. '
                       'Unable to make API calls for value set.')

    value_set_domain = urllib.parse.urlparse(value_set.url.value).netloc
    base_url = TERMINOLOGY_BASE_URL_PER_DOMAIN.get(value_set_domain)
    if base_url is None:
      raise ValueError(
          'Unknown domain %s. Can not find appropriate terminology server.' %
          value_set_domain)

    api_key = self.api_keys_per_terminology_server.get(base_url)

    offset = 0
    codes: List[value_set_pb2.ValueSet.Expansion.Contains] = []

    request_url = urllib.parse.urljoin(base_url, 'r4/ValueSet/$expand')
    params = {'url': value_set.url.value}
    if value_set.version.value:
      params['valueSetVerson'] = value_set.version.value

    session_ = _session_with_backoff()
    session_.headers.update({'Accept': 'application/json'})
    if api_key is not None:
      session_.auth = ('apikey', api_key)

    with session_ as session:
      while True:
        resp = session.get(request_url, params={'offset': offset, **params})
        resp.raise_for_status()
        resp_json = resp.json()

        response_value_set = json_format.json_fhir_object_to_proto(
            resp_json, value_set_pb2.ValueSet)
        codes.extend(response_value_set.expansion.contains)

        # See if we need to paginate through more results. The 'total' attribute
        # may be absent if pagination is not being used. If it is present, see
        # if we need to retrieve more results.
        offset += len(resp_json['expansion']['contains'])
        if 'total' not in resp_json['expansion'] or (
            offset >= resp_json['expansion']['total']):

          # Protocol buffers don't support assignment to slices
          # (i.e. contains[:] = codes) so we delete and extend.
          del response_value_set.expansion.contains[:]
          response_value_set.expansion.contains.extend(codes)
          return response_value_set


def _session_with_backoff():
  """Builds a request session with exponential back-off retries."""
  session = requests.Session()
  retry_policy = requests.packages.urllib3.util.Retry(backoff_factor=2)
  adapter = requests.adapters.HTTPAdapter(max_retries=retry_policy)
  session.mount('http://', adapter)
  session.mount('https://', adapter)
  return session
