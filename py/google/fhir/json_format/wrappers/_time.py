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
"""PrimitiveWrapper subclass for Time primitives."""

import datetime
from typing import Any, Callable, Dict, Tuple, Type, TypeVar

from google.protobuf import message
from google.fhir import _primitive_time_utils
from google.fhir import fhir_errors
from google.fhir.json_format.wrappers import _primitive_wrappers
from google.fhir.utils import proto_utils

_FORMAT_FUNCS: Dict[_primitive_time_utils.TimePrecision,
                    Callable[[datetime.time], str]] = {
                        _primitive_time_utils.TimePrecision.SECOND:
                            lambda t: t.strftime('%H:%M:%S'),
                        # Remove micro-precision
                        _primitive_time_utils.TimePrecision.MILLISECOND:
                            lambda t: t.strftime('%H:%M:%S.%f')[:-3],
                        _primitive_time_utils.TimePrecision.MICROSECOND:
                            lambda t: t.strftime('%H:%M:%S.%f'),
                    }

Time = TypeVar('Time', bound=message.Message)


def _parse(json_str: str, primitive_cls: Type[Time]) -> Time:
  """Parses the json_str into a Time FHIR primitive.

  Args:
    json_str: The raw JSON string to parse.
    primitive_cls: The FHIR primitive to parse into.

  Returns:
    A FHIR primitive Time instance.

  Raises:
    fhir_errors.InvalidFhirError: In the event that no FHIR primitive Time
    format was able to properly parse the json_str.
  """
  try:
    time = datetime.datetime.strptime(json_str, '%H:%M:%S').time()
    return _primitive_time_utils.build_time(
        time, _primitive_time_utils.TimePrecision.MICROSECOND.SECOND,
        primitive_cls)
  except ValueError:
    pass  # Fall through

  try:
    time = datetime.datetime.strptime(json_str, '%H:%M:%S.%f').time()
    if (_primitive_time_utils.PRECISION_PATTERN_MILLISECOND.search(json_str) is
        not None):
      return _primitive_time_utils.build_time(
          time, _primitive_time_utils.TimePrecision.MILLISECOND, primitive_cls)
    elif (_primitive_time_utils.PRECISION_PATTERN_MICROSECOND.search(json_str)
          is not None):
      return _primitive_time_utils.build_time(
          time, _primitive_time_utils.TimePrecision.MICROSECOND, primitive_cls)
  except ValueError:
    pass  # Fall through

  raise fhir_errors.InvalidFhirError(f'Invalid Time: {json_str!r}.')


class TimeWrapper(_primitive_wrappers.PrimitiveWrapper):
  """A wrapper around the Time FHIR primitive type."""

  _PARSABLE_TYPES: Tuple[Type[Any], ...] = (str,)

  @classmethod
  def from_json_str(cls, json_str: str, primitive_cls: Type[Time],
                    context: _primitive_wrappers.Context) -> 'TimeWrapper':
    """See PrimitiveWrapper.from_json_str."""
    _primitive_wrappers.validate_primitive_json_representation(
        primitive_cls.DESCRIPTOR, json_str)
    time = _parse(json_str, primitive_cls)
    return cls(time, context)

  def _nonnull_string_value(self) -> str:
    precision: int = proto_utils.get_value_at_field(self.wrapped, 'precision')
    f = _FORMAT_FUNCS.get(precision)
    if f is None:
      raise fhir_errors.InvalidFhirError(
          f'No format string for precision: {precision!r}.')

    # The wrapped value represents the time (based on a 24-hour clock) in micro-
    # seconds since 00:00:00.000000. In order to convert this to a datetime.time
    # object for formatting, we need to use a datetime.timedelta to compute the
    # time "offset" represented by self.wrapped.
    #
    # A timedelta object can only be added to a datetime. Note that the base
    # datetime created by datetime.combine() has no associated tzinfo (e.g. it
    # is "naive"), and therefore concepts like DST, timezones, etc. will not
    # affect this calculation.
    base_time = datetime.datetime.combine(_primitive_time_utils.UNIX_EPOCH,
                                          datetime.time())
    value_us: int = proto_utils.get_value_at_field(self.wrapped, 'value_us')
    delta = datetime.timedelta(microseconds=value_us)
    if delta.days > 0:
      raise fhir_errors.InvalidFhirError(
          f'Time value of {value_us!r}us is >= 24 hours.')

    time_value = (base_time + delta).time()
    return f(time_value)
