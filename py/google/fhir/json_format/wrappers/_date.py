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
"""PrimitiveWrapper subclass for Date primitives."""

import datetime
from typing import cast, Any, Callable, Dict, Tuple, Type, TypeVar

from google.protobuf import message
from google.fhir import _primitive_time_utils
from google.fhir import fhir_errors
from google.fhir.json_format.wrappers import _primitive_wrappers
from google.fhir.utils import proto_utils

_FORMAT_FUNCS: Dict[_primitive_time_utils.DateTimePrecision,
                    Callable[[datetime.datetime], str]] = {
                        _primitive_time_utils.DateTimePrecision.YEAR:
                            lambda dt: dt.strftime('%Y'),
                        _primitive_time_utils.DateTimePrecision.MONTH:
                            lambda dt: dt.strftime('%Y-%m'),
                        _primitive_time_utils.DateTimePrecision.DAY:
                            lambda dt: dt.strftime('%Y-%m-%d'),
                    }

Date = TypeVar('Date', bound=message.Message)


def _parse(json_str: str, primitive_cls: Type[Date], *,
           default_timezone: str) -> Date:
  """Parses the json_str into a Date FHIR primitive.

  Args:
    json_str: The raw JSON string to parse.
    primitive_cls: The FHIR primitive to parse into.
    default_timezone: The default timezone to use when parsing in the event that
      no timezone information is present.

  Returns:
    A FHIR primitive Date.

  Raises:
    fhir_errors.InvalidFhirError: In the event that no datetime format was
    able to properly parse the json_str.
  """
  try:
    dt = datetime.datetime.strptime(json_str, '%Y')
    return _primitive_time_utils.build_date_like(
        dt, default_timezone, _primitive_time_utils.DateTimePrecision.YEAR,
        primitive_cls)
  except ValueError:
    pass  # Fall through

  try:
    dt = datetime.datetime.strptime(json_str, '%Y-%m')
    return _primitive_time_utils.build_date_like(
        dt, default_timezone, _primitive_time_utils.DateTimePrecision.MONTH,
        primitive_cls)
  except ValueError:
    pass  # Fall through

  try:
    dt = datetime.datetime.strptime(json_str, '%Y-%m-%d')
    return _primitive_time_utils.build_date_like(
        dt, default_timezone, _primitive_time_utils.DateTimePrecision.DAY,
        primitive_cls)
  except ValueError:
    pass  # Fall through

  raise fhir_errors.InvalidFhirError('Invalid Date.')


class DateWrapper(_primitive_wrappers.PrimitiveWrapper):
  """A wrapper around the Date FHIR primitive type."""

  _PARSABLE_TYPES: Tuple[Type[Any], ...] = (str,)

  @classmethod
  def from_json_str(cls, json_str: str, primitive_cls: Type[message.Message],
                    context: _primitive_wrappers.Context) -> 'DateWrapper':
    """See _primitive_wrappers.PrimitiveWrapper.from_json_str."""
    _primitive_wrappers.validate_primitive_json_representation(
        primitive_cls.DESCRIPTOR, json_str)
    date = _parse(
        json_str, primitive_cls, default_timezone=context.default_timezone)
    return cls(date, context)

  def _nonnull_string_value(self) -> str:
    timezone: str = proto_utils.get_value_at_field(self.wrapped, 'timezone')
    if not timezone:
      raise fhir_errors.InvalidFhirError('Date missing timezone.')

    precision: int = proto_utils.get_value_at_field(self.wrapped, 'precision')
    f = _FORMAT_FUNCS.get(precision)
    if f is None:
      raise fhir_errors.InvalidFhirError('Invalid precision on Date.')

    tzinfo = _primitive_time_utils.timezone_info_for_timezone(timezone)
    delta = datetime.timedelta(
        microseconds=cast(
            int, proto_utils.get_value_at_field(self.wrapped, 'value_us')))
    dt_value = (_primitive_time_utils.UNIX_EPOCH + delta).astimezone(tzinfo)
    return f(dt_value)
