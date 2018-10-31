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

"""Python beam functions for bundle-to-seqex."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import apache_beam as beam

from proto.stu3 import google_extensions_pb2
from proto.stu3 import resources_pb2
from py.google.fhir.seqex import bundle_to_seqex_converter
from py.google.fhir.seqex import bundle_to_seqex_util
from py.google.fhir.stu3 import util
from tensorflow.core.example import example_pb2


_BUNDLE_TAG = "bundle"
_TRIGGERS_TAG = "trigger_labels_pair_lists"

typehints_trigger_labels_pair_list = beam.typehints.List[
    beam.typehints.Tuple[google_extensions_pb2.EventTrigger, beam.typehints
                         .List[beam.typehints.Any]]]


@beam.typehints.with_input_types(resources_pb2.Bundle)
@beam.typehints.with_output_types(
    beam.typehints.Tuple[bytes, resources_pb2.Bundle])
class KeyBundleByPatientIdFn(beam.DoFn):
  """ Key bundle by patient id."""

  def process(self, bundle):
    patient = util.GetPatient(bundle)
    if not patient:
      beam.metrics.Metrics.counter("medical_records",
                                   "num-bundle-without-patient").inc()
      return
    yield (bytes("Patient/" + patient.id.value), bundle)


@beam.typehints.with_input_types(google_extensions_pb2.EventLabel)
@beam.typehints.with_output_types(
    beam.typehints.Tuple[bytes, google_extensions_pb2.EventLabel])
class KeyEventLabelByPatientIdFn(beam.DoFn):
  """ Key EventLabel by patient id."""

  def process(self, event_label):
    if not event_label.HasField("patient"):
      beam.metrics.Metrics.counter("medical_records",
                                   "num-eventlabel-without-patient").inc()
      return
    yield (bytes("Patient/" + event_label.patient.patient_id.value),
           event_label)


@beam.typehints.with_input_types(
    beam.typehints.Tuple[bytes, beam.typehints.Dict[bytes, beam.typehints.Any]])
@beam.typehints.with_output_types(
    beam.typehints.Tuple[bytes, beam.typehints.Tuple[
        resources_pb2.Bundle, typehints_trigger_labels_pair_list]])
class _JoinBundleAndTriggersDoFn(beam.DoFn):
  """Join bundle and trigger labels from the join result."""

  def process(self, key_join_values):
    (key, join_values) = key_join_values
    yield_result = True
    if len(join_values[_BUNDLE_TAG]) != 1:
      beam.metrics.Metrics.counter("medical_records", "num-no-bundle").inc()
      yield_result = False
    else:
      beam.metrics.Metrics.counter("medical_records", "num-bundles-in").inc()
    if len(join_values[_TRIGGERS_TAG]) != 1:
      beam.metrics.Metrics.counter("medical_records",
                                   "num-bundle-without-trigger-event").inc()
      yield_result = False
    else:
      beam.metrics.Metrics.counter("medical_records", "num-triggers-in").inc()
    if not yield_result:
      return
    beam.metrics.Metrics.counter("medical_records",
                                 "num-trigger-bundles-out").inc()
    yield (key, (join_values[_BUNDLE_TAG][0], join_values[_TRIGGERS_TAG][0]))


def _CreateTriggerLabelsPairList(elem):
  """Map a tuple (key, event_label_list) to (key, trigger_labels_pair_list)."""
  (key, event_label_list) = elem
  return (key,
          bundle_to_seqex_util.get_trigger_labels_from_input_labels(
              event_label_list))


def CreateTriggerLabelsPairLists(event_labels):
  """Returns a PCollection of tuples (patient_id, trigger_labels_pair_list).

  Args:
    event_labels: a PCollection of EventLabels.
  """
  return (
      event_labels
      | "KeyEventLabelsByPatientId" >> beam.ParDo(KeyEventLabelByPatientIdFn())
      | "GroupEventLabelsByPatientId" >> beam.GroupByKey()
      |
      "CreateTriggerLabelsPairLists" >> beam.Map(_CreateTriggerLabelsPairList))


def CreateBundleAndLabels(bundles, trigger_labels_pair_lists):
  """Returns the joined result of bundles and trigger_labels_pair_lists.

  Args:
   bundles: a PCollection of tuples (patient_id, bundle).
   trigger_labels_pair_lists: a PCollection of tuples (patient_id,
     trigger_labels_pair_list).

  Returns:
   PCollection of tuples (patient_id, tuple(bundle, trigger_labels_pair_list)).
   Patient_id without bundle or trigger_labels_pair_list are dropped from
   the output.
  """
  tags = {
      _BUNDLE_TAG: bundles,
      _TRIGGERS_TAG: trigger_labels_pair_lists
  }
  join_results = tags | "GroupBundleAndTriggers" >> beam.CoGroupByKey()
  return (join_results
          | "JoinBundleAndTriggers" >> beam.ParDo(_JoinBundleAndTriggersDoFn()))


@beam.typehints.with_input_types(
    beam.typehints.Tuple[bytes, beam.typehints.Tuple[
        resources_pb2.Bundle, typehints_trigger_labels_pair_list]])
@beam.typehints.with_output_types(example_pb2.SequenceExample)
class BundleAndLabelsToSeqexDoFn(beam.DoFn):
  """ A DoFn converting bundle and labels into sequence examples.
  """

  def __init__(self, version_config, enable_attribution,
               generate_sequence_label):
    self._version_config = version_config
    self._enable_attribution = enable_attribution
    self._generate_sequence_label = generate_sequence_label

  def start_bundle(self):
    self._converter = bundle_to_seqex_converter.PyBundleToSeqexConverter(
        version_config=self._version_config,
        enable_attribution=self._enable_attribution,
        generate_sequence_label=self._generate_sequence_label)

  def process(self, input_pair):
    (patient_key, bundle_labels) = input_pair
    (bundle, trigger_labels_pair_list) = bundle_labels
    if not trigger_labels_pair_list:
      beam.metrics.Metrics.counter("medical_records",
                                   "bundle-without-trigger-event").inc()
      return
    (begin_result, stats) = self._converter.begin(patient_key, bundle,
                                                  trigger_labels_pair_list)
    if not begin_result:
      beam.metrics.Metrics.counter("medical_records",
                                   "converter-failed-to-initialize").inc()
      return
    for stat_key, stat_value in stats.items():
      beam.metrics.Metrics.counter("medical_records", stat_key).inc(stat_value)
    next_example_and_key = self._converter.get_next_example_and_key(
        append_prefix_to_key=True)
    while next_example_and_key:
      (_, seqex, seqex_len) = next_example_and_key
      if not seqex_len:
        # There's no data before the label, but it would be cheating to not
        # emit this sample anyways.
        beam.metrics.Metrics.counter("medical_records",
                                     "label-before-examples").inc()
      else:
        beam.metrics.Metrics.distribution("medical_records",
                                          "seqex-len").update(seqex_len)
      next_example_and_key = self._converter.get_next_example_and_key(
          append_prefix_to_key=True)
      yield seqex
