#
# Copyright 2020 Google LLC
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

# This is a copy from tensorflow/python/util/protobuf/compare.py
# which is based on the original internal google sources with changes
# reapplied by hand.
"""Test utility functions for comparing proto2 messages in Python.

assertProtoEqual() is useful for unit tests.  It produces much more helpful
output than assertEqual() for proto2 messages, e.g. this:

  outer {
    inner {
-     strings: "x"
?               ^
+     strings: "y"
?               ^
    }
  }

...compared to the default output from assertEqual() that looks like this:

AssertionError: <my.Msg object at 0x9fb353c> != <my.Msg object at 0x9fb35cc>

Call it inside your unit test's absltest.TestCase subclasses like this:

  from fhir.py.test_util import protobuf_compare

  class MyTest(absltest.TestCase):
    ...
    def testXXX(self):
      ...
      compare.assertProtoEqual(self, a, b)

Alternatively:

  from fhir.py.test_util import protobuf_compare

  class MyTest(protobuf_compare.ProtoAssertions, absltest.TestCase):
    ...
    def testXXX(self):
      ...
      self.assertProtoEqual(a, b)
"""

from typing import cast, Optional, TypeVar

from google.protobuf import descriptor
from google.protobuf import descriptor_pool
from google.protobuf import message
from google.protobuf import text_format
from absl.testing import absltest

_T = TypeVar('_T', bound=message.Message)


# pylint: disable=invalid-name
def assertProtoEqual(self,
                     a: message.Message,
                     b: message.Message,
                     check_initialized: bool = True,
                     normalize_numbers: bool = False,
                     msg: Optional[str] = None):
  """Fails with a useful error if a and b aren't equal.

  Comparison of repeated fields matches the semantics of
  unittest.TestCase.assertEqual(), ie order and extra duplicates fields matter.

  Args:
    self: absltest.TestCase
    a: proto2 PB instance, or text string representing one.
    b: proto2 PB instance -- message.Message or subclass thereof.
    check_initialized: boolean, whether to fail if either a or b isn't
      initialized.
    normalize_numbers: boolean, whether to normalize types and precision of
      numbers before comparison.
    msg: if specified, is used as the error message on failure.
  """
  pool = descriptor_pool.Default()
  if isinstance(a, str):
    a = text_format.Merge(a, b.__class__(), descriptor_pool=pool)

  for pb in a, b:
    if check_initialized:
      errors = pb.FindInitializationErrors()
      if errors:
        cast(absltest.TestCase,
             self).fail(f'Initialization errors: {errors}\n{pb}')
    if normalize_numbers:
      normalize_number_fields(pb)

  cast(absltest.TestCase, self).assertMultiLineEqual(
      text_format.MessageToString(a, descriptor_pool=pool),
      text_format.MessageToString(b, descriptor_pool=pool),
      msg=msg)


def normalize_number_fields(pb: _T) -> _T:
  """Normalizes types and precisions of number fields in a protocol buffer.

  Due to subtleties in the python protocol buffer implementation, it is possible
  for values to have different types and precision depending on whether they
  were set and retrieved directly or deserialized from a protobuf. This function
  normalizes integer values to ints and longs based on width, 32-bit floats to
  five digits of precision to account for python always storing them as 64-bit,
  and ensures doubles are floating point for when they're set to integers.

  Modifies pb in place. Recurses into nested objects.

  Args:
    pb: proto2 message.

  Returns:
    the given pb, modified in place.
  """
  for desc, values in pb.ListFields():
    is_repeated = True
    if desc.label is not descriptor.FieldDescriptor.LABEL_REPEATED:
      is_repeated = False
      values = [values]

    normalized_values = None

    # We force 32-bit values to int and 64-bit values to long to make
    # alternate implementations where the distinction is more significant
    # (e.g. the C++ implementation) simpler.
    if desc.type in (descriptor.FieldDescriptor.TYPE_INT64,
                     descriptor.FieldDescriptor.TYPE_UINT64,
                     descriptor.FieldDescriptor.TYPE_SINT64):
      normalized_values = [int(x) for x in values]
    elif desc.type in (descriptor.FieldDescriptor.TYPE_INT32,
                       descriptor.FieldDescriptor.TYPE_UINT32,
                       descriptor.FieldDescriptor.TYPE_SINT32,
                       descriptor.FieldDescriptor.TYPE_ENUM):
      normalized_values = [int(x) for x in values]
    elif desc.type == descriptor.FieldDescriptor.TYPE_FLOAT:
      normalized_values = [round(x, 6) for x in values]
    elif desc.type == descriptor.FieldDescriptor.TYPE_DOUBLE:
      normalized_values = [round(float(x), 7) for x in values]

    if normalized_values is not None:
      if is_repeated:
        pb.ClearField(desc.name)
        getattr(pb, desc.name).extend(normalized_values)
      else:
        setattr(pb, desc.name, normalized_values[0])

    if (desc.type == descriptor.FieldDescriptor.TYPE_MESSAGE or
        desc.type == descriptor.FieldDescriptor.TYPE_GROUP):
      if (desc.type == descriptor.FieldDescriptor.TYPE_MESSAGE and
          desc.message_type.has_options and
          desc.message_type.GetOptions().map_entry):
        # This is a map, only recurse if the values have a message type.
        if (desc.message_type.fields_by_number[2].type ==
            descriptor.FieldDescriptor.TYPE_MESSAGE):
          for v in values:
            normalize_number_fields(v)
      else:
        for v in values:
          # recursive step
          normalize_number_fields(v)

  return pb


class ProtoAssertions:
  """Mix this into a absltest.TestCase class to get proto2 assertions.

  Usage:

  class SomeTestCase(protobuf_compare.ProtoAssertions, absltest.TestCase):
    ...
    def testSomething(self):
      ...
      self.assertProtoEqual(a, b)

  See module-level definitions for method documentation.
  """

  # pylint: disable=invalid-name
  def assertProtoEqual(self, *args, **kwargs):
    assertProtoEqual(self, *args, **kwargs)
