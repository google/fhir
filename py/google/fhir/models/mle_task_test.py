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

"""Tests for model run and export."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import os
import shutil
import tempfile

from absl import flags
from absl.testing import absltest
from absl.testing import parameterized
import tensorflow as tf
from py.google.fhir.models import mle_task
from py.google.fhir.models import test_utils


FLAGS = flags.FLAGS
TESTDATA_DIR = ('com_google_fhir/testdata/stu3/tensorflow/')


LABEL_VALUES = ['less_or_equal_3', '3_7', '7_14', 'above_14']


class ExperimentTest(tf.test.TestCase, parameterized.TestCase):

  def setUp(self):
    super(ExperimentTest, self).setUp()
    self.seqex_list = [test_utils.read_seqex_ascii(
        filename,
        os.path.join(absltest.get_default_test_srcdir(), TESTDATA_DIR))
                       for filename in ['example1.ascii', 'example2.ascii']]
    self.input_data_dir = tempfile.mkdtemp()
    test_utils.create_input_tfrecord(
        self.seqex_list, self.input_data_dir, 'train')
    test_utils.create_input_tfrecord(
        self.seqex_list, self.input_data_dir, 'validation')
    self.log_dir = tempfile.mkdtemp()

  def tearDown(self):
    super(ExperimentTest, self).tearDown()
    if self.log_dir:
      shutil.rmtree(self.log_dir)
    if self.input_data_dir:
      shutil.rmtree(self.input_data_dir)

  def testExperiment(self):
    experiment_fn = mle_task.make_experiment_fn(
        self.input_data_dir, num_train_steps=100, num_eval_steps=5,
        label_name='label.length_of_stay_range.class',
        label_values=','.join(LABEL_VALUES), hparams_overrides='')
    experiment = experiment_fn(self.log_dir)
    experiment.train_and_evaluate()
    export_dir = os.path.join(self.log_dir, 'export/Servo')
    self.assertTrue(os.path.exists(export_dir))
    export_dir = os.path.join(export_dir, os.listdir(export_dir)[0])

    # Restore the model.
    with tf.Graph().as_default() as graph:
      with tf.Session(graph=graph) as sess:
        meta_graph_def = tf.saved_model.loader.load(
            sess, [tf.saved_model.tag_constants.SERVING], export_dir)
        k = tf.saved_model.signature_constants.DEFAULT_SERVING_SIGNATURE_DEF_KEY
        signature_def = meta_graph_def.signature_def[k]
        # Predict a class for one of the training examples.
        example = self.seqex_list[0].SerializeToString()
        if tf.__version__ >= '1.12.0':
          example = [example]
        predictions = sess.run(
            {
                'scores': signature_def.outputs['scores'].name,
                'classes': signature_def.outputs['classes'].name
            },
            feed_dict={
                signature_def.inputs['inputs'].name: example
            })
        # Assert that the prediction is correct.
        max_class = max(enumerate(predictions['scores'][0]),
                        key=lambda x: x[1])[0]
        print('max_class')
        print(max_class)
        print(predictions)
        self.assertAllEqual('above_14', predictions['classes'][0][max_class])


if __name__ == '__main__':
  absltest.main()
