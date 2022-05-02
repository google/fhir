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
"""A collection of functions for dealing with FHIR resource URLs."""

from typing import Collection, Iterable, Optional, Tuple
import urllib.parse


def parse_url_version(url: str) -> Tuple[str, Optional[str]]:
  """Parses the FHIR resource URL into its URL and version.

  For URLs ending with with a |[version] suffix, decompose them into URL and
  version components. For details, see
  https://www.hl7.org/fhir/elementdefinition-definitions.html#ElementDefinition.binding.valueSet

  Args:
    url: The URL to parse.

  Returns:
    A tuple of (URL, version) parsed from the given URL. If there is no
    |[version] component on the given URL, version will be `None`.
  """
  url_parts = url.rsplit('|', 1)
  if len(url_parts) == 1:
    version = None
  else:
    url, version = url_parts
  return url, version


def filter_urls_to_domains(
    urls: Iterable[str], restrict_to_domains: Collection[str]) -> Iterable[str]:
  """Filters a collection of URLs to those within the given set of domains.

  Args:
    urls: The URLs to filter.
    restrict_to_domains: The domains to limit URLs to, e.g. {'hl7.org'}

  Yields:
    The URLs filtered to only those within the given set of domains.
  """
  for url in urls:
    if urllib.parse.urlparse(url).netloc in restrict_to_domains:
      yield url
