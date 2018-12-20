#
# Copyright 2018 Google LLC
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

"""Code responsible to encounter handling on FHIR Bundle.

Details: These functions/generators operates on encounters either to
get data elements from encounter or yeild eligible encounter from
FHIR bundle.
"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from datetime import datetime
from datetime import timedelta


from proto.stu3 import codes_pb2
from proto.stu3 import datatypes_pb2
from proto.stu3 import resources_pb2


ENCOUNTER_CLASS_CODESYSTEM = 'http://hl7.org/fhir/v3/ActCode'
CLASS_INPATIENT = 'IMP'
SECS_PER_HOUR = 3600
SECS_PER_DAY = 3600 * 24


def ToTime(date_and_time):
  """Get utc seconds from FHIR DateTime.

  Args:
    date_and_time: FHIR DateTime proto in datatypes.proto.
      date_and_time.value_us is unix microseconds in UTC timezone.

  Returns:
    datetime.datetime in seconds precision, UTC timezone.
  """
  return datetime.utcfromtimestamp(date_and_time.value_us / 1000000)


def EncounterIsFinished(encounter):
  return (encounter.period.HasField('start') and
          encounter.period.HasField('end') and
          encounter.status.value ==
          codes_pb2.EncounterStatusCode.FINISHED)


def EncounterIsValidHospitalization(encounter):
  enc_class = encounter.class_value
  return (EncounterIsFinished(encounter) and
          enc_class.system.value == ENCOUNTER_CLASS_CODESYSTEM and
          enc_class.code.value == CLASS_INPATIENT)


def EncounterIsValidHospitalizationForSynthea(
    encounter):
  enc_class = encounter.class_value
  return (EncounterIsFinished(encounter) and
          enc_class.code.value == 'inpatient')


def AtDuration(encounter,
               hours):
  # encounter.start + hours
  result = ToTime(encounter.period.start) + timedelta(hours=hours)
  assert result <= ToTime(encounter.period.end)
  return result


def EncounterLengthDays(encounter):
  # Needs a float to properly put encounters in ranges.
  length_delta = ToTime(encounter.period.end) - ToTime(encounter.period.start)
  return float(length_delta.total_seconds()) / SECS_PER_DAY


def GetPatient(
    bundle):
  for entry in bundle.entry:
    if entry.resource.HasField('patient'):
      return entry.resource.patient
  return None


###############################################
# Use generator to be memory efficient.
#
def AllEncounters(bundle):
  """Yields all encounters in a bundle.

  Args:
    bundle: Bundle proto.
  Yields:
    all encounters in a bundle.
  """
  for entry in bundle.entry:
    if entry.resource.HasField('encounter'):
      yield entry.resource.encounter


def InpatientEncounters(bundle, for_synthea=False):
  """Yields all inpatient encounters in a bundle.

  Args:
    bundle: Bundle proto.
    for_synthea: Whether to use ENCOUNTER_CLASS_CODESYSTEM or string 'inpatient'
      as code value for encounter class (for synthea).
  Yields:
    all inpatient encounters in a bundle.
  """
  for encounter in AllEncounters(bundle):
    if (not for_synthea) & EncounterIsValidHospitalization(encounter):
      yield encounter
    elif for_synthea & EncounterIsValidHospitalizationForSynthea(encounter):
      yield encounter


def InpatientEncountersLongerThan(bundle, n_hours, for_synthea=False):
  """Yields all inpatient encounters in a bundle that is longer than N hours.

  Args:
    bundle: Bundle proto.
    n_hours: min duration of the encounter.
    for_synthea: Whether to use ENCOUNTER_CLASS_CODESYSTEM or string 'inpatient'
      as code value for encounter class (for synthea).
  Yields:
    all inpatient encounters in a bundle that is longer than N hours.
  """
  for encounter in InpatientEncounters(bundle, for_synthea):
    if (ToTime(encounter.period.end) - ToTime(encounter.period.start) >
        timedelta(seconds=n_hours * SECS_PER_HOUR)):
      yield encounter


# One line wrapper for 24.
def Inpatient24HrEncounters(bundle, for_synthea=False):
  return InpatientEncountersLongerThan(bundle, 24, for_synthea)
