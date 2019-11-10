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

r"""Train a model on the Google Cloud ML Engine.

PROJECT_ID=$(gcloud config list project --format "value(core.project)")
BUCKET="${PROJECT_ID}-ml"
JOB_NAME="job_$(date +%Y%m%d_%H%M%S)"

 gcloud ml-engine jobs submit training ${JOB_NAME} --package-path models \
 --module-name models.mle_task --staging-bucket gs://${BUCKET}   \
 --job-dir gs://${BUCKET}/${JOB_NAME}     --runtime-version 1.10 \
 --region us-central1  --scale-tier basic -- \
 --input_dir gs://${BUCKET}/data --output_dir gs://${BUCKET}/${JOB_NAME} \
 --num_train_steps 500

View tensorboard by running
tensorboard --logdir=gs://${BUCKET}/${JOB_NAME}
And preview on port 6006
"""

import argparse
import os
from . import model  # Using a relative import for ease of use with Cloud.
import tensorflow as tf
from tensorflow.contrib import learn as contrib_learn


def make_experiment_fn(input_dir, num_train_steps, num_eval_steps, label_name,
                       label_values, hparams_overrides, **experiment_args):
  """Creates the function used to create the Experiment object.

  This function creates a function that is used by the learn_runner
  module to create an Experiment. The Experiment encapsulates our
  Estimator; the Experiment object allows us to run training and
  evaluation locally or on the cloud.

  Args:
    input_dir: The directory the data can be found in.
    num_train_steps: The number of steps to run training for.
    num_eval_steps: The number of steps to run evaluation for.
    label_name: Name of the label present as context feature in the
      SequenceExamples.
    label_values: Comma-separated list of strings that specify the possible
      values of the label.
    hparams_overrides: String with possible overrides of HParams.
    **experiment_args: Arguments for the Experiment.

  Returns:
    A function that creates an Experiment object for the runner.
  """

  def experiment_fn(output_dir):
    """Function used in creating the Experiment object."""
    hparams = model.create_hparams(hparams_overrides)
    tf.logging.info('Using tf %s', str(tf.__version__))
    tf.logging.info('Using hyperparameters %s', hparams)

    time_crossed_features = [
        cross.split(':')
        for cross in hparams.time_crossed_features
        if cross and cross != 'n/a'
    ]
    train_input_fn = model.get_input_fn(
        mode=tf.estimator.ModeKeys.TRAIN,
        input_files=[os.path.join(input_dir, 'train')],
        label_name=label_name,
        dedup=hparams.dedup,
        time_windows=hparams.time_windows,
        include_age=hparams.include_age,
        categorical_context_features=hparams.categorical_context_features,
        sequence_features=hparams.sequence_features,
        time_crossed_features=time_crossed_features,
        batch_size=hparams.batch_size)
    eval_input_fn = model.get_input_fn(
        mode=tf.estimator.ModeKeys.EVAL,
        input_files=[os.path.join(input_dir, 'validation')],
        label_name=label_name,
        dedup=hparams.dedup,
        time_windows=hparams.time_windows,
        include_age=hparams.include_age,
        categorical_context_features=hparams.categorical_context_features,
        sequence_features=hparams.sequence_features,
        time_crossed_features=time_crossed_features,
        # Fixing the batch size to get comparable evaluations.
        batch_size=32)
    serving_input_fn = model.get_serving_input_fn(
        dedup=hparams.dedup,
        time_windows=hparams.time_windows,
        include_age=hparams.include_age,
        categorical_context_features=hparams.categorical_context_features,
        sequence_features=hparams.sequence_features,
        time_crossed_features=time_crossed_features,)

    estimator = model.make_estimator(hparams,
                                     label_values.split(','),
                                     output_dir)
    return contrib_learn.Experiment(
        estimator=estimator,
        train_input_fn=train_input_fn,
        eval_input_fn=eval_input_fn,
        export_strategies=[
            contrib_learn.utils.saved_model_export_utils.make_export_strategy(
                serving_input_fn,
                default_output_alternative_key=None,
                exports_to_keep=1)
        ],
        train_steps=num_train_steps,
        eval_steps=num_eval_steps,
        eval_delay_secs=0,
        continuous_eval_throttle_secs=60,
        **experiment_args)

  return experiment_fn


if __name__ == '__main__':
  parser = argparse.ArgumentParser()
  parser.add_argument(
      '--input_dir',
      help='GCS or local path to data in TFRecord format containing '
           'tf.SequenceExamples.',
      required=True
  )
  parser.add_argument(
      '--label_name',
      help='Name of the label in the context of the tf.SequenceExamples.',
      default='label.length_of_stay_range.class'
  )
  parser.add_argument(
      '--label_values',
      help='Comma-separated list of values that the label can be.',
      default='less_or_equal_3,3_7,7_14,above_14'
  )
  parser.add_argument(
      '--hparams_overrides',
      help='A comma-separated list of name=value hyperparameter values.',
      default=''
  )
  parser.add_argument(
      '--num_train_steps',
      help='Steps to run the training job for.',
      type=int,
      default=1000
  )
  parser.add_argument(
      '--num_eval_steps',
      help='Number of steps to run evalution for at each checkpoint',
      default=100,
      type=int
  )
  parser.add_argument(
      '--output_dir',
      help='GCS location to write checkpoints and export models',
      required=True
  )
  parser.add_argument(
      '--job-dir',
      help='this model ignores this field, but it is required by gcloud',
      default='junk'
  )

  args = parser.parse_args()
  arguments = args.__dict__

  # unused args provided by service
  arguments.pop('job_dir', None)
  arguments.pop('job-dir', None)

  output_directory = arguments.pop('output_dir')

  # Run the training job
  contrib_learn.learn_runner.run(
      make_experiment_fn(**arguments), output_directory)
