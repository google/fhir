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
"""Generate label proto from FHIR Bundle."""

from typing import Iterator, List

from absl import app
from absl import flags
import apache_beam as beam
from apache_beam.options import pipeline_options

from proto.r4 import ml_extensions_pb2
from proto.r4.core.resources import bundle_and_contained_resource_pb2
from py.google.fhir.labels import encounter
from py.google.fhir.labels import label

# If set to false, encounter class codesystem is expected to be
# http://hl7.org/fhir/v3/ActCode .
# On the other hand, Synthea data is using the string 'inpatient' as the code
# value to represent inpatient encounter.
flags.DEFINE_bool('for_synthea', False,
                  'Set to true if the underlying data is from synthea.')
flags.DEFINE_string('output_path', '', 'The output file path')
flags.DEFINE_string('input_path', '', 'The input file path')

# Flags to configure Cloud Dataflow runner.
flags.DEFINE_string('runner', '', 'The beam runner; default means DirectRunner')
flags.DEFINE_string('project_id', '', 'The Google Cloud project ID')
flags.DEFINE_string('gcs_temp_location', '',
                    'The Google Cloud Storage temporary location')
flags.DEFINE_string('setup_file', '',
                    'The setup file for Dataflow dependencies')


@beam.typehints.with_input_types(bundle_and_contained_resource_pb2.Bundle)
@beam.typehints.with_output_types(ml_extensions_pb2.EventLabel)
class LengthOfStayRangeLabelAt24HoursFn(beam.DoFn):
  """Converts Bundle into length of stay range at 24 hours label.

    Cohort: inpatient encounter that is longer than 24 hours
    Trigger point: 24 hours after admission
    Label: multi-label for length of stay ranges, see label.py for detail
  """

  def __init__(self, for_synthea: bool = False):
    self._for_synthea = for_synthea

  def process(
      self, bundle: bundle_and_contained_resource_pb2.Bundle
  ) -> Iterator[ml_extensions_pb2.EventLabel]:
    """Iterate through bundle and yield label.

    Args:
      bundle: input R4 Bundle proto

    Yields:
      stu3.EventLabel proto.
    """
    patient = encounter.GetPatient(bundle)
    if patient is not None:
      # Cohort: inpatient encounter > 24 hours.
      for enc in encounter.Inpatient24HrEncounters(bundle, self._for_synthea):
        for one_label in label.LengthOfStayRangeAt24Hours(patient, enc):
          yield one_label


def GetPipelineOptions() -> pipeline_options.PipelineOptions:
  """Parse the command line flags and return proper pipeline options.

  Returns:
    pipeline_options.PipelineOptions struct.
  """

  opts = pipeline_options.PipelineOptions()
  if flags.FLAGS.runner == 'DataflowRunner':
    # Construct Dataflow runner options.
    opts.view_as(pipeline_options.StandardOptions).runner = flags.FLAGS.runner
    # Make the main session available to Dataflow workers. We should try not to
    # add any more global variables which will complicate the session saving.
    opts.view_as(pipeline_options.SetupOptions).save_main_session = True
    opts.view_as(
        pipeline_options.SetupOptions).setup_file = flags.FLAGS.setup_file
    opts.view_as(
        pipeline_options.GoogleCloudOptions).project = flags.FLAGS.project_id
    opts.view_as(pipeline_options.GoogleCloudOptions
                ).temp_location = flags.FLAGS.gcs_temp_location
  return opts


def main(argv: List[str]):
  del argv
  assert flags.FLAGS.input_path
  assert flags.FLAGS.output_path
  p = beam.Pipeline(options=GetPipelineOptions())
  bundles = p | 'read' >> beam.io.ReadFromTFRecord(
      flags.FLAGS.input_path,
      coder=beam.coders.ProtoCoder(bundle_and_contained_resource_pb2.Bundle))
  labels = bundles | 'BundleToLabel' >> beam.ParDo(
      LengthOfStayRangeLabelAt24HoursFn(for_synthea=flags.FLAGS.for_synthea))
  _ = labels | beam.io.WriteToTFRecord(
      flags.FLAGS.output_path,
      coder=beam.coders.ProtoCoder(ml_extensions_pb2.EventLabel),
      file_name_suffix='.tfrecords')

  p.run()


if __name__ == '__main__':
  app.run(main)
