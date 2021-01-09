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
"""Test proto_utils functionality."""

from absl.testing import absltest
from proto.google.fhir.proto.r4 import uscore_pb2
from proto.google.fhir.proto.r4.core import codes_pb2
from proto.google.fhir.proto.r4.core import datatypes_pb2
from proto.google.fhir.proto.r4.core.resources import patient_pb2
from google.fhir.utils import proto_utils

# TODO: These util tests should not be FHIR-specific.


class ProtoUtilsTest(absltest.TestCase):
  """Unit tests for functionality in proto_utils.py."""

  def testAreSameMessageType_withSameMessageType_returnsTrue(self):
    """Test are_same_message_type with the same message types."""
    patient_a = patient_pb2.Patient()
    patient_b = patient_pb2.Patient()
    self.assertTrue(
        proto_utils.are_same_message_type(patient_a.DESCRIPTOR,
                                          patient_b.DESCRIPTOR))
    self.assertTrue(
        proto_utils.are_same_message_type(patient_pb2.Patient.DESCRIPTOR,
                                          patient_pb2.Patient.DESCRIPTOR))

  def testAreSameMessageType_withDifferentMessageType_returnsFalse(self):
    """Test are_same_message_type with two different message types."""
    patient = patient_pb2.Patient()
    uscore_patient_profile = uscore_pb2.USCorePatientProfile()
    self.assertFalse(
        proto_utils.are_same_message_type(patient.DESCRIPTOR,
                                          uscore_patient_profile.DESCRIPTOR))
    self.assertFalse(
        proto_utils.are_same_message_type(patient_pb2.Patient.DESCRIPTOR,
                                          uscore_patient_profile.DESCRIPTOR))

  def testMessageIsType_withActualMessageType_returnsTrue(self):
    """Test MessageIsType functionality against the proper FHIR type."""
    patient = patient_pb2.Patient()
    self.assertTrue(proto_utils.is_message_type(patient, patient_pb2.Patient))

    boolean = datatypes_pb2.Boolean()
    self.assertTrue(proto_utils.is_message_type(boolean, datatypes_pb2.Boolean))

  def testMessageIsType_withDifferentMessageType_returnsFalse(self):
    """Test MessageIsType functionality against a different FHIR type."""
    patient = patient_pb2.Patient()
    self.assertFalse(
        proto_utils.is_message_type(patient, datatypes_pb2.Boolean))

    boolean = datatypes_pb2.Boolean()
    self.assertFalse(proto_utils.is_message_type(boolean, patient_pb2.Patient))

  def testFieldContentLength_withRepeatedField_returnsContentLength(self):
    """Test field_content_length functionality on repeated field input."""
    patient = self._create_patient_with_names(["A", "B", "C"])
    self.assertEqual(proto_utils.field_content_length(patient, "name"), 3)

  def testFieldContentLength_withSingularField_returnsSingleCount(self):
    """Test field_content_length functionality on singular field input."""
    patient = patient_pb2.Patient(active=datatypes_pb2.Boolean(value=True))
    self.assertEqual(proto_utils.field_content_length(patient, "active"), 1)

  def testFieldContentLength_withNonExistentField_returnsZero(self):
    """Test field_content_length functionality on non-existent field input."""
    default_patient = patient_pb2.Patient()  # Leave active unset
    self.assertEqual(
        proto_utils.field_content_length(default_patient, "active"), 0)

  def testFieldIsSet_withSetField_returnsTrue(self):
    """Test field_is_set with a set field."""
    patient = patient_pb2.Patient(active=datatypes_pb2.Boolean(value=True))
    self.assertTrue(proto_utils.field_is_set(patient, "active"))

  def testFieldIsSet_withUnsetField_returnsFalse(self):
    """Test field_is_set with an unset field."""
    default_patient = patient_pb2.Patient()  # Leave active unset
    self.assertFalse(proto_utils.field_is_set(default_patient, "active"))

  def testSetInParentOrAdd_withSingularComposite_returnsMessage(self):
    """Test set_in_parent_or_add with a singlular composite field."""
    patient = patient_pb2.Patient()
    self.assertFalse(proto_utils.field_is_set(patient, "active"))

    active_set_in_parent = proto_utils.set_in_parent_or_add(patient, "active")
    self.assertTrue(proto_utils.field_is_set(patient, "active"))
    self.assertFalse(active_set_in_parent.value)
    self.assertFalse(patient.active.value)

    active_set_in_parent.value = True
    self.assertTrue(active_set_in_parent.value)
    self.assertTrue(patient.active.value)

  def testSetInParentOrAdd_withRepeatedComposite_returnsMessage(self):
    """Test set_in_parent_or_add with repeated composite field."""
    patient = patient_pb2.Patient()
    self.assertFalse(proto_utils.field_is_set(patient, "name"))

    name_set_in_parent = proto_utils.set_in_parent_or_add(patient, "name")
    self.assertTrue(proto_utils.field_is_set(patient, "name"))
    self.assertEmpty(name_set_in_parent.text.value)
    self.assertEmpty(patient.name[0].text.value)

    name_set_in_parent.text.value = "Foo"
    self.assertEqual(name_set_in_parent.text.value, "Foo")
    self.assertEqual(patient.name[0].text.value, "Foo")

  def testSetInParentOrAdd_withSingularPrimitive_raises(self):
    """Test set_in_parent_or_add with singular proto primitive."""
    boolean = datatypes_pb2.Boolean()
    with self.assertRaises(ValueError) as ve:
      proto_utils.set_in_parent_or_add(boolean, "value")

    self.assertIsInstance(ve.exception, ValueError)

  def testGetValueAtField_withSingularPrimitive_returnsValue(self):
    """Test get_value_at_field with a basic singular primitive field."""
    arbitrary_string = datatypes_pb2.String(value="foo")
    result = proto_utils.get_value_at_field(arbitrary_string, "value")
    self.assertEqual(result, "foo")

  def testGetValueAtField_withSingularComposite_returnsValue(self):
    """Test get_value_at_field with a singular composite field."""
    active_value = datatypes_pb2.Boolean(value=True)
    patient = patient_pb2.Patient(active=active_value)
    result = proto_utils.get_value_at_field(patient, "active")
    self.assertEqual(result, active_value)

  def testGetValueAtField_withRepeatedComposite_returnsList(self):
    """Test get_value_at_field with a repeated composite field."""
    patient_names = [
        datatypes_pb2.HumanName(text=datatypes_pb2.String(value="Foo")),
        datatypes_pb2.HumanName(text=datatypes_pb2.String(value="Bar")),
        datatypes_pb2.HumanName(text=datatypes_pb2.String(value="Bats")),
    ]
    patient = patient_pb2.Patient(name=patient_names)
    result = proto_utils.get_value_at_field(patient, "name")
    self.assertEqual(result, patient_names)

  def testGetValueAtFieldIndex_withRepeatedField_returnsValueAtIndex(self):
    """Test get_value_at_field_index with a repeated field."""
    patient = self._create_patient_with_names(["A", "B", "C"])
    result = proto_utils.get_value_at_field_index(patient, "name", 1)
    self.assertEqual(result.text.value, "B")

  def testGetValueAtFieldIndex_withSingularField_returnsValue(self):
    """Test get_value_at_field_index with a singular field."""
    patient = patient_pb2.Patient(active=datatypes_pb2.Boolean(value=True))
    result = proto_utils.get_value_at_field_index(patient, "active", 0)
    self.assertTrue(result.value)

  def testGetValueAtFieldIndex_invalidIndex_raisesException(self):
    """Test get_value_at_field_index with an invalid index."""
    patient = patient_pb2.Patient(active=datatypes_pb2.Boolean(value=True))
    with self.assertRaises(ValueError) as ve:
      proto_utils.get_value_at_field_index(patient, "active", 1)

    self.assertIsInstance(ve.exception, ValueError)

  def testGetValueAtFieldName_invalidName_raisesException(self):
    arbitrary_string = datatypes_pb2.String(value="foo")
    with self.assertRaises(ValueError) as ve:
      proto_utils.get_value_at_field(arbitrary_string, "notvalue")

    self.assertIsInstance(ve.exception, ValueError)

  def testAppendValueAtField_repeatedCompositeValue_appendsValue(self):
    """Test append_value_at_field with a repeated composite type."""
    patient = patient_pb2.Patient()

    patient_names = [
        datatypes_pb2.HumanName(text=datatypes_pb2.String(value="Foo")),
        datatypes_pb2.HumanName(text=datatypes_pb2.String(value="Bar")),
        datatypes_pb2.HumanName(text=datatypes_pb2.String(value="Bats")),
    ]
    self.assertEqual(proto_utils.field_content_length(patient, "name"), 0)

    for name in patient_names:
      proto_utils.append_value_at_field(patient, "name", name)

    self.assertEqual(proto_utils.field_content_length(patient, "name"), 3)
    self.assertEqual(patient.name[:], patient_names)

  def testAppendValueAtField_singularCompositeValue_raises(self):
    """Test append_value_at_field with a singular composite type."""
    patient = patient_pb2.Patient()
    active = datatypes_pb2.Boolean(value=True)

    with self.assertRaises(ValueError) as ve:
      proto_utils.append_value_at_field(patient, "active", active)

    self.assertIsInstance(ve.exception, ValueError)

  def testSetValueAtField_singlePrimitiveValue_setsValue(self):
    """Test set_value_at_field with a singular primitive type."""
    arbitrary_string = datatypes_pb2.String(value="foo")

    self.assertEqual(arbitrary_string.value, "foo")
    proto_utils.set_value_at_field(arbitrary_string, "value", "bar")
    self.assertEqual(arbitrary_string.value, "bar")

  def testSetValueAtField_singleCompositeValue_setsValue(self):
    """Test set_value_at_field with a singular compositie type."""
    patient = patient_pb2.Patient(active=datatypes_pb2.Boolean(value=False))

    self.assertFalse(patient.active.value)
    proto_utils.set_value_at_field(patient, "active",
                                   datatypes_pb2.Boolean(value=True))
    self.assertTrue(patient.active.value)

  def testSetValueAtField_repeatedCompositeValue_setsList(self):
    """Test set_value_at_field with a repeated composite type."""
    old_names = [
        datatypes_pb2.HumanName(text=datatypes_pb2.String(value="A")),
        datatypes_pb2.HumanName(text=datatypes_pb2.String(value="B")),
        datatypes_pb2.HumanName(text=datatypes_pb2.String(value="C")),
    ]
    patient = patient_pb2.Patient(name=old_names)
    self.assertEqual(patient.name[:], old_names)

    new_names = [
        datatypes_pb2.HumanName(text=datatypes_pb2.String(value="Foo")),
        datatypes_pb2.HumanName(text=datatypes_pb2.String(value="Bar")),
        datatypes_pb2.HumanName(text=datatypes_pb2.String(value="Bats")),
    ]
    proto_utils.set_value_at_field(patient, "name", new_names)
    self.assertEqual(patient.name[:], new_names)

  def testSetValueAtFieldIndex_singleCompositeField_setsValue(self):
    """Test set_value_at_field_index with a singular composite type."""
    known_gender = patient_pb2.Patient.GenderCode(
        value=codes_pb2.AdministrativeGenderCode.MALE)
    unknown_gender = patient_pb2.Patient.GenderCode(
        value=codes_pb2.AdministrativeGenderCode.UNKNOWN)

    patient = patient_pb2.Patient(gender=unknown_gender)
    self.assertEqual(patient.gender, unknown_gender)
    proto_utils.set_value_at_field_index(patient, "gender", 0, known_gender)
    self.assertEqual(patient.gender, known_gender)

  def testSetValueAtFieldIndex_repeatCompositeField_setsValue(self):
    """Test set_value_at_field_index with a repeated composite type."""
    patient = self._create_patient_with_names(["A", "B", "C"])
    new_name = datatypes_pb2.HumanName(
        text=datatypes_pb2.String(value="Foo"),
        family=datatypes_pb2.String(value="Bar"))
    proto_utils.set_value_at_field_index(patient, "name", 1, new_name)
    self.assertEqual(patient.name[1], new_name)

  def testSetValueAtFieldIndex_singlePrimitiveField_setsValue(self):
    """Test set_value_at_field_index with a singular primitive type."""
    arbitrary_string = datatypes_pb2.String(value="foo")
    proto_utils.set_value_at_field_index(arbitrary_string, "value", 0, "bar")
    self.assertEqual(arbitrary_string.value, "bar")

  def testSetValueAtFieldIndex_invalidIndex_raisesException(self):
    """Test set_value_at_field_index with an invalid index."""
    patient = self._create_patient_with_names(["A", "B", "C"])
    new_name = datatypes_pb2.HumanName(
        text=datatypes_pb2.String(value="Foo"),
        family=datatypes_pb2.String(value="Bar"))

    with self.assertRaises(ValueError) as ve:
      proto_utils.set_value_at_field_index(patient, "name", 3, new_name)

    self.assertIsInstance(ve.exception, ValueError)

  def testCopyCommonField_differentMessageTypes_succeeds(self):
    """Tests that copy_common_field succeeds on a single Message field."""
    string_value = datatypes_pb2.String(id=datatypes_pb2.String(value="foo"))
    boolean_value = datatypes_pb2.Boolean(id=datatypes_pb2.String(value="bar"))

    # Before copy
    self.assertEqual(string_value.id.value, "foo")
    self.assertEqual(boolean_value.id.value, "bar")

    proto_utils.copy_common_field(string_value, boolean_value, "id")

    # After copy
    self.assertEqual(string_value.id.value, "foo")
    self.assertEqual(boolean_value.id.value, "foo")

  def testCopyCommonField_notPresentInBothMessages_raisesException(self):
    """Tests copy_common_field with an invalid descriptor raises."""
    first_patient = patient_pb2.Patient(
        active=datatypes_pb2.Boolean(value=True))
    second_patient = patient_pb2.Patient(
        active=datatypes_pb2.Boolean(value=False))

    with self.assertRaises(ValueError) as ve:
      proto_utils.copy_common_field(first_patient, second_patient, "value")
    self.assertIsInstance(ve.exception, ValueError)

  def testGetMessageClassFromDescriptor_returnsMessageClass(self):
    """Tests that the correct class is returned for a message."""
    actual = proto_utils.get_message_class_from_descriptor(
        patient_pb2.Patient.DESCRIPTOR)
    self.assertTrue(
        proto_utils.are_same_message_type(actual.DESCRIPTOR,
                                          patient_pb2.Patient.DESCRIPTOR))

  def testCreateMessageFromDescriptor_returnsMessage(self):
    """Tests that the correct class is returned for a message."""
    self.assertEqual(
        proto_utils.create_message_from_descriptor(
            patient_pb2.Patient.DESCRIPTOR), patient_pb2.Patient())

  def testCreateMessageFromDescriptor_withArguments_returnsMessage(self):
    """Tests that the correct class is instantiated with kwargs."""

    patient_name = datatypes_pb2.HumanName(
        text=datatypes_pb2.String(value="Foo"),
        family=datatypes_pb2.String(value="Bar"))
    expected_patient = patient_pb2.Patient(name=[patient_name])
    actual_patient = proto_utils.create_message_from_descriptor(
        patient_pb2.Patient.DESCRIPTOR, name=[patient_name])
    self.assertEqual(expected_patient, actual_patient)

  def _create_patient_with_names(self, names):
    patient = patient_pb2.Patient()
    for name in names:
      patient.name.append(
          datatypes_pb2.HumanName(text=datatypes_pb2.String(value=name)))
    return patient


if __name__ == "__main__":
  absltest.main()
