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

"""Utilities for reading and wrting testdata."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import os

import tensorflow as tf
from google.protobuf import text_format


def read_seqex_ascii(filename, testdata_dir):
  """Read a tf.SequenceExample in ascii format from disk."""
  seqex_pb = tf.train.SequenceExample()
  example_file = os.path.join(testdata_dir, filename)
  with open(example_file, 'r') as f:
    text_format.Parse(f.read(), seqex_pb)
    return seqex_pb


def create_input_tfrecord(seqex_list, tmp_dir, filename):
  """Create a temporary TFRecord file on disk with the tf.SequenceExamples.

  Args:
    seqex_list: A list of tf.SequenceExamples.
    tmp_dir: Path to the test tmp directory.
    filename: Temporary filename for TFRecord output.
  Returns:
    The path to an TFRecord table containing the provided examples.
  """
  path = os.path.join(tmp_dir, filename)
  with tf.python_io.TFRecordWriter(path) as writer:
    for seqex in seqex_list:
      writer.write(seqex.SerializeToString())
  return path
