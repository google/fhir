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

from typing import Callable, Dict, List, Tuple
import urllib.parse

import logging
import requests
import requests.adapters
import requests.packages

from proto.google.fhir.proto.r4.core.resources import value_set_pb2
from google.fhir.r4 import json_format
from google.fhir.utils import url_utils

TERMINOLOGY_BASE_URL_PER_DOMAIN = {
    'hl7.org': 'https://tx.fhir.org/r4/',
    'terminology.hl7.org': 'https://tx.fhir.org/r4/',
    'loinc.org': 'https://fhir.loinc.org',
}


class TerminologyServiceClient:
  """Client for interacting with terminology servers.

  Attributes:
    auth_per_terminology_server: The basic auth values to use when communicating
      with each terminology server. The keys of this dictionary should be the
      values of the TERMINOLOGY_BASE_URL_PER_DOMAIN dictionary. The values
      should be tuples of (username, password) strings for use in basic auth. If
      the terminology server does not require an authorization to access, the
      entry for that server may be omitted from api_keys_per_terminology_server
      or given a value of None.
  """

  def __init__(self,
               auth_per_terminology_server: Dict[str, Tuple[str, str]]) -> None:
    allowed_servers = set(TERMINOLOGY_BASE_URL_PER_DOMAIN.values())
    unknown_servers = [
        key for key in auth_per_terminology_server.keys()
        if key not in allowed_servers
    ]
    if unknown_servers:
      raise ValueError(
          'Unexpected server(s) in api_keys_per_terminology_server: %s. '
          'Must be one of %s' %
          (', '.join(unknown_servers), ', '.join(allowed_servers)))

    self.auth_per_terminology_server = auth_per_terminology_server

  def expand_value_set_url(self, value_set_url: str) -> value_set_pb2.ValueSet:
    """Expands the value set using a terminology server.

    Requests an expansion of the value set from the appropriate terminology
    server for the given URL and version if present on the URL.
    Retrieves the current definition of the value set from the terminology
    service as well as its expansion.

    Args:
      value_set_url: The url of the value set to expand.

    Returns:
      The current definition of the value set from the server with its expanded
      codes present.
    """
    value_set_url, value_set_version = url_utils.parse_url_version(
        value_set_url)

    value_set_domain = urllib.parse.urlparse(value_set_url).netloc
    base_url = TERMINOLOGY_BASE_URL_PER_DOMAIN.get(value_set_domain)
    if base_url is None:
      raise ValueError(
          'Unknown domain %s. Can not find appropriate terminology server.' %
          value_set_domain)

    request_url = urllib.parse.urljoin(base_url, 'ValueSet/$expand')
    params = {'url': value_set_url}
    if value_set_version is not None:
      params['valueSetVersion'] = value_set_version

    session_ = _session_with_backoff()
    session_.headers.update({'Accept': 'application/json'})

    auth = self.auth_per_terminology_server.get(base_url)
    if auth is not None:
      session_.auth = auth

    logging.info(
        'Expanding value set url: %s version: %s using terminology service: %s',
        value_set_url, value_set_version, base_url)
    with session_ as session:

      def request_func(offset: int) -> requests.Response:
        return session.get(request_url, params={'offset': offset, **params})

      expanded_value_set = _paginate_expand_value_set_request(request_func)

    logging.info(
        'Retrieved %d codes for value set url: %s version: %s '
        'using terminology service: %s',
        len(expanded_value_set.expansion.contains), value_set_url,
        value_set_version, base_url)
    return expanded_value_set

  def expand_value_set_definition(
      self, value_set: value_set_pb2.ValueSet) -> value_set_pb2.ValueSet:
    """Expands the value set definition using a terminology server.

    Requests an expansion of the given value set from the appropriate
    terminology server. Attempts to expand arbitrary value sets by passing their
    entire definition to the terminology service for expansion.

    If possible, requests expansion from the domain associated with the value
    set's URL. If the value set URL is not associated with a known terminology
    service, uses the tx.fhir.org service as it is able to expand value sets
    defined outside its own specifications.

    Retrieves the current definition of the value set from the terminology
    service as well as its expansion.

    Args:
      value_set: The value set to expand.

    Returns:
      The current definition of the value set from the server with its expanded
      codes present.
    """
    value_set_domain = urllib.parse.urlparse(value_set.url.value).netloc
    base_url = TERMINOLOGY_BASE_URL_PER_DOMAIN.get(value_set_domain,
                                                   'https://tx.fhir.org/r4/')

    request_url = urllib.parse.urljoin(base_url, 'ValueSet/$expand')
    request_json = json_format.print_fhir_to_json_string(value_set).encode(
        'utf-8')

    session_ = _session_with_backoff()
    session_.headers.update({
        'Accept': 'application/json',
        'Content-Type': 'application/json'
    })

    auth = self.auth_per_terminology_server.get(base_url)
    if auth is not None:
      session_.auth = auth

    logging.info(
        'Expanding value set url: %s version: %s using terminology service: %s',
        value_set.url.value, value_set.version.value, base_url)
    with session_ as session:

      def request_func(offset: int) -> requests.Response:
        return session.post(
            request_url, data=request_json, params={'offset': offset})

      expanded_value_set = _paginate_expand_value_set_request(request_func)

    logging.info(
        'Retrieved %d codes for value set url: %s version: %s '
        'using terminology service: %s',
        len(expanded_value_set.expansion.contains), value_set.url.value,
        value_set.version.value, base_url)
    return expanded_value_set


def _paginate_expand_value_set_request(
    request_func: Callable[[int], requests.Response]) -> value_set_pb2.ValueSet:
  """Performs a request to the terminology service, including pagination.

  Given a function which performs a request against a terminology service, use
  the function to make requests until the full response has been paginated
  through.

  Args:
    request_func: The function to call to perform a request to the terminology
      service. The function must accept an integer representing the pagination
      offset value to include in the request and return a requests Response
      object.

  Returns:
    The current definition of the value set from the server with its expanded
    codes present.
  """
  offset = 0
  codes: List[value_set_pb2.ValueSet.Expansion.Contains] = []
  while True:
    resp = request_func(offset)

    if resp.status_code >= 400:
      logging.error('Error from terminology service: %s', resp.text)
    resp.raise_for_status()

    resp_json = resp.json()
    response_value_set = json_format.json_fhir_object_to_proto(
        resp_json, value_set_pb2.ValueSet, validate=False)
    codes.extend(response_value_set.expansion.contains)

    # See if we need to paginate through more results. The 'total' attribute
    # may be absent if pagination is not being used. If it is present, see
    # if we need to retrieve more results.
    offset += len(resp_json['expansion'].get('contains', ()))
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
