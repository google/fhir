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

r"""Run the model locally.
"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import glob
import os
from absl import flags

import tensorflow as tf

from py.google.fhir.models import model

FLAGS = flags.FLAGS

flags.DEFINE_string(
    'input_dir', '',
    'Input data dir in TFRecord format containing tf.SequenceExamples.')
flags.DEFINE_string(
    'output_dir', '',
    'Output dir to store the model checkpoints and evaluations.')
flags.DEFINE_string(
    'label_name', 'label.length_of_stay_range.class',
    'Name of the label in the context of the tf.SequenceExamples.')
flags.DEFINE_list(
    'label_values', ['less_or_equal_3', '3_7', '7_14', 'above_14'],
    'List of values that the label can be.')
flags.DEFINE_string(
    'hparams_override', '',
    """A comma-separated list of `name=value` hyperparameter values. This flag
    is used to override hyperparameter settings either when manually selecting
    hyperparameters. If a hyperparameter is specified by this flag then it must
    be a valid hyperparameter name for the model, see create_hparams.""")
flags.DEFINE_integer('num_train_steps', 100, 'Total training steps.')
flags.DEFINE_integer('num_eval_steps', 100, 'Total evaluation steps.')


def main(unused_argv):
  hparams = model.create_hparams(FLAGS.hparams_override)
  tf.logging.info('Using hyperparameters %s', hparams)
  time_crossed_features = [
      cross.split(':')
      for cross in hparams.time_crossed_features
      if cross and cross != 'n/a'
  ]
  train_input_fn = model.get_input_fn(
      mode=tf.estimator.ModeKeys.TRAIN,
      input_files=glob.glob(os.path.join(FLAGS.input_dir, 'train*')),
      label_name=FLAGS.label_name,
      dedup=hparams.dedup,
      time_windows=hparams.time_windows,
      include_age=hparams.include_age,
      categorical_context_features=hparams.categorical_context_features,
      sequence_features=hparams.sequence_features,
      time_crossed_features=time_crossed_features,
      batch_size=hparams.batch_size)
  eval_input_fn = model.get_input_fn(
      mode=tf.estimator.ModeKeys.EVAL,
      input_files=glob.glob(os.path.join(FLAGS.input_dir, 'validation*')),
      label_name=FLAGS.label_name,
      dedup=hparams.dedup,
      time_windows=hparams.time_windows,
      include_age=hparams.include_age,
      categorical_context_features=hparams.categorical_context_features,
      sequence_features=hparams.sequence_features,
      time_crossed_features=time_crossed_features,
      # Fixing the batch size to get comparable evaluations.
      batch_size=32)
  train_spec = tf.estimator.TrainSpec(input_fn=train_input_fn,
                                      max_steps=FLAGS.num_train_steps)
  eval_spec = tf.estimator.EvalSpec(
      input_fn=eval_input_fn, steps=FLAGS.num_eval_steps, throttle_secs=60)

  estimator = model.make_estimator(
      hparams, FLAGS.label_values, FLAGS.output_dir)
  tf.estimator.train_and_evaluate(estimator, train_spec, eval_spec)


if __name__ == '__main__':
  tf.app.run()
