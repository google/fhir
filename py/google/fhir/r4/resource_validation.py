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
"""Performs resource validation on R4 FHIR resource protos."""

from google.protobuf import message
from google.fhir import resource_validation
from google.fhir.r4 import primitive_handler

_PRIMITIVE_HANDLER = primitive_handler.PrimitiveHandler()


def validate_resource(resource: message.Message):
  """Performs basic FHIR constraint validation on the provided resource."""
  resource_validation.validate_resource(resource, _PRIMITIVE_HANDLER)
