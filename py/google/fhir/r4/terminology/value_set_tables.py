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
"""Utilities for maintaining value set codes database tables.

Read more about value set codes tables here:
https://github.com/FHIR/sql-on-fhir/blob/master/sql-on-fhir.md#valueset-support
"""

import itertools
from typing import Iterable, Tuple, Union

import logging
from google.cloud import bigquery
import sqlalchemy
import sqlalchemy_bigquery

from proto.google.fhir.proto.r4.core.resources import value_set_pb2
from google.fhir.core.utils import url_utils
from google.fhir.r4.terminology import terminology_service_client
from google.fhir.r4.terminology import value_sets


def valueset_codes_insert_statement_for(
    expanded_value_sets: Iterable[value_set_pb2.ValueSet],
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
    expanded_value_sets: The expanded value sets with codes to insert into the
      given table. The value sets should have already been expanded, for
      instance by a ValueSetResolver or TerminologyServiceClient's
      expand_value_set_url method.
    table: The SqlAlchemy table to receive the INSERT. May be an sqlalchemy
      Table or TableClause object. The table is assumed to have the columns
      'valueseturi', 'valuesetversion', 'system', 'code.'
    batch_size: The maximum number of rows to insert in a single query.

  Yields:
    The sqlalchemy insert expressions which you may execute to perform the
    actual database writes. Each yielded insert expression will insert at most
    batch_size number of rows.
  Raises:
    ValueError: If the given table does not have the expected columns.
  """
  expected_cols = ('valueseturi', 'valuesetversion', 'system', 'code')
  missing_cols = [col for col in expected_cols if col not in table.columns]
  if missing_cols:
    raise ValueError('Table %s missing expected columns: %s' %
                     (table, ', '.join(missing_cols)))

  def value_set_codes() -> Iterable[Tuple[
      value_set_pb2.ValueSet, value_set_pb2.ValueSet.Expansion.Contains]]:
    """Yields (value_set, code) tuples for each code in each value set."""
    for value_set in expanded_value_sets:
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


def materialize_value_set_expansion(
    urls: Iterable[str],
    expander: Union[terminology_service_client.TerminologyServiceClient,
                    value_sets.ValueSetResolver],
    engine: Union[bigquery.Client, sqlalchemy.engine.base.Engine],
    table: Union[str, sqlalchemy.sql.expression.TableClause],
    batch_size: int = 500) -> None:
  """Expands a sequence of value set and materializes their expanded codes.

  Expands the given value set URLs to obtain the set of codes they describe.
  Then writes these expanded codes into the given database table using the given
  sqlalchemy engine. Builds a valueset_codes table as described by
  https://github.com/FHIR/sql-on-fhir/blob/master/sql-on-fhir.md#valueset-support

  The function will avoid inserting duplicate rows if some of the codes are
  already present in the given table. It will not attempt to perform an 'upsert'
  or modify any existing rows.

  Provided as a utility function for user convenience. If `urls` is a large set
  of URLs, callers may prefer to use multi-processing and/or multi-threading to
  perform expansion and table insertion of the URLs concurrently. This function
  performs all expansions and table insertions serially.

  Args:
    urls: The urls for value sets to expand and materialize.
    expander: The ValueSetResolver or TerminologyServiceClient to perform value
      set expansion. A ValueSetResolver may be used to attempt to avoid some
      network requests by expanding value sets locally. A
      TerminologyServiceClient will use external terminology services to perform
      all value set expansions.
    engine: Either an SQLAlchemy database engine or a BigQuery Client to use
      when writing expanded value sets to `table`.
    table: The database table to be written. May be a string representing the
      qualified table name or an SQLAlchemy Table or TableClause object. The
      table is assumed to have the columns 'valueseturi', 'valuesetversion',
      'system', 'code.'
    batch_size: The maximum number of rows to insert in a single query.
  """
  if isinstance(table, str):
    if isinstance(engine, sqlalchemy.engine.base.Engine):
      # Use the engine to reflect the table definition.
      table = sqlalchemy.Table(
          table,
          sqlalchemy.MetaData(bind=engine),
          autoload=True,
      )
    elif isinstance(engine, bigquery.Client):
      # Build a table object with the columns we expect the table to have.
      table = sqlalchemy.table(
          table,
          sqlalchemy.column('valueseturi'),
          sqlalchemy.column('valuesetversion'),
          sqlalchemy.column('system'),
          sqlalchemy.column('code'),
      )

  expanded_value_sets = (expander.expand_value_set_url(url) for url in urls)
  queries = valueset_codes_insert_statement_for(
      expanded_value_sets, table, batch_size=batch_size)

  if isinstance(engine, sqlalchemy.engine.base.Engine):
    with engine.connect() as connection:
      for query in queries:
        connection.execute(query)
  elif isinstance(engine, bigquery.Client):
    # Render the query objects as strings and use the client to execute them.
    for query in queries:
      query_string = str(
          query.compile(
              dialect=(sqlalchemy_bigquery.BigQueryDialect()),
              compile_kwargs={'literal_binds': True}))
      engine.query(query_string).result()


def count_code_systems_within_valueset_query(
    value_set_url: str,
    table: sqlalchemy.sql.expression.TableClause) -> sqlalchemy.sql.Select:
  """Builds a query for the number of code systems within a value set.

  Returns an sqlalchemy SELECT query for the number of code systems within the
  given value set. If the value set url contains a |version suffix, the query
  will report the number of code systems for that version of the value set. If
  the url does not contain a |version suffix, the number of systems for any
  version of the value set will be reported.

  Args:
    value_set_url: The value set to retrieve counts for.
    table: The SqlAlchemy table to query.

  Returns:
    A sqlalchemy SELECT query which may be executed for the code systems count.
  """
  url, version = url_utils.parse_url_version(value_set_url)

  query = sqlalchemy.select([
      sqlalchemy.func.count(sqlalchemy.distinct(table.c.system))
  ]).where(table.c.valueseturi == url)
  if version:
    query = query.where(table.c.valuesetversion == version)

  return query


def _code_as_select_literal(
    value_set: value_set_pb2.ValueSet,
    code: value_set_pb2.ValueSet.Expansion.Contains) -> sqlalchemy.select:
  """Builds a SELECT statement for the literals in the given code."""
  return sqlalchemy.select((
      _literal_or_null(value_set.url.value).label('valueseturi'),
      _literal_or_null(value_set.version.value).label('valuesetversion'),
      _literal_or_null(code.system.value).label('system'),
      _literal_or_null(code.code.value).label('code'),
  ))


def _literal_or_null(val: str) -> sqlalchemy.sql.elements.ColumnElement:
  """Returns a literal for the given string or NULL for an empty string."""
  if val:
    return sqlalchemy.sql.expression.literal(val)
  else:
    return sqlalchemy.null()
