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

"""Define the input_fn and estimator."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function


import tensorflow as tf
from tensorflow.contrib import data as contrib_data
from tensorflow.contrib import estimator as contrib_estimator
from tensorflow.contrib import learn as contrib_learn
from tensorflow.contrib import lookup as contrib_lookup
from tensorflow.contrib import training as contrib_training


CONTEXT_KEY_PREFIX = 'c-'
SEQUENCE_KEY_PREFIX = 's-'

AGE_KEY = 'Patient.ageInYears'


def _example_index_to_sparse_index(example_indices, batch_size):
  """Creates a sparse index tensor from a list of example indices.

  For example, this would do the transformation:
  [0, 0, 0, 1, 3, 3] -> [[0,0], [0,1], [0,2], [1,0], [3,0], [3,1]]

  The second column of the output tensor is the running count of the occurrences
  of that example index.

  Args:
    example_indices: A sorted 1D Tensor with example indices.
    batch_size: The batch_size. Could be larger than max(example_indices) if the
      last examples of the batch do not have the feature present.
  Returns:
    The sparse index tensor.
    The maxmium length of a row in this tensor.
  """
  binned_counts = tf.bincount(example_indices, minlength=batch_size)
  max_len = tf.to_int64(tf.reduce_max(binned_counts))
  return tf.where(tf.sequence_mask(binned_counts)), max_len


def _dedup_tensor(sp_tensor):
  """Dedup values of a SparseTensor along each row.

  Args:
    sp_tensor: A 2D SparseTensor to be deduped.
  Returns:
    A deduped SparseTensor of shape [batch_size, max_len], where max_len is
    the maximum number of unique values for a row in the Tensor.
  """
  string_batch_index = tf.as_string(sp_tensor.indices[:, 0])

  # tf.unique only works on 1D tensors. To avoid deduping across examples,
  # prepend each feature value with the example index. This requires casting
  # to and from strings for non-string features.
  string_values = sp_tensor.values
  original_dtype = sp_tensor.values.dtype
  if original_dtype != tf.string:
    string_values = tf.as_string(sp_tensor.values)
  index_and_value = tf.strings.join([string_batch_index, string_values],
                                    separator='|')
  unique_index_and_value, _ = tf.unique(index_and_value)

  # split is a shape [tf.size(values), 2] tensor. The first column contains
  # indices and the second column contains the feature value (we assume no
  # feature contains | so we get exactly 2 values from the string split).
  split = tf.string_split(unique_index_and_value, delimiter='|')
  split = tf.reshape(split.values, [-1, 2])
  string_indices = split[:, 0]
  values = split[:, 1]

  indices = tf.reshape(
      tf.string_to_number(string_indices, out_type=tf.int32), [-1])
  if original_dtype != tf.string:
    values = tf.string_to_number(values, out_type=original_dtype)
  values = tf.reshape(values, [-1])
  # Convert example indices into SparseTensor indices, e.g.
  # [0, 0, 0, 1, 3, 3] -> [[0,0], [0,1], [0,2], [1,0], [3,0], [3,1]]
  batch_size = tf.to_int32(sp_tensor.dense_shape[0])
  new_indices, max_len = _example_index_to_sparse_index(indices, batch_size)
  return tf.SparseTensor(
      indices=tf.to_int64(new_indices),
      values=values,
      dense_shape=[tf.to_int64(batch_size), max_len])


def _make_parsing_fn(mode, label_name, include_age,
                     categorical_context_features, sequence_features,
                     time_crossed_features):
  """Creates an input function to an estimator.

  Args:
    mode: The execution mode, as defined in tf.estimator.ModeKeys.
    label_name: Name of the label present as context feature in the
      SequenceExamples.
    include_age: Whether to include the age_in_years as a feature.
    categorical_context_features: List of string context features that are valid
      keys in the tf.SequenceExample.
    sequence_features: List of sequence features (strings) that are valid keys
      in the tf.SequenceExample.
    time_crossed_features: List of list of sequence features (strings) that
      should be crossed at each step along the time dimension.

  Returns:
    Two dictionaries with the parsing config for the context features and
    sequence features.
  """
  sequence_features_config = dict()
  for feature in sequence_features:
    dtype = tf.string
    if feature == 'Observation.value.quantity.value':
      dtype = tf.float32
    sequence_features_config[feature] = tf.VarLenFeature(dtype)

  sequence_features_config['eventId'] = tf.FixedLenSequenceFeature(
      [], tf.int64, allow_missing=False)
  for cross in time_crossed_features:
    for feature in cross:
      dtype = tf.string
      if feature == 'Observation.value.quantity.value':
        dtype = tf.float32
      sequence_features_config[feature] = tf.VarLenFeature(dtype)
  context_features_config = dict()
  if include_age:
    context_features_config['timestamp'] = tf.FixedLenFeature(
        [], tf.int64, default_value=-1)
    context_features_config['Patient.birthDate'] = tf.FixedLenFeature(
        [], tf.int64, default_value=-1)
  context_features_config['sequenceLength'] = tf.FixedLenFeature(
      [], tf.int64, default_value=-1)

  for context_feature in categorical_context_features:
    context_features_config[context_feature] = tf.VarLenFeature(tf.string)
  if mode != tf.estimator.ModeKeys.PREDICT:
    context_features_config[label_name] = tf.FixedLenFeature(
        [], tf.string, default_value='MISSING')

  def _parse_fn_old(serialized_example):
    """Parses tf.(Sparse)Tensors from the serialized tf.SequenceExample.

    Also works with TF versions < 1.12 but is slower than _parse_fn_new.

    Args:
      serialized_example: A single serialized tf.SequenceExample.

    Returns:
      A dictionary from name to (Sparse)Tensors of the context and sequence
      features.
    """
    context, sequence = tf.parse_single_sequence_example(
        serialized_example,
        context_features=context_features_config,
        sequence_features=sequence_features_config,
        example_name='parsing_examples')
    feature_map = dict()
    for k, v in context.items():
      feature_map[CONTEXT_KEY_PREFIX + k] = v
    for k, v in sequence.items():
      feature_map[SEQUENCE_KEY_PREFIX + k] = v
    return feature_map

  def _parse_fn_new(serialized_examples):
    """Parses tf.(Sparse)Tensors from the serialized tf.SequenceExamples.

    Requires TF versions >= 1.12 but is faster than _parse_fn_old.

    Args:
      serialized_examples: A batch of serialized tf.SequenceExamples.

    Returns:
      A dictionary from name to (Sparse)Tensors of the context and sequence
      features.
    """
    context, sequence, _ = tf.io.parse_sequence_example(
        serialized_examples,
        context_features=context_features_config,
        sequence_features=sequence_features_config,
        name='parse_sequence_example')
    feature_map = dict()
    for k, v in context.items():
      feature_map[CONTEXT_KEY_PREFIX + k] = v
    for k, v in sequence.items():
      feature_map[SEQUENCE_KEY_PREFIX + k] = v
    return feature_map
  parse_fn = _parse_fn_new if tf.__version__ >= '1.12.0' else _parse_fn_old
  return parse_fn


def _make_feature_engineering_fn(dedup, time_windows, include_age,
                                 sequence_features, time_crossed_features):
  """Creates an input function to an estimator.

  Args:
    dedup: Whether to remove duplicate values.
    time_windows: List of time windows - we bucket all sequence features by
      their age into buckets [time_windows[i], time_windows[i+1]).
    include_age: Whether to include the age_in_years as a feature.
    sequence_features: List of sequence features (strings) that are valid keys
      in the tf.SequenceExample.
    time_crossed_features: List of list of sequence features (strings) that
      should be crossed at each step along the time dimension.

  Returns:
    Two dictionaries with the parsing config for the context features and
    sequence features.
  """
  def _process(examples):
    """Supplies input to our model.

    This function supplies input to our model after parsing.

    Args:
      examples: The dictionary from key to (Sparse)Tensors with context
        and sequence features.

    Returns:
      A tuple consisting of 1) a dictionary of tensors whose keys are
      the feature names, and 2) a tensor of target labels if the mode
      is not INFER (and None, otherwise).
    """
    # Combine into a single dictionary.
    feature_map = {}
    # Add age if requested.
    if include_age:
      age_in_seconds = (
          examples[CONTEXT_KEY_PREFIX + 'timestamp'] -
          examples.pop(CONTEXT_KEY_PREFIX + 'Patient.birthDate'))
      age_in_years = tf.to_float(age_in_seconds) / (60 * 60 * 24 * 365.0)
      feature_map[CONTEXT_KEY_PREFIX + AGE_KEY] = age_in_years

    sequence_length = examples.pop(CONTEXT_KEY_PREFIX + 'sequenceLength')
    # Cross the requested features.
    for cross in time_crossed_features:
      # The features may be missing at different rates - we take the union
      # of the indices supplying defaults.
      extended_features = dict()
      dense_shape = tf.concat(
          [[tf.to_int64(tf.shape(sequence_length)[0])],
           [tf.reduce_max(sequence_length)],
           tf.constant([1], dtype=tf.int64)],
          axis=0)
      for i, feature in enumerate(cross):
        sp_tensor = examples[SEQUENCE_KEY_PREFIX + feature]
        additional_indices = []
        covered_indices = sp_tensor.indices
        for j, other_feature in enumerate(cross):
          if i != j:
            additional_indices.append(
                tf.sets.set_difference(
                    tf.sparse_reorder(
                        tf.SparseTensor(
                            indices=examples[SEQUENCE_KEY_PREFIX +
                                             other_feature].indices,
                            values=tf.zeros([
                                tf.shape(examples[SEQUENCE_KEY_PREFIX +
                                                  other_feature].indices)[0]
                            ],
                                            dtype=tf.int32),
                            dense_shape=dense_shape)),
                    tf.sparse_reorder(
                        tf.SparseTensor(
                            indices=covered_indices,
                            values=tf.zeros([tf.shape(covered_indices)[0]],
                                            dtype=tf.int32),
                            dense_shape=dense_shape))).indices)
            covered_indices = tf.concat(
                [sp_tensor.indices] + additional_indices, axis=0)

        additional_indices = tf.concat(additional_indices, axis=0)

        # Supply defaults for all other indices.
        default = tf.tile(
            tf.constant(['n/a']),
            multiples=[tf.shape(additional_indices)[0]])

        string_value = sp_tensor.values
        if string_value.dtype != tf.string:
          string_value = tf.as_string(string_value)

        extended_features[feature] = tf.sparse_reorder(
            tf.SparseTensor(
                indices=tf.concat([sp_tensor.indices, additional_indices],
                                  axis=0),
                values=tf.concat([string_value, default], axis=0),
                dense_shape=dense_shape))

      new_values = tf.strings.join(
          [extended_features[f].values for f in cross], separator='-')
      crossed_sp_tensor = tf.sparse_reorder(
          tf.SparseTensor(
              indices=extended_features[cross[0]].indices,
              values=new_values,
              dense_shape=extended_features[cross[0]].dense_shape))
      examples[SEQUENCE_KEY_PREFIX + '_'.join(cross)] = crossed_sp_tensor
    # Remove unwanted features that are used in the cross but should not be
    # considered outside the cross.
    for cross in time_crossed_features:
      for feature in cross:
        if (feature not in sequence_features and
            SEQUENCE_KEY_PREFIX + feature in examples):
          del examples[SEQUENCE_KEY_PREFIX + feature]

    # Flatten sparse tensor to compute event age. This dense tensor also
    # contains padded values. These will not be used when gathering elements
    # from the dense tensor since each sparse feature won't have a value
    # defined for the padding.
    padded_event_age = (
        # Broadcast current time along sequence dimension.
        tf.expand_dims(examples.pop(CONTEXT_KEY_PREFIX + 'timestamp'), 1)
        # Subtract time of events.
        - examples.pop(SEQUENCE_KEY_PREFIX + 'eventId'))

    for i in range(len(time_windows) - 1):
      max_age = time_windows[i]
      min_age = time_windows[i+1]
      padded_in_time_window = tf.logical_and(padded_event_age <= max_age,
                                             padded_event_age > min_age)

      for k, v in examples.iteritems():
        if k.startswith(CONTEXT_KEY_PREFIX):
          continue
        # For each sparse feature entry, look up whether it is in the time
        # window or not.
        in_time_window = tf.gather_nd(padded_in_time_window,
                                      v.indices[:, 0:2])
        v = tf.sparse_retain(v, in_time_window)
        sp_tensor = tf.sparse_reshape(v, [v.dense_shape[0], -1])
        if dedup:
          sp_tensor = _dedup_tensor(sp_tensor)

        feature_map[k + '-til-%d' %min_age] = sp_tensor

    for k, v in examples.iteritems():
      if k.startswith(CONTEXT_KEY_PREFIX):
        feature_map[k] = v
    return feature_map
  return _process


def get_input_fn(mode,
                 input_files,
                 label_name,
                 dedup,
                 time_windows,
                 include_age,
                 categorical_context_features,
                 sequence_features,
                 time_crossed_features,
                 batch_size,
                 shuffle=True):
  """Creates an input function to an estimator.

  Args:
    mode: The execution mode, as defined in tf.estimator.ModeKeys.
    input_files: List of input files in TFRecord format containing
      tf.SequenceExamples.
    label_name: Name of the label present as context feature in the
      SequenceExamples.
    dedup: Whether to remove duplicate values.
    time_windows: List of time windows - we bucket all sequence features by
      their age into buckets [time_windows[i], time_windows[i+1]).
    include_age: Whether to include the age_in_years as a feature.
    categorical_context_features: List of string context features that are valid
      keys in the tf.SequenceExample.
    sequence_features: List of sequence features (strings) that are valid keys
      in the tf.SequenceExample.
    time_crossed_features: List of list of sequence features (strings) that
      should be crossed at each step along the time dimension.
    batch_size: The size of the batch when reading in data.
    shuffle: Whether to shuffle the examples.

  Returns:
    A function that returns a dictionary of features and the target labels.
  """

  def input_fn():
    """Supplies input to our model.

    This function supplies input to our model, where this input is a
    function of the mode. For example, we supply different data if
    we're performing training versus evaluation.

    Returns:
      A tuple consisting of 1) a dictionary of tensors whose keys are
      the feature names, and 2) a tensor of target labels if the mode
      is not INFER (and None, otherwise).
    """
    is_training = mode == tf.estimator.ModeKeys.TRAIN
    num_epochs = None if is_training else 1

    with tf.name_scope('read_batch'):
      file_names = input_files
      files = tf.data.Dataset.list_files(file_names)
      if shuffle:
        files = files.shuffle(buffer_size=len(file_names))
      dataset = (
          files.apply(
              contrib_data.parallel_interleave(
                  tf.data.TFRecordDataset, cycle_length=10)).repeat(num_epochs))
      if shuffle:
        dataset = dataset.shuffle(buffer_size=100)
      parse_fn = _make_parsing_fn(
          mode, label_name, include_age, categorical_context_features,
          sequence_features, time_crossed_features)
      feature_engineering_fn = _make_feature_engineering_fn(
          dedup, time_windows, include_age, sequence_features,
          time_crossed_features)
      if tf.__version__ < '1.12.0':
        dataset = dataset.map(parse_fn, num_parallel_calls=8)
        feature_map = (dataset
                       .prefetch(buffer_size=batch_size)
                       .make_one_shot_iterator()
                       .get_next())
        # Batch with padding.
        feature_map = tf.train.batch(
            feature_map,
            batch_size,
            num_threads=8,
            capacity=2,
            enqueue_many=False,
            dynamic_pad=True)
        feature_map = feature_engineering_fn(feature_map)
      else:
        feature_map = (dataset
                       .batch(batch_size)
                       # Parallelize the input processing and put it behind a
                       # queue to increase performance by removing it from the
                       # critical path of per-step-computation.
                       .map(parse_fn, num_parallel_calls=8)
                       .map(feature_engineering_fn, num_parallel_calls=8)
                       .prefetch(buffer_size=1)
                       .make_one_shot_iterator()
                       .get_next())
      label = None
      if mode != tf.estimator.ModeKeys.PREDICT:
        label = feature_map.pop(CONTEXT_KEY_PREFIX +
                                label_name)
      return feature_map, label
  return input_fn


def get_serving_input_fn(dedup,
                         time_windows,
                         include_age,
                         categorical_context_features,
                         sequence_features,
                         time_crossed_features):
  """Creates an input function to an estimator.

  Args:
    dedup: Whether to remove duplicate values.
    time_windows: List of time windows - we bucket all sequence features by
      their age into buckets [time_windows[i], time_windows[i+1]).
    include_age: Whether to include the age_in_years as a feature.
    categorical_context_features: List of string context features that are valid
      keys in the tf.SequenceExample.
    sequence_features: List of sequence features (strings) that are valid keys
      in the tf.SequenceExample.
    time_crossed_features: List of list of sequence features (strings) that
      should be crossed at each step along the time dimension.

  Returns:
    A function that returns a dictionary of features and the target labels.
  """

  parse_fn = _make_parsing_fn(
      tf.estimator.ModeKeys.PREDICT, '', include_age,
      categorical_context_features, sequence_features, time_crossed_features)
  feature_engineering_fn = _make_feature_engineering_fn(
      dedup, time_windows, include_age, sequence_features,
      time_crossed_features)

  def example_serving_input_fn():
    """Build the serving inputs."""
    shape = [None] if tf.__version__ >= '1.12.0' else [1]
    examples = tf.placeholder(
        shape=shape,
        dtype=tf.string,
        name='input_examples')
    if tf.__version__ < '1.12.0':
      examples = tf.squeeze(examples)
    feature_map = parse_fn(examples)
    if tf.__version__ < '1.12.0':
      # Add a batch dimension of 1.
      keys = feature_map.keys()
      for k in keys:
        t = feature_map[k]
        if isinstance(t, tf.SparseTensor):
          t = sparse_expand_dims(t, axis=0)
        else:
          t = tf.expand_dims(t, axis=0)
        feature_map[k] = t

    feature_map = feature_engineering_fn(feature_map)
    return tf.estimator.export.ServingInputReceiver(
        feature_map, {'input_examples': examples})
  return example_serving_input_fn


def sparse_expand_dims(s_tensor, axis=0, index_value=None):
  """Add a new dimension to a sparse tensor while setting its index.

  For example, if s is a 2d sparse tensor with 1 at (0, 0) and 2 at (8, 7) then
  sparse_expand_and_set_dim(s, axis=1, index_value=4) will return a 3d sparse
  tensor with 1 at (0, 4, 0) and 2 at (8, 4, 7).

  Args:
    s_tensor: A SparseTensor.
    axis: The new axis, default is 0.
    index_value: The index to put in the new axis, default is 0.

  Returns:
    A SparseTensor.
  """
  if tf.__version__ >= '1.12.0':
    return tf.sparse.expand_dims(s_tensor, axis, index_value)
  if index_value is None:
    index_value = 0

  indices = s_tensor.indices
  shape = tf.shape(indices)
  indices = tf.concat(
      [
          indices[:, :axis],
          tf.to_int64(tf.tile([[index_value]], multiples=[shape[0], 1])),
          indices[:, axis:]
      ],
      axis=1)
  shape = tf.to_int64(tf.shape(s_tensor))
  shape = tf.concat([shape[:axis], [index_value + 1], shape[axis:]], axis=0)
  shape.set_shape([s_tensor.get_shape().ndims + 1])
  return tf.SparseTensor(indices, s_tensor.values, shape)


def make_metrics(label_values):
  """Creates a function to compute precsion/recall@k metrics for each class.

  Args:
    label_values: List of strings that specify the possible values of the label.

  Returns:
    A function to compute precsion/recall@k metrics for each class.
  """

  def multiclass_metrics_fn(labels, predictions):
    """Computes precsion/recall@k metrics for each class and micro-weighted.

    Args:
      labels: A string Tensor of shape [batch_size] with the true labels
      predictions: A float Tensor of shape [batch_size, num_classes].

    Returns:
      A dictionary with metrics of precision/recall @1/2 and precision/recall
      per class.
    """

    label_ids = contrib_lookup.index_table_from_tensor(
        tuple(label_values), name='class_id_lookup').lookup(labels)

    # We convert the task to a binary one of < 7 days.
    # 'less_or_equal_3', '3_7', '7_14', 'above_14'
    binary_labels = label_ids < 2
    binary_probs = tf.reduce_sum(predictions['probabilities'][:, 0:2], axis=1)

    metrics_dict = {
        'precision_at_1':
            tf.metrics.precision_at_k(
                labels=label_ids,
                predictions=predictions['probabilities'], k=1),
        'precision_at_2':
            tf.metrics.precision_at_k(
                labels=label_ids,
                predictions=predictions['probabilities'], k=2),
        'recall_at_1':
            tf.metrics.recall_at_k(
                labels=label_ids,
                predictions=predictions['probabilities'], k=1),
        'recall_at_2':
            tf.metrics.recall_at_k(
                labels=label_ids,
                predictions=predictions['probabilities'], k=2),
        'auc_roc_at_most_7d':
            tf.metrics.auc(
                labels=binary_labels,
                predictions=binary_probs,
                curve='ROC',
                summation_method='careful_interpolation'),
        'auc_pr_at_most_7d':
            tf.metrics.auc(
                labels=binary_labels,
                predictions=binary_probs,
                curve='PR',
                summation_method='careful_interpolation'),
        'precision_at_most_7d':
            tf.metrics.precision(
                labels=binary_labels,
                predictions=binary_probs >= 0.5),
        'recall_at_most_7d':
            tf.metrics.recall(
                labels=binary_labels,
                predictions=binary_probs >= 0.5),
    }
    for i, label in enumerate(label_values):
      metrics_dict['precision_%s' % label] = tf.metrics.precision_at_k(
          labels=label_ids,
          predictions=predictions['probabilities'],
          k=1,
          class_id=i)
      metrics_dict['recall_%s' % label] = tf.metrics.recall_at_k(
          labels=label_ids,
          predictions=predictions['probabilities'],
          k=1,
          class_id=i)

    return metrics_dict
  return multiclass_metrics_fn


def make_estimator(hparams, label_values, output_dir):
  """Creates an Estimator.

  Args:
    hparams: HParams specfying the configuration of the estimator.
    label_values: List of strings that specify the possible values of the label.
    output_dir: Directory to store the model checkpoints and metrics.

  Returns:
    An Estimator.
  """
  seq_features = []
  seq_features_sizes = []
  for k, bucket_size in zip(
      hparams.sequence_features + hparams.time_crossed_features,
      hparams.sequence_bucket_sizes + hparams.time_concat_bucket_sizes):
    if 'n/a' in k:
      continue
    for max_age in hparams.time_windows[1:]:
      seq_features.append(
          tf.feature_column.categorical_column_with_hash_bucket(
              SEQUENCE_KEY_PREFIX + k.replace(':', '_') + '-til-' +
              str(max_age), bucket_size))
      seq_features_sizes.append(bucket_size)

  categorical_context_features = [
      tf.feature_column.categorical_column_with_hash_bucket(
          CONTEXT_KEY_PREFIX + k, bucket_size)
      for k, bucket_size in zip(hparams.categorical_context_features,
                                hparams.context_bucket_sizes)
  ]
  discretized_context_features = []
  if hparams.include_age:
    discretized_context_features.append(
        tf.feature_column.bucketized_column(
            tf.feature_column.numeric_column(CONTEXT_KEY_PREFIX + AGE_KEY),
            boundaries=hparams.age_boundaries))

  if hparams.optimizer == 'Ftrl':
    optimizer = tf.train.FtrlOptimizer(
        learning_rate=hparams.learning_rate,
        l1_regularization_strength=hparams.l1_regularization_strength,
        l2_regularization_strength=hparams.l2_regularization_strength)
  elif hparams.optimizer == 'Adam':
    optimizer = tf.train.AdamOptimizer(
        learning_rate=hparams.learning_rate,
        beta1=0.9,
        beta2=0.999,
        epsilon=1e-8)
  else:
    raise ValueError(
        'Invalid Optimizer %s needs to be Ftrl or Adam' % hparams.optimizer)
  run_config = contrib_learn.RunConfig(save_checkpoints_secs=180)
  if hparams.model_type == 'linear':
    estimator = tf.estimator.LinearClassifier(
        feature_columns=seq_features + categorical_context_features +
        discretized_context_features,
        n_classes=len(label_values),
        label_vocabulary=label_values,
        optimizer=optimizer,
        model_dir=output_dir,
        config=run_config,
        loss_reduction=tf.losses.Reduction.SUM_OVER_BATCH_SIZE)
  elif hparams.model_type == 'dnn':
    # Heuristically choose an embedding dimension dependent on the number of
    # of tokens of a feature.
    embed_seq_features = [
        tf.feature_column.embedding_column(
            fc, dimension=int(min(6*pow(size, 0.25), size)))
        for fc, size in zip(seq_features, seq_features_sizes)]
    embed_context_features = [
        tf.feature_column.embedding_column(
            fc, dimension=int(min(6*pow(size, 0.25), size)))
        for fc, size
        in zip(categorical_context_features, hparams.context_bucket_sizes)]
    embed_age = []
    if hparams.include_age:
      embed_age.append(tf.feature_column.embedding_column(
          discretized_context_features[0], 4))
    estimator = tf.estimator.DNNClassifier(
        feature_columns=embed_seq_features + embed_context_features +
        embed_age,
        n_classes=len(label_values),
        label_vocabulary=label_values,
        optimizer=optimizer,
        hidden_units=hparams.dnn_hidden_units,
        dropout=hparams.dnn_dropout,
        model_dir=output_dir,
        config=run_config,
        loss_reduction=tf.losses.Reduction.SUM_OVER_BATCH_SIZE)
  else:
    raise ValueError('Invalid model_type %s needs to be linear or dnn'
                     % hparams.model_type)

  return contrib_estimator.add_metrics(estimator, make_metrics(label_values))


def create_hparams(hparams_override_str=''):
  """Creates default HParams with the option of overrides.

  Args:
    hparams_override_str: String with possible overrides.

  Returns:
    Default HParams.
  """
  hparams = contrib_training.HParams(
      # Sequence features are bucketed by their age at time of prediction in:
      # [time_windows[0] - time_windows[1]),
      # [time_windows[1] - time_windows[2]),
      # ...
      time_windows=[
          5 * 365 * 24 * 60 * 60,  # 5 years
          365 * 24 * 60 * 60,  # 1 year
          30 * 24 * 60 * 60,  # 1 month
          7 * 24 * 60 * 60,  # 1 week
          1 * 24 * 60 * 60,  # 1 day
          0,  # now
      ],
      batch_size=64,
      learning_rate=0.003,
      dedup=True,
      # Currently supported optimizers are Adam and Ftrl.
      optimizer='Ftrl',
      # Note that these regularization terms are only applied for Ftrl.
      l1_regularization_strength=0.0,
      l2_regularization_strength=0.0,
      include_age=True,
      age_boundaries=[1, 5, 18, 30, 50, 70, 90],
      categorical_context_features=['Patient.gender'],
      sequence_features=[
          'Composition.section.text.div.tokenized',
          'Composition.type',
          'Condition.code',
          'Encounter.hospitalization.admitSource',
          'Encounter.reason.hcc',
          'MedicationRequest.contained.medication.code.gsn',
          'Procedure.code.cpt',
      ],
      # Number of hash buckets to map the tokens of the sequence_features into.
      sequence_bucket_sizes=[
          17000,
          16,
          3052,
          10,
          62,
          1600,
          732,
      ],
      # List of strings each of which is a ':'-separated list of feature that we
      # want to concatenate over the time dimension
      time_crossed_features=[
          '%s:%s:%s:%s' %
          ('Observation.code', 'Observation.value.quantity.value',
           'Observation.value.quantity.unit', 'Observation.value.string')
      ],
      time_concat_bucket_sizes=[39571],
      context_bucket_sizes=[4],
      # Model type needs to be linear or dnn.
      model_type='linear',
      # In case of model_type of dnn we can specify the hidden layer dimension.
      dnn_hidden_units=[256],
      # In case of model_type of dnn we can specify the dropout probability.
      dnn_dropout=0.1)
  # hparams_override_str override any of the preceding hyperparameter values.
  if hparams_override_str:
    hparams = hparams.parse(hparams_override_str)
  return hparams
