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
"""Utilities for working with Value Sets."""

import copy
import itertools
from typing import Iterable, List, Optional, Set, Sequence, Tuple

import logging
import sqlalchemy

from proto.google.fhir.proto.r4.core.resources import code_system_pb2
from proto.google.fhir.proto.r4.core.resources import structure_definition_pb2
from proto.google.fhir.proto.r4.core.resources import value_set_pb2
from google.fhir import terminology_service_client
from google.fhir.utils import fhir_package
from google.fhir.utils import proto_utils
from google.fhir.utils import url_utils


class ValueSetResolver:
  """Utility for retrieving and resolving value set resources to the codes they contain.

  Attributes:
    package_manager: The FhirPackageManager object to use when retrieving
      resource definitions. The FhirPackage objects contained in package_manager
      will be consulted when value set resource definitions are needed. The
      package manager should contain common resources, for instance, ones from
      the US Core implementation guide, in order to ensure definitions for all
      relevant value sets may be found. If a requisite value set definition is
      not present in the package manager, the resolver will throw an error
      instead of attempting to retrieve it over the network.
  """

  def __init__(self, package_manager: fhir_package.FhirPackageManager) -> None:
    self.package_manager = package_manager

  def value_set_urls_from_fhir_package(
      self, package: fhir_package.FhirPackage) -> Iterable[str]:
    """Retrieves URLs for all value sets referenced by the given FHIR package.

    Finds all value set resources in the package as well as any value sets
    referenced by structure definitions in the package.

    Args:
      package: The FHIR package from which to retrieve value sets.

    Yields:
      URLs for all value sets referenced by the FHIR package.
    """
    value_set_urls_from_structure_definitions = itertools.chain.from_iterable(
        self.value_set_urls_from_structure_definition(structure_definition)
        for structure_definition in package.structure_definitions)
    value_set_urls_from_value_sets = (
        value_set.url.value for value_set in package.value_sets)
    all_value_set_urls = itertools.chain(
        value_set_urls_from_value_sets,
        value_set_urls_from_structure_definitions,
    )
    yield from _unique_urls(all_value_set_urls)

  def value_set_urls_from_structure_definition(
      self, structure_definition: structure_definition_pb2.StructureDefinition
  ) -> Iterable[str]:
    """Retrieves URLs for value sets referenced by the structure definition.

    Finds the union of value sets bound to any element from either the
    differential or snapshot definition.

    Args:
      structure_definition: The structure definition from which to retrieve
        value sets.

    Yields:
      URLs for all value sets referenced by the structure definition.
    """
    elements = itertools.chain(structure_definition.differential.element,
                               structure_definition.snapshot.element)
    value_set_urls = (
        element.binding.value_set.value
        for element in elements
        if element.binding.value_set.value)

    if structure_definition.url.value == (
        'http://hl7.org/fhir/StructureDefinition/ExplanationOfBenefit'):
      # A bug in the FHIR spec has this structure definition bound to a code
      # system by mistake. It should be bound to a value set instead. We swap
      # the URLs until the bug is addressed.
      # https://jira.hl7.org/browse/FHIR-36128
      bad_code_system_url = 'http://terminology.hl7.org/CodeSystem/processpriority'
      correct_value_set_url = 'http://hl7.org/fhir/ValueSet/process-priority'
      value_set_urls = (
          correct_value_set_url if url == bad_code_system_url else url
          for url in value_set_urls)

    yield from _unique_urls(value_set_urls)

  def expand_value_set_url(
      self, url: str,
      terminology_client: terminology_service_client.TerminologyServiceClient
  ) -> value_set_pb2.ValueSet:
    """Retrieves the expanded value set definition for the given URL.

    Attempts to expand the value set using definitions available to the
    instance's package manager. If the expansion can not be performed with
    available resources, makes network calls to a terminology service to perform
    the expansion.

    Args:
      url: The URL of the value set to expand.
      terminology_client: The client to use when using a terminology service to
        expand the value set.

    Returns:
      A value set protocol buffer expanded to include the codes it represents.
    """
    value_set = self.value_set_from_url(url)
    if value_set is not None:
      expanded_value_set = self._expand_value_set_locally(value_set)
      if expanded_value_set is not None:
        return expanded_value_set

    return terminology_client.expand_value_set_url(url)

  def value_set_from_url(self, url: str) -> Optional[value_set_pb2.ValueSet]:
    """Retrieves the value set for the given URL.

    The value set is assumed to be a member of one of the packages contained in
    self.package_manager. This function will not attempt to look up resources
    over the network in other locations.

    Args:
      url: The url of the value set to retrieve.

    Returns:
      The value set for the given URL or None if it can not be found in the
      package manager.

    Raises:
      ValueError: If the URL belongs to a resource that is not a value set.
    """
    url, version = url_utils.parse_url_version(url)
    value_set = self.package_manager.get_resource(url)
    if value_set is None:
      logging.info(
          'Unable to find value set for url: %s in given resolver packages.',
          url)
      return None
    elif not isinstance(value_set, value_set_pb2.ValueSet):
      raise ValueError('URL: %s does not refer to a value set, found: %s' %
                       (url, value_set.DESCRIPTOR.name))
    elif version is not None and version != value_set.version.value:
      logging.warning(
          'Found incompatible version for value set with url: %s. Requested: %s, found: %s',
          url, version, value_set.version.value)
      return None
    else:
      return value_set

  def _expand_value_set_locally(
      self,
      value_set: value_set_pb2.ValueSet) -> Optional[value_set_pb2.ValueSet]:
    """Attempts to expand value sets without contacting a terminology service.

    For value sets with an extensional set of codes, collect all codes
    referenced in the value set's 'compose' field.

    For value sets which reference a code system without specifying the
    extensional set of codes within it, attempt to find the definition of the
    code system in the ValueSetResolver and expand to all codes in the code
    system. If the code system resource is not available locally, returns `None`
    indicating a terminology service should instead be used to find the value
    set expansion.

    If the value set has an intensional set of codes, returns `None` indicating
    a terminology service should instead be used to find the value set
    expansion.

    See https://www.hl7.org/fhir/valueset.html#int-ext
    for more details about value set expansion and intensional versus
    extensional value sets.

    Args:
      value_set: The value set for which to retrieve expanded codes.

    Returns:
      The expanded value set or None if a terminology service should be
      consulted instead.
    """
    concept_sets = itertools.chain(value_set.compose.include,
                                   value_set.compose.exclude)
    if any(concept_set.filter for concept_set in concept_sets):
      # The value set requires intensional filtering rules we do not implement.
      # We may wish to reduce the frequency with which we need to defer to
      # external terminology services.
      # TODO: Add more support for filtering logic.
      return None

    includes = [
        self._concept_set_to_expansion(value_set, include)
        for include in value_set.compose.include
    ]
    excludes = [
        self._concept_set_to_expansion(value_set, exclude)
        for exclude in value_set.compose.exclude
    ]

    if None in includes or None in excludes:
      # The value set references code system definitions unavailable locally.
      return None

    logging.info('Expanding value set url: %s version: %s locally',
                 value_set.url.value, value_set.version.value)
    includes = itertools.chain.from_iterable(includes)
    excludes = itertools.chain.from_iterable(excludes)

    # Build tuples of the fields to use for equality when testing if a code from
    # include is also in exclude.
    codes_to_remove = set(
        (concept.version.value, concept.system.value, concept.code.value)
        for concept in excludes)
    # Use tuples of the same form to filter excluded codes.
    codes = [
        concept for concept in includes
        if (concept.version.value, concept.system.value,
            concept.code.value) not in codes_to_remove
    ]

    expanded_value_set = copy.deepcopy(value_set)
    expanded_value_set.expansion.contains.extend(codes)
    return expanded_value_set

  def _concept_set_to_expansion(
      self, value_set: value_set_pb2.ValueSet,
      concept_set: value_set_pb2.ValueSet.Compose.ConceptSet
  ) -> Optional[Sequence[value_set_pb2.ValueSet.Expansion.Contains]]:
    """Expands the ConceptSet into a collection of Expansion.Contains objects.

    Args:
      value_set: The value set for which the concept set is being expanded.
      concept_set: The concept set to expand.

    Returns:
      The expansion represented by the concept set or None if the expansion
      requires a code system definition not present locally.
    """
    if concept_set.concept:
      # If given, take the expansion as the list of given codes.
      concepts = concept_set.concept
    else:
      # No explicit codes list is given, meaning we should take the expansion as
      # the entire code system.
      logging.info('Expanding to entire code system: %s',
                   concept_set.system.value)
      code_system = self.package_manager.get_resource(concept_set.system.value)

      if code_system is None:
        logging.warning(
            'Expansion of code system: %s for value set: %s version: %s '
            'requires code system definition not available locally. Deferring '
            'expansion to external terminology service.',
            concept_set.system.value, value_set.url.value,
            value_set.version.value)
        return None
      elif not isinstance(code_system, code_system_pb2.CodeSystem):
        raise ValueError(
            'system: %s does not refer to a code system, found: %s' %
            (concept_set.system.value, code_system.DESCRIPTOR.name))
      else:
        concepts = code_system.concept

    expansion: List[value_set_pb2.ValueSet.Expansion.Contains] = []
    for concept in concepts:
      contains = value_set_pb2.ValueSet.Expansion.Contains()
      for field, copy_from in (
          ('system', concept_set),
          ('version', concept_set),
          ('code', concept),
          ('display', concept),
      ):
        proto_utils.copy_common_field(copy_from, contains, field)

      for designation in concept.designation:
        designation_copy = contains.designation.add()
        for field in ('id', 'extension', 'modifier_extension', 'language',
                      'use', 'value'):
          proto_utils.copy_common_field(designation, designation_copy, field)

      expansion.append(contains)
    return expansion


def valueset_codes_insert_statement_for(
    value_sets: Iterable[value_set_pb2.ValueSet],
    table: sqlalchemy.sql.expression.TableClause,
    batch_size: int = 500) -> Iterable[sqlalchemy.sql.dml.Insert]:
  """Builds INSERT statements for placing value sets' codes into a given table.

  The INSERT may be used to build a valueset_codes table as described by:
  https://github.com/FHIR/sql-on-fhir/blob/master/sql-on-fhir.md#valueset-support

  Returns an sqlalchemy insert expression which inserts all of the value set's
  expanded codes into the given table which do not already exist in the table.
  The query will avoid inserting duplicate rows if some of the codes are already
  present in the given table. It will not attempt to perform an 'upsert' or
  modify any existing rows.

  Args:
    value_sets: The expanded value sets with codes to insert into the given
      table. The value sets should have already been expanded by the
      ValueSetResolver.expand_value_set_url method.
    table: The SqlAlchemy table to receive the INSERT. May be an sqlalchemy
      Table or TableClause object. The table is assumed to have the columns
      'valueseturi', 'valuesetversion', 'system', 'code.'
    batch_size: The maximum number of rows to insert in a single query.

  Yields:
    The sqlalchemy insert expressions which you may execute to perform the
    actual database writes. Each yielded insert expression will insert at most
    batch_size number of rows.
  """

  def value_set_codes() -> Iterable[Tuple[
      value_set_pb2.ValueSet, value_set_pb2.ValueSet.Expansion.Contains]]:
    """Yields (value_set, code) tuples for each code in each value set."""
    for value_set in value_sets:
      if not value_set.expansion.contains:
        logging.warning('Value set: %s version: %s has no expanded codes',
                        value_set.url.value, value_set.version.value)
      for code in value_set.expansion.contains:
        yield value_set, code

  # Break the value set codes into batches.
  batch_iterables = [iter(value_set_codes())] * batch_size
  batches = itertools.zip_longest(*batch_iterables)

  for batch in batches:
    # Build a SELECT statement for each code.
    code_literals = []
    for pair in batch:
      # The last batch will have `None`s padding it out to `batch_size`.
      if pair is not None:
        value_set, code = pair
        code_literals.append(_code_as_select_literal(value_set, code))

    # UNION each SELECT to build a single select subquery for all codes.
    codes = sqlalchemy.union_all(*code_literals).alias('codes')
    # Filter the codes to those not already present in `table` with a LEFT JOIN.
    new_codes = sqlalchemy.select((codes,)).select_from(
        codes.outerjoin(
            table,
            sqlalchemy.and_(
                codes.c.valueseturi == table.c.valueseturi,
                codes.c.valuesetversion == table.c.valuesetversion,
                codes.c.system == table.c.system,
                codes.c.code == table.c.code,
            ))).where(
                sqlalchemy.and_(
                    table.c.valueseturi.is_(None),
                    table.c.valuesetversion.is_(None),
                    table.c.system.is_(None),
                    table.c.code.is_(None),
                ))
    yield table.insert().from_select(new_codes.columns, new_codes)


def _code_as_select_literal(
    value_set: value_set_pb2.ValueSet,
    code: value_set_pb2.ValueSet.Expansion.Contains) -> sqlalchemy.select:
  """Builds a SELECT statement for the literals in the given code."""
  return sqlalchemy.select((
      sqlalchemy.sql.expression.literal(
          value_set.url.value).label('valueseturi'),
      sqlalchemy.sql.expression.literal(
          value_set.version.value).label('valuesetversion'),
      sqlalchemy.sql.expression.literal(code.system.value).label('system'),
      sqlalchemy.sql.expression.literal(code.code.value).label('code'),
  ))


def _unique_urls(urls: Iterable[str]) -> Iterable[str]:
  """Filters URLs to remove duplicates.

  Args:
    urls: The URLs to filter.

  Yields:
    The URLs filtered to only those without duplicates.
  """
  seen: Set[str] = set()

  for url in urls:
    if url not in seen:
      seen.add(url)
      yield url
