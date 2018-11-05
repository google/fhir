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

"""Code responsible to generate label on FHIR Bundle.

This example shows how to define a multi-class label for length of stay (LOS)
of all inpatient encounters.

Cohort: Inpatient encounters with valid start and end date and length of stay
is greater than 24 hours.

Trigger time: 24 hours after admission.

Labels: divide LOS into following ranges: (1, 3], (3, 7], (7, 14], (14, inf)
in terms of days.
"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import datetime


from proto.stu3 import datatypes_pb2
from proto.stu3 import google_extensions_pb2
from proto.stu3 import resources_pb2
from py.google.fhir.labels import encounter


GOOGLE_FHIR_LABEL_SYTEM = 'urn:google:fhir:label'
LOS_RANGE_LABEL = 'length_of_stay_range'

# This can be easily changed to for different bucketing.
# boundaries needs to be sorted in ascending order.
LOS_BOUNDARIES = [3, 7, 14]


def ExtractCodeBySystem(
    codable_concept,
    system):
  """Extract code in codable_concept."""
  for coding in codable_concept.coding:
    if (coding.HasField('system') and coding.HasField('code') and
        coding.system.value == system):
      return coding.code.value
  return None


def ToMicroSeconds(dt):
  delta = dt - datetime.datetime(1970, 1, 1)
  return int(delta.total_seconds()) * 1000000


# Note: this API only compose encounter level API.
def ComposeLabel(
    patient, enc,
    label_name, label_val,
    label_time):
  """Compose an event_label proto given inputs.

  Args:
    patient: patient proto
    enc: encounter proto
    label_name: name of label
    label_val: value of label.
    label_time: datetime of label.

  Returns:
    event_label proto.
  """
  event_label = google_extensions_pb2.EventLabel()
  # set patient_id
  event_label.patient.patient_id.value = patient.id.value
  event_label.type.system.value = GOOGLE_FHIR_LABEL_SYTEM
  event_label.type.code.value = label_name
  event_label.event_time.value_us = ToMicroSeconds(label_time)
  event_label.source.encounter_id.value = enc.id.value

  label = google_extensions_pb2.EventLabel.Label()
  label.class_name.system.value = GOOGLE_FHIR_LABEL_SYTEM
  label.class_name.code.value = label_val
  event_label.label.add().CopyFrom(label)

  return event_label


def LengthOfStayRangeAt24Hours(patient, enc):
  """Generate length of stay range labels at 24 hours after admission.

  Args:
    patient: patient proto, needed for label proto.
    enc: encounter, caller needs to do the proper cohort filterings.

  Yields:
    (label_name, value, label_time) tuple.
  """
  label_time = encounter.AtDuration(enc, 24)
  ecounter_length_days = encounter.EncounterLengthDays(enc)
  label_val = None
  for idx in range(len(LOS_BOUNDARIES)):
    if ecounter_length_days <= LOS_BOUNDARIES[idx]:
      if idx == 0:
        label_val = 'less_or_equal_%d' % LOS_BOUNDARIES[idx]
      else:
        label_val = '%d_%d' % (LOS_BOUNDARIES[idx - 1], LOS_BOUNDARIES[idx])
      break
  if label_val is None:
    label_val = 'above_%d' % LOS_BOUNDARIES[-1]

  yield ComposeLabel(patient, enc,
                     LOS_RANGE_LABEL, label_val, label_time)
