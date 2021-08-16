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
from typing import List

import logging


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
    raise NotImplementedError(
        'Subclasses *must* override `report_conversion_error`.')

  @abc.abstractmethod
  def report_validation_error(self, element_path: str, msg: str) -> None:
    """Reports the given error during FHIR validation.

    This indicates that the resource does not fully comply with the FHIR
    specification or profile.

    Args:
      element_path: The path to the field where the issue occurred.
      msg: The error message produced.
    """
    raise NotImplementedError(
        'Subclasses *must* override `report_validation_error`.')

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
    raise NotImplementedError(
        'Subclasses *must* override `report_validation_warning`.')

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
    raise NotImplementedError(
        'Subclasses *must* override `report_fhir_path_error`.')

  @abc.abstractmethod
  def report_fhir_path_warning(self, element_path: str,
                               fhir_path_constraint: str, msg: str) -> None:
    """Reports a FHIRPath constraint warning during validation and/or encoding.

    Args:
      element_path: The path to the field that the constraint is defined on.
      fhir_path_constraint: The FHIRPath constraint expression.
      msg: The warning message produced.
    """
    raise NotImplementedError(
        'Subclasses *must* override `report_fhir_path_warning`.')


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
    logging.error('%s; %s', element_path, msg)
    self.errors.append(msg)

  def report_validation_error(self, element_path: str, msg: str) -> None:
    """Logs to the `error` context and stores `msg` in `errors`."""
    logging.error('%s; %s', element_path, msg)
    self.errors.append(msg)

  def report_validation_warning(self, element_path: str, msg: str) -> None:
    """Logs to the `warning` context and stores `msg` in `warnings`."""
    logging.warning('%s; %s', element_path, msg)
    self.warnings.append(msg)

  def report_fhir_path_error(self, element_path: str, fhir_path_constraint: str,
                             msg: str) -> None:
    """Logs to the `error` context and stores `msg` in `errors`."""
    logging.error('%s:%s; %s', element_path, fhir_path_constraint, msg)
    self.errors.append(msg)

  def report_fhir_path_warning(self, element_path: str,
                               fhir_path_constraint: str, msg: str) -> None:
    """Logs to the `warning` context and stores `msg` in `warnings`."""
    logging.warning('%s:%s; %s', element_path, fhir_path_constraint, msg)
    self.warnings.append(msg)
