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

"""The main python function for bundle_to_seqex.

"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from absl import app
from absl import flags
import apache_beam as beam
from apache_beam.options.pipeline_options import PipelineOptions
from apache_beam.options.pipeline_options import StandardOptions
from google.protobuf import text_format
from proto.stu3 import google_extensions_pb2
from proto.stu3 import resources_pb2
from proto.stu3 import version_config_pb2
from py.google.fhir.seqex import bundle_to_seqex
from tensorflow.core.example import example_pb2

FLAGS = flags.FLAGS

flags.DEFINE_string('bundle_path', None, 'Input FHIR bundle filepattern.')
flags.DEFINE_string('output_path', None, 'Output SequenceExample filepattern.')
flags.DEFINE_integer('num_output_shards', 10, 'Number of output shards')
flags.DEFINE_string(
    'label_path', None, 'Pattern of input files with EventLabel protos to be '
    'used in place of the Bundle to mark both the end of the '
    'sequence the training label.')
flags.DEFINE_string('fhir_version_config', None,
'Location of the fhir version config ')


def _get_version_config(version_config_path):
  with open(version_config_path) as f:
    return text_format.Parse(f.read(), version_config_pb2.VersionConfig())


def main(argv):
  del argv  # Unused.

  # Always use DirectRunner.
  options = PipelineOptions()
  options.view_as(StandardOptions).runner = 'DirectRunner'
  p = beam.Pipeline(options=options)

  version_config = _get_version_config(FLAGS.fhir_version_config)

  keyed_bundles = (
      p
      | 'readBundles' >> beam.io.ReadFromTFRecord(
          FLAGS.bundle_path, coder=beam.coders.ProtoCoder(resources_pb2.Bundle))
      | 'KeyBundlesByPatientId' >> beam.ParDo(
          bundle_to_seqex.KeyBundleByPatientIdFn()))
  event_labels = (
      p | 'readEventLabels' >> beam.io.ReadFromTFRecord(
          FLAGS.label_path,
          coder=beam.coders.ProtoCoder(google_extensions_pb2.EventLabel)))
  keyed_event_labels = bundle_to_seqex.CreateTriggerLabelsPairLists(
      event_labels)
  bundles_and_labels = bundle_to_seqex.CreateBundleAndLabels(
      keyed_bundles, keyed_event_labels)
  _ = (
      bundles_and_labels
      | 'Reshuffle1' >> beam.Reshuffle()
      | 'GenerateSeqex' >> beam.ParDo(
          bundle_to_seqex.BundleAndLabelsToSeqexDoFn(
              version_config=version_config,
              enable_attribution=False,
              generate_sequence_label=False))
      | 'Reshuffle2' >> beam.Reshuffle()
      | 'WriteSeqex' >> beam.io.WriteToTFRecord(
          FLAGS.output_path,
          coder=beam.coders.ProtoCoder(example_pb2.SequenceExample),
          file_name_suffix='.tfrecords',
          num_shards=FLAGS.num_output_shards))

  p.run()


if __name__ == '__main__':
  app.run(main)
