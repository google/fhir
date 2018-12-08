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

"""Generate synthetic train and validation TFRecords of tf.SequenceExamples.

Usage from a Google Cloud shell:

python make_sequence_example.py

Results then can be copied to a GCS Bucket

PROJECT_ID=$(gcloud config list project --format "value(core.project)")
BUCKET="${PROJECT_ID}-ml"

First, create the bucket using:

gsutil mb -c regional -l us-central1 gs://${BUCKET}

Second, copy the data into the bucket using:

gsutil cp /tmp/data/train gs://${BUCKET}/data/
gsutil cp /tmp/data/validation gs://${BUCKET}/data/
"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import argparse
import os
import sys

import tensorflow as tf
from google.protobuf import text_format

FLAGS = None


def make_records(filename):
  """Create TFRecords stored at filename with 100 tf.SequenceExamples.

  Args:
    filename: The name of the file to store the TFRecords.
  """

  sequence_examples_pbtxt = """
  context {
    feature: {
      key  : "Patient.gender"
      value: {
        bytes_list: {
          value: [ "male" ]
        }
      }
    }
    feature: {
      key  : "label.length_of_stay_range.class"
      value: {
        bytes_list: {
          value: [ "above_14" ]
        }
      }
    }
    feature: {
      key  : "Patient.birthDate"
      value: {
        int64_list: {
          value: [ -1167580800 ]
        }
      }
    }
    feature: {
      key  : "timestamp"
      value: {
        int64_list: {
          value: [ 1528917657 ]
        }
      }
    }
    feature: {
      key  : "sequenceLength"
      value: {
        int64_list: {
          value: [ 4 ]
        }
      }
    }
  }
  feature_lists {
    feature_list {
      key: "Observation.code"
      value {
        feature {
          bytes_list {
            value: "loinc:2"
          }
        }
        feature {
          bytes_list {
            value: "loinc:4"
          }
        }
        feature {
          bytes_list {
            value: "loinc:6"
          }
        }
        feature {
          bytes_list {
            value: "loinc:6"
          }
        }
      }
    }
    feature_list {
      key: "Observation.value.quantity.value"
      value {
        feature {
          float_list {
            value: 1.0
          }
        }
        feature {
          float_list {
            value: 2.0
          }
        }
        feature {
          float_list {
          }
        }
        feature {
          float_list {
          }
        }
      }
    }
    feature_list {
      key: "Observation.value.quantity.unit"
      value {
        feature {
          bytes_list {
            value: "mg/L"
          }
        }
        feature {
          bytes_list {
          }
        }
        feature {
          bytes_list {
          }
        }
        feature {
          bytes_list {
          }
        }
      }
    }
    feature_list {
      key: "eventId"
      value {
        feature {
          int64_list {
            value: 1528917644
          }
        }
        feature {
          int64_list {
            value: 1528917645
          }
        }
        feature {
          int64_list {
            value: 1528917646
          }
        }
        feature {
          int64_list {
            value: 1528917647
          }
        }
      }
    }
  }
  """

  seqex = tf.train.SequenceExample()
  text_format.Parse(sequence_examples_pbtxt, seqex)

  with tf.python_io.TFRecordWriter(filename) as writer:
    for _ in range(100):
      writer.write(seqex.SerializeToString())


def main(unused_argv):
  # Write the tf.SequenceExample into training and validation files.
  make_records(os.path.join(FLAGS.directory, 'train'))
  make_records(os.path.join(FLAGS.directory, 'validation'))


if __name__ == '__main__':
  parser = argparse.ArgumentParser()
  parser.add_argument(
      '--directory',
      type=str,
      default='/tmp/data',
      help='Directory to download data files and write the converted result'
  )
  FLAGS, unparsed = parser.parse_known_args()
  tf.app.run(main=main, argv=[sys.argv[0]] + unparsed)
