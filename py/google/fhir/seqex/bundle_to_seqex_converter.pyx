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

from libcpp.memory cimport unique_ptr

from tensorflow.core.example import example_pb2


cdef class PyBundleToSeqexConverter:

  cdef unique_ptr[UnprofiledBundleToSeqexConverter] _thisptr

  def __init__(self, version_config,
               enable_attribution, generate_sequence_label):
    """A python wrapper for the c++ BundleToSeqexConverter class. The python
    wrapper accesses the c++ class via pointer.

    Args:
      version_config: VersionConfig in python proto format.
      enable_attribution: bool indicating if we enable attribution.
      generate_sequence_label: bool indicating whether to generate labels as
          sequence feature.

    Raises:
      AssertError: If the input proto parameters cannot be parsed.
    """

    cdef VersionConfig c_version_config
    assert c_version_config.ParseFromString(
      version_config.SerializeToString())
    self._thisptr.reset(new UnprofiledBundleToSeqexConverter(
      c_version_config, enable_attribution,
      generate_sequence_label))

  def begin(self, patient_id, bundle, trigger_labels_pair_list):
    """ This should be called once per bundle. When it returns, callers can
    keep on calling get_next_example_and_key() until the function returns None.

    Args:
      patient_id: bytes containing the patient id in the form of "Patient/x"
      bundle: Bundle in python proto format.
      trigger_labels_pair_list: A list of tuple(trigger, labels). Trigger is in
        the form of EventTrigger python proto format, labels is a list of
        label in the form of EventLabel python proto format.

    Returns:
      A tuple of (boolean, dict[string, int]). The boolean indicates if the
      converter has been initialized successfully, the dict containing the
      counters for various stats.
    """
    # Implementation wise, this function converts the python parameters into
    # the c++ containers, and then invokes the underlying c++ function.
    cdef Bundle c_bundle
    assert c_bundle.ParseFromString(bundle.SerializeToString())
    cdef vector[TriggerLabelsPair] c_trigger_labels_pair_vector
    cdef cpp_map[string, int] c_stats
    cdef EventTrigger c_trigger
    cdef EventLabel c_label
    cdef vector[EventLabel] c_labels
    for (trigger, labels) in trigger_labels_pair_list:
      c_trigger.ParseFromString(trigger.SerializeToString())
      for label in labels:
        c_label.ParseFromString(label.SerializeToString())
        c_labels.push_back(c_label)
      c_trigger_labels_pair_vector.push_back(
        TriggerLabelsPair(c_trigger, c_labels))
      c_labels.clear()

    bool_result = self._thisptr.get().Begin(
      patient_id, c_bundle, c_trigger_labels_pair_vector, &c_stats)
    counter_stats = {}
    for c_stat in c_stats:
      counter_stats[c_stat.first] = c_stat.second
    return bool_result, counter_stats


  def get_next_example_and_key(self, append_prefix_to_key=False):
    """ Return the next available tuple of (key, SequenceExample, sequence
    length of the example); return
    None when we are done.

    Args:
      append_prefix_to_key: Boolean indicates if the example key is going to be
        appended with a sixteen digit hex prefix based on a hash, which would
        make it easier to shuffle the outputs.
    """
    converter = self._thisptr.get()
    if converter.Done():
      return None
    c_example = converter.GetExample()
    example_key = (converter.ExampleKeyWithPrefix()
                   if append_prefix_to_key else converter.ExampleKey())
    seq_len = converter.ExampleSeqLen()
    converter.Next()
    return (example_key, example_pb2.SequenceExample().FromString(
      c_example.SerializeAsString()), seq_len)
