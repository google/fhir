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
from libcpp.map cimport map as cpp_map
from libcpp.vector cimport vector
from libcpp.utility cimport pair

from libcpp.string cimport string

cdef extern from "tensorflow/core/example/example.pb.h" namespace "tensorflow" nogil:
  cdef cppclass SequenceExample:
    SequenceExample()
    bool ParseFromString(const string& input)
    string SerializeAsString() const

cdef extern from "proto/stu3/google_extensions.pb.h" namespace "google::fhir::stu3::google" nogil:
  cdef cppclass EventTrigger:
    bool ParseFromString(const string& input)
    pass

cdef extern from "proto/stu3/google_extensions.pb.h" namespace "google::fhir::stu3::google" nogil:
  cdef cppclass EventLabel:
    bool ParseFromString(const string& input)
    pass

cdef extern from "proto/stu3/resources.pb.h" namespace "google::fhir::stu3::proto" nogil:
  cdef cppclass Bundle:
    bool ParseFromString(const string& input)
    pass

cdef extern from "proto/version_config.pb.h" namespace "google::fhir::proto" nogil:
  cdef cppclass VersionConfig:
    bool ParseFromString(const string& input)

ctypedef pair[EventTrigger, vector[EventLabel]] TriggerLabelsPair

cdef extern from "google/fhir/seqex/bundle_to_seqex_converter.h" namespace "google::fhir::seqex" nogil:
  cdef cppclass UnprofiledBundleToSeqexConverter:

    UnprofiledBundleToSeqexConverter(const VersionConfig&, const bool, const bool),

    bool Begin(const string& patient_id, const Bundle& bundle,
               const vector[TriggerLabelsPair]& labels,
               cpp_map[string, int]* counter_stats)

    bool Next()

    string ExampleKey()

    string ExampleKeyWithPrefix()

    SequenceExample GetExample()

    int ExampleSeqLen()

    bool Done()
