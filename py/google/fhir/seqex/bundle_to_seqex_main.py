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
from absl import logging
import apache_beam as beam
from google.protobuf import text_format
from proto.stu3 import google_extensions_pb2
from proto.stu3 import resources_pb2
from proto.stu3 import version_config_pb2
from py.google.fhir.seqex import bundle_to_seqex
from tensorflow.core.example import example_pb2

FLAGS = flags.FLAGS

flags.DEFINE_string('input_filepattern', None, 'Input FHIR bundle filepattern.')
flags.DEFINE_string('output_filepattern', None,
                    'Output SequenceExample filepattern.')
flags.DEFINE_string(
    'labels_filepattern', None,
    'Pattern of input files with EventLabel protos to be '
    'used in place of the Bundle to mark both the end of the '
    'sequence the training label.')


def _get_version_config(version_config_path):
  with open(version_config_path) as f:
    return text_format.Parse(f.read(), version_config_pb2.VersionConfig())


def main(argv):
  del argv  # Unused.
  p = beam.Pipeline()

  version_config = _get_version_config(FLAGS.fhir_version_config)

  keyed_bundles = (
      p
      | 'readBundles' >> beam.io.ReadFromTFRecord(
          FLAGS.input_filepattern,
          coder=beam.coders.ProtoCoder(resources_pb2.Bundle))
      | 'KeyBundlesByPatientId' >> beam.ParDo(
          bundle_to_seqex.KeyBundleByPatientIdFn()))
  event_labels = (
      p | 'readEventLabels' >> beam.io.ReadFromTFRecord(
          FLAGS.labels_filepattern,
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
              version_config=version_config, enable_attribution=False))
      | 'Reshuffle2' >> beam.Reshuffle()
      | 'WriteSeqex' >> beam.io.WriteToTFRecord(
          FLAGS.output_filepattern,
          coder=beam.coders.ProtoCoder(example_pb2.SequenceExample)))

  result = p.run()
  logging.info('Job result: %s', result)


if __name__ == '__main__':
  app.run(main)
