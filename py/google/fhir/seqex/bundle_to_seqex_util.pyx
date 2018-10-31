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

from proto.stu3 import google_extensions_pb2


def get_trigger_labels_from_input_labels(event_label_list):
  """ Returns list[trigger_labels_tuple] from a list of event_label.

  Args:
    event_label_list: a list of EventLabel in python proto format.

  Returns:
    A list of trigger_labels_tuple.
  """
  cdef vector[TriggerLabelsPair] c_trigger_labels_pair_vector
  cdef vector[EventLabel] c_event_label_vector
  cdef EventLabel c_event_label
  for event_label in event_label_list:
    assert c_event_label.ParseFromString(event_label.SerializeToString())
    c_event_label_vector.push_back(c_event_label)
  GetTriggerLabelsPairFromInputLabels(c_event_label_vector,
                                      &c_trigger_labels_pair_vector)
  trigger_labels_tuple_list = []
  for c_trigger_labels_pair in c_trigger_labels_pair_vector:
    c_trigger_event = c_trigger_labels_pair.first
    c_event_label_vector = c_trigger_labels_pair.second
    trigger_event = google_extensions_pb2.EventTrigger().FromString(
      c_trigger_event.SerializeAsString())
    event_label_list = [
      google_extensions_pb2.EventLabel().FromString(
        c_event_label.SerializeAsString()) for c_event_label in c_event_label_vector
    ]
    trigger_labels_tuple_list.append((trigger_event, event_label_list))
  return trigger_labels_tuple_list


def get_trigger_labels_pair(bundle, label_names, trigger_event_name):
  """ Returns a list of trigger_labels_tuple from Bundle.

  Args:
    bundle: Bundle in python proto format.
    label_names: A list of label name.
    trigger_event_name: The name of the trigger event.

  Returns:
    A tuple of (list[trigger_labels_tuple], num_triggers_filtered) where
    the second element indicates the number of triggers filtered due to
    the label time happens before the trigger time.
  """
  cdef Bundle c_bundle
  assert c_bundle.ParseFromString(bundle.SerializeToString())
  cdef cpp_set[string] c_label_names
  cdef vector[TriggerLabelsPair] c_trigger_labels_pair_vector
  cdef int num_triggers_filtered = 0
  GetTriggerLabelsPair(c_bundle, label_names, trigger_event_name,
                       &c_trigger_labels_pair_vector, &num_triggers_filtered)
  trigger_labels_tuple_list = []
  for c_trigger_labels_pair in c_trigger_labels_pair_vector:
    c_trigger_event = c_trigger_labels_pair.first
    c_event_label_vector = c_trigger_labels_pair.second
    trigger_event = google_extensions_pb2.EventTrigger().FromString(
      c_trigger_event.SerializeAsString())
    event_label_list = [
      google_extensions_pb2.EventLabel().FromString(
        c_event_label.SerializeAsString()) for c_event_label in c_event_label_vector
    ]
    trigger_labels_tuple_list.append((trigger_event, event_label_list))
  return (num_triggers_filtered, trigger_labels_tuple_list)
