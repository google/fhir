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

"""Tests for model."""

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
from py.google.fhir.models import model
from py.google.fhir.models import test_utils


FLAGS = flags.FLAGS
TESTDATA_DIR = ('com_google_fhir/testdata/stu3/tensorflow/')


LABEL_VALUES = ['less_or_equal_3', '3_7', '7_14', 'above_14']


class ModelTest(tf.test.TestCase, parameterized.TestCase):

  def setUp(self):
    super(ModelTest, self).setUp()
    seqex_list = [test_utils.read_seqex_ascii(
        filename,
        os.path.join(absltest.get_default_test_srcdir(), TESTDATA_DIR))
                  for filename in ['example1.ascii', 'example2.ascii']]
    self.input_data_dir = test_utils.create_input_tfrecord(
        seqex_list, tempfile.mkdtemp(), 'input')
    self.log_dir = tempfile.mkdtemp()

  def tearDown(self):
    super(ModelTest, self).tearDown()
    if self.log_dir:
      shutil.rmtree(self.log_dir)
    if self.input_data_dir:
      os.remove(self.input_data_dir)

  @parameterized.parameters(0, 1, 2)
  def testInputFn(self, num_too_old_events):
    event_ids = [1528917644, 1528917645, 1528917646, 1528917647]
    timestamp = 1528917657
    oldest_event_cutoff = event_ids[num_too_old_events]
    feature_map, label = model.get_input_fn(
        tf.estimator.ModeKeys.TRAIN,
        [self.input_data_dir],
        'label.length_of_stay_range.class',
        dedup=True,
        time_windows=[timestamp - oldest_event_cutoff, 0],
        include_age=True,
        categorical_context_features=[],
        sequence_features=['Observation.code'],
        time_crossed_features=[[
            'Observation.code', 'Observation.value.quantity.value',
            'Observation.value.quantity.unit'
        ]],
        batch_size=1,
        shuffle=False)()
    with self.test_session() as sess:
      sess.run(tf.tables_initializer())
      coord = tf.train.Coordinator()
      tf.train.start_queue_runners(sess=sess, coord=coord)
      feature_map['label'] = label
      results = sess.run(feature_map)
      self.assertAllEqual(['above_14'], results['label'])
      key = (model.SEQUENCE_KEY_PREFIX +
             'Observation.code-til-%d' % 0)
      self.assertAllEqual(
          #  Second "loinc:6" will be de-duped.
          ['loinc:2', 'loinc:4', 'loinc:6'][num_too_old_events:],
          results[key].values)
      self.assertAllEqual(
          [[0, 0], [0, 1], [0, 2]][:3 - num_too_old_events],
          results[key].indices)
      self.assertAllEqual(
          [1, 3 - num_too_old_events],
          results[key].dense_shape)
      cross_key = (
          model.SEQUENCE_KEY_PREFIX +
          'Observation.code_Observation.value.quantity.value_'
          'Observation.value.quantity.unit-til-0')
      all_loincs = ['loinc:2-1.000000-mg/L', 'loinc:4-2.000000-n/a',
                    'loinc:6-n/a-n/a']
      self.assertAllEqual(
          all_loincs[num_too_old_events:],
          results[cross_key].values)
      self.assertAllEqual(
          [[0, 0], [0, 1], [0, 2]][:3 - num_too_old_events],
          results[cross_key].indices)
      self.assertAllEqual(
          [1, 3 - num_too_old_events],
          results[cross_key].dense_shape)
      self.assertAllClose(
          [85.505402],
          results[model.CONTEXT_KEY_PREFIX + model.AGE_KEY])

  def testInputFnBatch(self):
    feature_map, label = model.get_input_fn(
        tf.estimator.ModeKeys.TRAIN,
        [self.input_data_dir],
        'label.length_of_stay_range.class',
        dedup=False,
        time_windows=[12, 0],
        include_age=True,
        categorical_context_features=[],
        sequence_features=[
            'Observation.code', 'Observation.value.quantity.value'
        ],
        time_crossed_features=[[
            'Observation.code', 'Observation.value.quantity.value',
            'Observation.value.quantity.unit'
        ]],
        batch_size=2,
        shuffle=False)()
    with self.test_session() as sess:
      sess.run(tf.tables_initializer())
      coord = tf.train.Coordinator()
      tf.train.start_queue_runners(sess=sess, coord=coord)
      feature_map['label'] = label
      results = sess.run(feature_map)
      self.assertAllEqual(['above_14', 'above_14'], results['label'])

      code_key = (
          model.SEQUENCE_KEY_PREFIX + 'Observation.code-til-%d' % 0)
      self.assertAllEqual(
          #  First loinc:2 from example1 is out of range.
          ['loinc:4', 'loinc:6', 'loinc:6', 'loinc:1', 'loinc:4'],
          results[code_key].values)
      # When not deduping, original indices are kept in the SparseTensor.
      self.assertAllEqual(
          [[0, 1], [0, 2], [0, 3], [1, 0], [1, 2]],
          results[code_key].indices)
      self.assertAllEqual(
          [2, 4],
          results[code_key].dense_shape)

      value_key = (
          model.SEQUENCE_KEY_PREFIX +
          'Observation.value.quantity.value-til-%d' % 0)
      self.assertAllEqual(
          [2.0, 1.0, 2.0],
          #  First value from example1 is out of range.
          results[value_key].values)
      # When not deduping, original indices are kept in the SparseTensor.
      self.assertAllEqual(
          [[0, 1], [1, 0], [1, 2]],
          results[value_key].indices)
      self.assertAllEqual(
          [2, 4],
          results[value_key].dense_shape)

      self.assertAllClose(
          [85.505402, 85.505402],
          results[model.CONTEXT_KEY_PREFIX + model.AGE_KEY])

  def testInputFnBatchDedup(self):
    feature_map, label = model.get_input_fn(
        tf.estimator.ModeKeys.TRAIN,
        [self.input_data_dir],
        'label.length_of_stay_range.class',
        dedup=True,
        time_windows=[12, 0],
        include_age=True,
        categorical_context_features=[],
        sequence_features=[
            'Observation.code', 'Observation.value.quantity.value'
        ],
        time_crossed_features=[[
            'Observation.code', 'Observation.value.quantity.value',
            'Observation.value.quantity.unit'
        ]],
        batch_size=2,
        shuffle=False)()
    with self.test_session() as sess:
      sess.run(tf.tables_initializer())
      coord = tf.train.Coordinator()
      tf.train.start_queue_runners(sess=sess, coord=coord)
      feature_map['label'] = label
      results = sess.run(feature_map)
      self.assertAllEqual(['above_14', 'above_14'], results['label'])

      code_key = (
          model.SEQUENCE_KEY_PREFIX + 'Observation.code-til-%d' % 0)
      self.assertAllEqual(
          #  First loinc:2 from example1 is out of range.
          #  Second "loinc:6" will be deduped.
          ['loinc:4', 'loinc:6', 'loinc:1', 'loinc:4'],
          results[code_key].values)
      # Indices are reordered on axis 1 due to deduping.
      self.assertAllEqual(
          [[0, 0], [0, 1], [1, 0], [1, 1]],
          results[code_key].indices)
      self.assertAllEqual(
          [2, 2],
          results[code_key].dense_shape)

      value_key = (
          model.SEQUENCE_KEY_PREFIX +
          'Observation.value.quantity.value-til-%d' % 0)
      self.assertAllEqual(
          #  First value from example1 is out of range.
          [2.0, 1.0, 2.0],
          results[value_key].values)
      # Indices are reordered on axis 1 due to deduping.
      self.assertAllEqual(
          [[0, 0], [1, 0], [1, 1]],
          results[value_key].indices)
      self.assertAllEqual(
          [2, 2],
          results[value_key].dense_shape)

      self.assertAllClose(
          [85.505402, 85.505402],
          results[model.CONTEXT_KEY_PREFIX + model.AGE_KEY])

  def testBasicTraining(self):
    """Test that we can learn a constant label of 0.0 for a fixed example."""
    hparams = model.create_hparams(
        'sequence_features=[Observation.code],'
        'time_crossed_features=[Observation.code:'
        'Observation.value.quantity.value:Observation.value.quantity.unit:'
        'Observation.value.string],'
        'time_concat_bucket_sizes=[12],'
        'learning_rate=0.5')
    time_crossed_features = [
        features.split(':') for features in hparams.time_crossed_features
    ]
    estimator = model.make_estimator(hparams, LABEL_VALUES, FLAGS.test_tmpdir)
    estimator.train(
        input_fn=model.get_input_fn(
            mode=tf.estimator.ModeKeys.TRAIN,
            input_files=[self.input_data_dir],
            label_name='label.length_of_stay_range.class',
            dedup=hparams.dedup,
            time_windows=hparams.time_windows,
            include_age=hparams.include_age,
            categorical_context_features=hparams.categorical_context_features,
            sequence_features=hparams.sequence_features,
            time_crossed_features=time_crossed_features,
            batch_size=10),
        steps=100)
    estimator.evaluate(
        input_fn=model.get_input_fn(
            mode=tf.estimator.ModeKeys.EVAL,
            input_files=[self.input_data_dir],
            label_name='label.length_of_stay_range.class',
            dedup=hparams.dedup,
            time_windows=hparams.time_windows,
            include_age=hparams.include_age,
            categorical_context_features=hparams.categorical_context_features,
            sequence_features=hparams.sequence_features,
            time_crossed_features=time_crossed_features,
            # Use a batch_size larger than the dataset to ensure we don't rely
            # on the static batch_size anywhere.
            batch_size=3),
        steps=1)
    results = list(estimator.predict(
        input_fn=model.get_input_fn(
            mode=tf.estimator.ModeKeys.EVAL,
            input_files=[self.input_data_dir],
            label_name='label.length_of_stay_range.class',
            dedup=hparams.dedup,
            time_windows=hparams.time_windows,
            include_age=hparams.include_age,
            categorical_context_features=hparams.categorical_context_features,
            sequence_features=hparams.sequence_features,
            time_crossed_features=time_crossed_features,
            batch_size=1,
            shuffle=False)))
    self.assertAllClose([0.0, 0.0, 0.0, 1.0], results[0]['probabilities'],
                        atol=0.1)


if __name__ == '__main__':
  absltest.main()
