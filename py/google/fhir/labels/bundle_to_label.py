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
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from absl import app
from absl import flags
import apache_beam as beam
from apache_beam.options.pipeline_options import GoogleCloudOptions
from apache_beam.options.pipeline_options import PipelineOptions
from apache_beam.options.pipeline_options import SetupOptions
from apache_beam.options.pipeline_options import StandardOptions
from proto.stu3 import google_extensions_pb2
from proto.stu3 import resources_pb2
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


@beam.typehints.with_input_types(resources_pb2.Bundle)
@beam.typehints.with_output_types(google_extensions_pb2.EventLabel)
class LengthOfStayRangeLabelAt24HoursFn(beam.DoFn):
  """Converts Bundle into length of stay range at 24 hours label.

    Cohort: inpatient encounter that is longer than 24 hours
    Trigger point: 24 hours after admission
    Label: multi-label for length of stay ranges, see label.py for detail
  """

  def __init__(self, for_synthea=False):
    self._for_synthea = for_synthea

  def process(self, bundle):
    """Iterate through bundle and yield label.

    Args:
      bundle: input stu3.Bundle proto

    Yields:
      stu3.EventLabel proto.
    """
    patient = encounter.GetPatient(bundle)
    if patient is not None:
      # Cohort: inpatient encounter > 24 hours.
      for enc in encounter.Inpatient24HrEncounters(bundle, self._for_synthea):
        for one_label in label.LengthOfStayRangeAt24Hours(patient, enc):
          yield one_label


def GetPipelineOptions():
  """Parse the command line flags and return proper pipeline options.

  Returns:
    PipelineOptions struct.
  """

  opts = PipelineOptions()
  if flags.FLAGS.runner == 'DataflowRunner':
    # Construct Dataflow runner options.
    opts.view_as(StandardOptions).runner = flags.FLAGS.runner
    # Make the main session available to Dataflow workers. We should try not to
    # add any more global variables which will complicate the session saving.
    opts.view_as(SetupOptions).save_main_session = True
    opts.view_as(SetupOptions).setup_file = flags.FLAGS.setup_file
    opts.view_as(GoogleCloudOptions).project = flags.FLAGS.project_id
    opts.view_as(
        GoogleCloudOptions).temp_location = flags.FLAGS.gcs_temp_location
  return opts


def main(argv):
  del argv
  assert flags.FLAGS.input_path
  assert flags.FLAGS.output_path
  p = beam.Pipeline(options=GetPipelineOptions())
  bundles = p | 'read' >> beam.io.ReadFromTFRecord(
      flags.FLAGS.input_path,
      coder=beam.coders.ProtoCoder(resources_pb2.Bundle))
  labels = bundles | 'BundleToLabel' >> beam.ParDo(
      LengthOfStayRangeLabelAt24HoursFn(for_synthea=flags.FLAGS.for_synthea))
  _ = labels | beam.io.WriteToTFRecord(
      flags.FLAGS.output_path,
      coder=beam.coders.ProtoCoder(google_extensions_pb2.EventLabel),
      file_name_suffix='.tfrecords')

  p.run()


if __name__ == '__main__':
  app.run(main)
