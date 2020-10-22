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
"""Concrete implementation of the FHIR PrimitiveHandler for R4 datatypes."""

from typing import Any, Dict, Optional, Type

from google.protobuf import message
from proto.google.fhir.proto.r4 import fhirproto_extensions_pb2
from proto.google.fhir.proto.r4.core import datatypes_pb2
from proto.google.fhir.proto.r4.core.resources import bundle_and_contained_resource_pb2
from google.fhir import _primitive_time_utils
from google.fhir import primitive_handler
from google.fhir.json_format.wrappers import _base64_binary
from google.fhir.json_format.wrappers import _date
from google.fhir.json_format.wrappers import _date_time
from google.fhir.json_format.wrappers import _decimal
from google.fhir.json_format.wrappers import _instant
from google.fhir.json_format.wrappers import _primitive_wrappers
from google.fhir.json_format.wrappers import _time
from google.fhir.utils import annotation_utils
from google.fhir.utils import fhir_types


class PrimitiveHandler(primitive_handler.PrimitiveHandler):
  """An implementation of PrimitiveHandler for vending R4 FHIR datatypes."""

  _DESC_TO_PRIMITIVE_WRAPPER_TYPE: Dict[
      str, Type[_primitive_wrappers.PrimitiveWrapper]] = {
          datatypes_pb2.Base64Binary.DESCRIPTOR.full_name:
              _base64_binary.Base64BinaryWrapper,
          datatypes_pb2.Boolean.DESCRIPTOR.full_name:
              _primitive_wrappers.BooleanWrapper,
          datatypes_pb2.Canonical.DESCRIPTOR.full_name:
              _primitive_wrappers.StringLikePrimitiveWrapper,
          datatypes_pb2.Date.DESCRIPTOR.full_name:
              _date.DateWrapper,
          datatypes_pb2.DateTime.DESCRIPTOR.full_name:
              _date_time.DateTimeWrapper,
          datatypes_pb2.Decimal.DESCRIPTOR.full_name:
              _decimal.DecimalWrapper,
          datatypes_pb2.Id.DESCRIPTOR.full_name:
              _primitive_wrappers.StringLikePrimitiveWrapper,
          datatypes_pb2.Instant.DESCRIPTOR.full_name:
              _instant.InstantWrapper,
          datatypes_pb2.Integer.DESCRIPTOR.full_name:
              _primitive_wrappers.IntegerLikePrimitiveWrapper,
          datatypes_pb2.Markdown.DESCRIPTOR.full_name:
              _primitive_wrappers.StringLikePrimitiveWrapper,
          datatypes_pb2.Oid.DESCRIPTOR.full_name:
              _primitive_wrappers.StringLikePrimitiveWrapper,
          datatypes_pb2.PositiveInt.DESCRIPTOR.full_name:
              _primitive_wrappers.IntegerLikePrimitiveWrapper,
          datatypes_pb2.String.DESCRIPTOR.full_name:
              _primitive_wrappers.StringLikePrimitiveWrapper,
          datatypes_pb2.Time.DESCRIPTOR.full_name:
              _time.TimeWrapper,
          datatypes_pb2.UnsignedInt.DESCRIPTOR.full_name:
              _primitive_wrappers.IntegerLikePrimitiveWrapper,
          datatypes_pb2.Uri.DESCRIPTOR.full_name:
              _primitive_wrappers.StringLikePrimitiveWrapper,
          datatypes_pb2.Url.DESCRIPTOR.full_name:
              _primitive_wrappers.StringLikePrimitiveWrapper,
          datatypes_pb2.Uuid.DESCRIPTOR.full_name:
              _primitive_wrappers.StringLikePrimitiveWrapper,
          datatypes_pb2.Xhtml.DESCRIPTOR.full_name:
              _primitive_wrappers.XhtmlWrapper,
      }

  @property
  def base64_binary_cls(self) -> Type[datatypes_pb2.Base64Binary]:
    return datatypes_pb2.Base64Binary

  @property
  def boolean_cls(self) -> Type[datatypes_pb2.Boolean]:
    return datatypes_pb2.Boolean

  @property
  def canonical_cls(self) -> Type[datatypes_pb2.Canonical]:
    return datatypes_pb2.Canonical

  @property
  def code_cls(self) -> Type[datatypes_pb2.Code]:
    return datatypes_pb2.Code

  @property
  def contained_resource_cls(
      self) -> Type[bundle_and_contained_resource_pb2.ContainedResource]:
    return bundle_and_contained_resource_pb2.ContainedResource

  @property
  def date_cls(self) -> Type[datatypes_pb2.Date]:
    return datatypes_pb2.Date

  @property
  def date_time_cls(self) -> Type[datatypes_pb2.DateTime]:
    return datatypes_pb2.DateTime

  @property
  def decimal_cls(self) -> Type[datatypes_pb2.Decimal]:
    return datatypes_pb2.Decimal

  @property
  def id_cls(self) -> Type[datatypes_pb2.Id]:
    return datatypes_pb2.Id

  @property
  def instant_cls(self) -> Type[datatypes_pb2.Instant]:
    return datatypes_pb2.Instant

  @property
  def integer_cls(self) -> Type[datatypes_pb2.Integer]:
    return datatypes_pb2.Integer

  @property
  def markdown_cls(self) -> Type[datatypes_pb2.Markdown]:
    return datatypes_pb2.Markdown

  @property
  def oid_cls(self) -> Type[datatypes_pb2.Oid]:
    return datatypes_pb2.Oid

  @property
  def positive_int_cls(self) -> Type[datatypes_pb2.PositiveInt]:
    return datatypes_pb2.PositiveInt

  @property
  def string_cls(self) -> Type[datatypes_pb2.String]:
    return datatypes_pb2.String

  @property
  def time_cls(self) -> Type[datatypes_pb2.Time]:
    return datatypes_pb2.Time

  @property
  def unsigned_int_cls(self) -> Type[datatypes_pb2.UnsignedInt]:
    return datatypes_pb2.UnsignedInt

  @property
  def uri_cls(self) -> Type[datatypes_pb2.Uri]:
    return datatypes_pb2.Uri

  @property
  def url_cls(self) -> Type[datatypes_pb2.Url]:
    return datatypes_pb2.Url

  @property
  def uuid_cls(self) -> Type[datatypes_pb2.Uuid]:
    return datatypes_pb2.Uuid

  @property
  def xhtml_cls(self) -> Type[datatypes_pb2.Xhtml]:
    return datatypes_pb2.Xhtml

  def get_primitive_wrapper_cls_for_primitive_cls(
      self, primitive_cls: Type[message.Message]
  ) -> Type[_primitive_wrappers.PrimitiveWrapper]:
    if fhir_types.is_type_or_profile_of_code(primitive_cls.DESCRIPTOR):
      return _primitive_wrappers.CodeWrapper

    primitive_wrapper_cls = self._DESC_TO_PRIMITIVE_WRAPPER_TYPE.get(
        primitive_cls.DESCRIPTOR.full_name)
    if primitive_wrapper_cls is None:
      raise ValueError(
          'Unexpected R4 FHIR primitive: '
          f'{primitive_cls.DESCRIPTOR.full_name!r} for handler: {type(self)}.')
    return primitive_wrapper_cls

  def primitive_wrapper_from_primitive(
      self,
      primitive_message: message.Message,
  ) -> _primitive_wrappers.PrimitiveWrapper:
    """See jsonformat PrimitiveHandler.primitive_wrapper_from_primitive."""
    if not annotation_utils.is_primitive_type(primitive_message):
      raise ValueError(
          f'Not a primitive type: {primitive_message.DESCRIPTOR.full_name!r}.')

    wrapper_cls = self.get_primitive_wrapper_cls_for_primitive_cls(
        type(primitive_message))
    wrapper_context = _primitive_wrappers.Context(
        separator_stride_cls=fhirproto_extensions_pb2
        .Base64BinarySeparatorStride,
        code_cls=self.code_cls,
        default_timezone=_primitive_time_utils.SIMPLE_ZULU)
    return wrapper_cls.from_primitive(primitive_message, wrapper_context)

  def primitive_wrapper_from_json_value(
      self,
      json_value: Optional[Any],
      primitive_cls: Type[message.Message],
      *,
      default_timezone: str = _primitive_time_utils.SIMPLE_ZULU
  ) -> _primitive_wrappers.PrimitiveWrapper:
    """See jsonformat PrimitiveHandler.primitive_wrapper_from_json_value."""
    if not annotation_utils.is_primitive_type(primitive_cls.DESCRIPTOR):
      raise ValueError(
          f'Not a primitive type: {primitive_cls.DESCRIPTOR.full_name!r}.')

    wrapper_cls = self.get_primitive_wrapper_cls_for_primitive_cls(
        primitive_cls)
    wrapper_context = _primitive_wrappers.Context(
        separator_stride_cls=fhirproto_extensions_pb2
        .Base64BinarySeparatorStride,
        code_cls=self.code_cls,
        default_timezone=default_timezone)
    return wrapper_cls.from_json_value(json_value, primitive_cls,
                                       wrapper_context)
