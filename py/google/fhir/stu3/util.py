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
"""Util function for stu3 protos."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from proto.stu3 import resources_pb2


def GetPatient(
    bundle):
  """Returns the patient resource from the bundle, if it exists."""
  for entry in bundle.entry:
    if entry.resource.HasField('patient'):
      return entry.resource.patient
  return None
