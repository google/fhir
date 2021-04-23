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
"""Functions for dealing with time-/date-like FHIR primitives."""

import datetime
import enum
import re

from typing import cast, Any, Tuple, Type, TypeVar

from dateutil import parser
from dateutil import tz
from google.protobuf import message
from google.fhir.utils import proto_utils

# google-fhir supports multiple <major>.<minor>.x interpreters. If unable to
# import zoneinfo from stdlib, fallback to the backports package. See more at:
# https://pypi.org/project/backports.zoneinfo/.
try:
  import zoneinfo  # pytype: disable=import-error
except ImportError:
  from backports import zoneinfo

_T = TypeVar('_T', bound=message.Message)

_TIMEZONE_OFFSET_PATTERN = re.compile(
    r'(Z|(\+|-)((0[0-9]|1[0-3]):[0-5][0-9]|14:00))$')
PRECISION_PATTERN_MICROSECOND = re.compile(
    r'([01][0-9]|2[0-3]):[0-5][0-9]:([0-5][0-9]|60)\.[0-9]{4,}$')
PRECISION_PATTERN_MILLISECOND = re.compile(
    r'([01][0-9]|2[0-3]):[0-5][0-9]:([0-5][0-9]|60)\.[0-9]{1,3}$')
SIMPLE_ZULU = 'Z'
UNIX_EPOCH = datetime.datetime(1970, 1, 1, tzinfo=tz.UTC)


class DateTimePrecision(enum.IntEnum):
  """FHIR version-abstract precision for Date* primitives."""
  UNSPECIFIED = 0
  YEAR = 1
  MONTH = 2
  DAY = 3
  SECOND = 4
  MILLISECOND = 5
  MICROSECOND = 6


class TimePrecision(enum.IntEnum):
  """FHIR version-abstract precision for Time/Instant primitives."""
  UNSPECIFIED = 0
  SECOND = 1
  MILLISECOND = 2
  MICROSECOND = 3


def build_date_like(dt: datetime.datetime, tzstr: str,
                    precision: DateTimePrecision,
                    primitive_cls: Type[_T]) -> _T:
  """Constructs an instance of Date/DateTime given a naive Python datetime."""
  tzinfo = timezone_info_for_timezone(tzstr)
  aware_dt = dt.replace(tzinfo=tzinfo).astimezone(tz.UTC)
  delta = (aware_dt - UNIX_EPOCH)

  # Note: Per the Python documentation, `total_seconds` will lose microsecond
  # precision for very large time intervals (greater than 270 years on most
  # platforms).
  value_us = int(delta.total_seconds() * 1e6)
  result = cast(Any, primitive_cls)(
      value_us=value_us,
      timezone=tzstr,
      precision=cast(Any, primitive_cls).Precision.Value(precision.name))
  return result


def build_time(time: datetime.time, precision: TimePrecision,
               primitive_cls: Type[_T]) -> _T:
  """Constructs an instance of Time given a Python time object."""
  total_s = time.second + 60 * time.minute + 3600 * time.hour
  total_us = time.microsecond + 1e6 * total_s
  return cast(Any, primitive_cls)(value_us=int(total_us), precision=precision)


def get_date_time_value(primitive: message.Message) -> datetime.datetime:
  """Returns a Python-native datetime.datetime from the wrapped primitive."""
  tzinfo = timezone_info_for_timezone(
      cast(str, proto_utils.get_value_at_field(primitive, 'timezone')))
  delta = datetime.timedelta(
      microseconds=cast(int,
                        proto_utils.get_value_at_field(primitive, 'value_us')))
  return (UNIX_EPOCH + delta).astimezone(tzinfo)


def get_duration_from_precision(
    precision: DateTimePrecision) -> datetime.timedelta:
  """Returns a timedelta reflecting the minimum unit based on precision."""
  # TODO: Handle YEAR and MONTH precision properly
  if precision == DateTimePrecision.YEAR:
    return datetime.timedelta(days=366)
  elif precision == DateTimePrecision.MONTH:
    return datetime.timedelta(days=31)
  elif precision == DateTimePrecision.DAY:
    return datetime.timedelta(hours=24)
  elif precision == DateTimePrecision.SECOND:
    return datetime.timedelta(seconds=1)
  elif precision == DateTimePrecision.MILLISECOND:
    return datetime.timedelta(milliseconds=1)
  elif precision == DateTimePrecision.MICROSECOND:
    return datetime.timedelta(microseconds=1)
  else:
    raise ValueError(
        f'Unsupported datetime precision: {DateTimePrecision(precision)!r}.')


def get_upper_bound(primitive: message.Message,
                    precision: DateTimePrecision) -> datetime.datetime:
  """Returns the value of the wrapped primitive plus '1 unit' of duration."""
  return get_date_time_value(primitive) + get_duration_from_precision(precision)


# TODO: Look into a cleaner method of restoring the UTC timezone
def restore_utc_timezone(datetime_str: str, original_timezone: str) -> str:
  """Restores UTC '+00:00' offset with 'Z' or '-00:00' if necessary.

  This is necessary because dateutil will replace the 'Z' and '-00:00' timezones
  with '+00:00', which is irreversible.

  Args:
    datetime_str: The datetime string to examine.
    original_timezone: The original timezone string on the primitive.

  Returns:
    The restored datetime_str. Note that this will remain unchanged if there was
    no UTC timezone replacement that occurred.
  """
  if datetime_str.endswith('+00:00'):
    if (original_timezone == SIMPLE_ZULU or original_timezone == '-00:00'):
      return datetime_str.replace('+00:00', original_timezone, 1)
    if original_timezone == 'UTC':
      return datetime_str.replace('+00:00', SIMPLE_ZULU, 1)
  return datetime_str


def split_timezone(raw_datetime_str: str) -> Tuple[str, str]:
  """Given a raw FHIR datetime string, splits into datetime and timezone."""
  match = _TIMEZONE_OFFSET_PATTERN.search(raw_datetime_str)
  if match is None:
    raise ValueError(f'Unable to find timezone in {raw_datetime_str!r}')
  start, end = match.span()
  return raw_datetime_str[:start] + raw_datetime_str[end:], match.group()


def timezone_info_for_timezone(tzstr: str) -> datetime.tzinfo:
  """Returns a datetime.tzinfo subclass for a timezone string."""
  offset_err = None
  tzname_err = None

  try:
    isoparser = parser.isoparser()
    return isoparser.parse_tzstr(tzstr)
  except ValueError as e:
    offset_err = e  # Fall through

  try:
    return cast(datetime.tzinfo, zoneinfo.ZoneInfo(tzstr))
  except ValueError as e:
    tzname_err = e  # Fall through

  raise ValueError(
      f'Cannot serialize timezone: {tzstr!r} ({offset_err!r}, {tzname_err!r}).')
