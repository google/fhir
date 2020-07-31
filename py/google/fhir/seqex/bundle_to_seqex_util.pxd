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

from libcpp cimport bool
from libcpp.set cimport set as cpp_set
from libcpp.vector cimport vector
from libcpp.utility cimport pair

from libcpp.string cimport string

cdef extern from "proto/r4/ml_extensions.pb.h" namespace "::google::fhir::r4::ml" nogil:
  cdef cppclass EventLabel:
    bool ParseFromString(const string& input)
    string SerializeAsString() const
    pass

cdef extern from "proto/r4/ml_extensions.pb.h" namespace "::google::fhir::r4::ml" nogil:
  cdef cppclass EventTrigger:
    bool ParseFromString(const string& input)
    string SerializeAsString() const
    pass

cdef extern from "proto/r4/core/resources/bundle_and_contained_resource.pb.h" namespace "google::fhir::r4::core" nogil:
  cdef cppclass Bundle:
    bool ParseFromString(const string& input)
    pass

ctypedef pair[EventTrigger, vector[EventLabel]] TriggerLabelsPair

cdef extern from "google/fhir/seqex/r4.h" namespace "::google::fhir::seqex_r4" nogil:

  void GetTriggerLabelsPairFromInputLabels(
    const vector[EventLabel]&, vector[TriggerLabelsPair]*)

  void GetTriggerLabelsPair(const Bundle&, const cpp_set[string]&,
                            const string&, vector[TriggerLabelsPair]*,
                            int*)
