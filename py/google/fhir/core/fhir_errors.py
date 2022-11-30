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
"""Defines FHIR-specific Python exceptions."""

import abc
import collections
from typing import List, Tuple

import logging


def _create_event_frequency_table(frequency_list: List[Tuple[str, int]]) -> str:
  """Returns a string depicting unique log events and their counts."""
  result: List[str] = []
  max_event_len = max(len(key) for key, _ in frequency_list)

  for event, count in frequency_list:
    result.append(f'{event:<{max_event_len}}:   {count}')
  return '\n'.join(result)


def aggregate_events(events: List[str]) -> List[Tuple[str, int]]:
  """Returns a list of tuples: (event string, count of times they appear).

  The list is sorted descending by count and then ascending by event string.

  Args:
    events: A list of strings defining an event (either an error or a warning).

  Returns:
    List of tuples : (event string, number of times they appear).
  """
  frequency_list = collections.Counter(events)
  # Sort descending by count then ascending by event string.
  return sorted(frequency_list.items(), key=lambda x: (-x[1], x[0]))


class InvalidFhirError(Exception):
  """Invalid FHIR data was encountered."""
  pass


class ErrorReporter(abc.ABC):
  """An abstract base class for FHIRPath encoding errors."""

  @abc.abstractmethod
  def report_conversion_error(self, element_path: str, msg: str) -> None:
    """Reports the given error during FHIR conversion.

    This indicates that the resource does not fully comply with the FHIR
    specification or profile, and the field could not be converted to the target
    structure. Data may have been lost during the conversion.

    Args:
      element_path: The path to the field where the issue occurred.
      msg: The error message produced.
    """

  @abc.abstractmethod
  def report_validation_error(self, element_path: str, msg: str) -> None:
    """Reports the given error during FHIR validation.

    This indicates that the resource does not fully comply with the FHIR
    specification or profile.

    Args:
      element_path: The path to the field where the issue occurred.
      msg: The error message produced.
    """

  @abc.abstractmethod
  def report_validation_warning(self, element_path: str, msg: str) -> None:
    """Reports the given warning during FHIR validation.

    This indicates that the element complies with the FHIR specification, but
    may be missing some desired-but-not-required property, like additional
    fields that are useful to consumers.

    Args:
      element_path: The path to the field where the issue occurred.
      msg: The warning message that was produced.
    """

  @abc.abstractmethod
  def report_fhir_path_error(self, element_path: str, fhir_path_constraint: str,
                             msg: str) -> None:
    """Reports a FHIRPath constraint error during validation and/or encoding.

    The base implementation logs to the `error` context and raises `e` by
    default. Subclasses should override this behavior as necessary.

    Args:
      element_path: The path to the field that the constraint is defined on.
      fhir_path_constraint: The FHIRPath constraint expression.
      msg: The error message produced.
    """

  @abc.abstractmethod
  def report_fhir_path_warning(self, element_path: str,
                               fhir_path_constraint: str, msg: str) -> None:
    """Reports a FHIRPath constraint warning during validation and/or encoding.

    Args:
      element_path: The path to the field that the constraint is defined on.
      fhir_path_constraint: The FHIRPath constraint expression.
      msg: The warning message produced.
    """


class ListErrorReporter(ErrorReporter):
  """A delegate for FHIRPath encoding errors.

  Errors are logged to the corresponding `logging` context (e.g. "warning" or
  "error") and any encountered messages are stored in the corresponding
  attribute (`warnings` and `errors`, respectively). These can then be retrieved
  by the caller within the context of the larger system.

  Attributes:
    errors: A list of error messages encountered.
    warnings: A list of warning messages encountered.
  """

  def __init__(self) -> None:
    self.errors: List[str] = []
    self.warnings: List[str] = []

  def report_conversion_error(self, element_path: str, msg: str) -> None:
    """Logs to the `error` context and stores `msg` in `errors`."""
    full_msg = f'Conversion Error: {element_path}; {msg}'
    logging.error(full_msg)
    self.errors.append(full_msg)

  def report_validation_error(self, element_path: str, msg: str) -> None:
    """Logs to the `error` context and stores `msg` in `errors`."""
    full_msg = f'Validation Error: {element_path}; {msg}'
    logging.error(full_msg)
    self.errors.append(full_msg)

  def report_validation_warning(self, element_path: str, msg: str) -> None:
    """Logs to the `warning` context and stores `msg` in `warnings`."""
    full_msg = f'Validation Warning: {element_path}; {msg}'
    logging.warning(full_msg)
    self.warnings.append(full_msg)

  def report_fhir_path_error(self, element_path: str, fhir_path_constraint: str,
                             msg: str) -> None:
    """Logs to the `error` context and stores `msg` in `errors`."""
    full_msg = _build_fhir_path_message('Error', element_path,
                                        fhir_path_constraint, msg)
    logging.error(full_msg)
    self.errors.append(full_msg)

  def report_fhir_path_warning(self, element_path: str,
                               fhir_path_constraint: str, msg: str) -> None:
    """Logs to the `warning` context and stores `msg` in `warnings`."""
    full_msg = _build_fhir_path_message('Warning', element_path,
                                        fhir_path_constraint, msg)
    logging.warning(full_msg)
    self.warnings.append(full_msg)

  def get_error_report(self) -> str:
    """Returns an aggregated report of warnings and errors encountered."""

    report = ''
    if self.errors:
      errors_freq_tbl = _create_event_frequency_table(
          aggregate_events(self.errors))
      report += f'Encountered {len(self.errors)} errors:\n{errors_freq_tbl}'

    if self.warnings:
      warnings_freq_tbl = _create_event_frequency_table(
          aggregate_events(self.warnings))
      report += (f'\n\nEncountered {len(self.warnings)} warnings:\n'
                 f'{warnings_freq_tbl}')

    return report


def _build_fhir_path_message(level: str, element_path: str,
                             fhir_path_constraint: str, msg: str) -> str:
  """Builds a FHIR Path error message from the given components."""
  return (f'FHIR Path {level}: {element_path + "; " if element_path else ""}'
          f'{fhir_path_constraint}; {msg}')
