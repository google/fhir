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
"""Version-agnostic abstractions around FHIR primitive datatypes."""

import abc
from typing import cast, Any, List, Optional, Type

from google.protobuf import message
from google.fhir import _primitive_time_utils
from google.fhir.json_format.wrappers import _primitive_wrappers


class PrimitiveHandler(abc.ABC):
  """An abstract base class to return FHIR primitive wrappers."""

  @property
  @abc.abstractmethod
  def base64_binary_cls(self) -> Type[message.Message]:
    """A stream of bytes, base64 encoded (https://tools.ietf.org/html/rfc4648).

    See more at: https://www.hl7.org/fhir/datatypes.html#base64Binary.
    """
    raise NotImplementedError('Subclasses *must* implement base64_binary_cls.')

  @property
  @abc.abstractmethod
  def boolean_cls(self) -> Type[message.Message]:
    """true | false.

    See more at: https://www.hl7.org/fhir/datatypes.html#boolean.
    """
    raise NotImplementedError('Subclasses *must* implement boolean_cls.')

  @property
  def canonical_cls(self) -> Type[message.Message]:
    """A URI that refers to a resource by its canonical URL.

    See more at: https://www.hl7.org/fhir/datatypes.html#canonical.

    Returns:
      A FHIR R4 Canonical primitive class.
    """
    raise NotImplementedError('Subclasses *must* implement canonical_cls.')

  @property
  @abc.abstractmethod
  def code_cls(self) -> Type[message.Message]:
    """A value taken from a set of controlled strings defined elsewhere.

    See more at: https://www.hl7.org/fhir/datatypes.html#code.
    """
    raise NotImplementedError('Subclasses *must* implement code_cls.')

  # TODO: Find more appropriate location for contained_resource_cls
  @property
  @abc.abstractmethod
  def contained_resource_cls(self) -> Type[message.Message]:
    """A reference to a resource associated with some record.

    The contained resource is a reference to content that does not have an
    independent existence apart from the resource that contains it. It cannot
    be identified independently, nor can it have its own independent transaction
    scope.

    See more at: https://www.hl7.org/fhir/references.html#contained.
    """
    raise NotImplementedError(
        'Subclasses *must* implement contained_resource_cls.')

  @property
  @abc.abstractmethod
  def date_cls(self) -> Type[message.Message]:
    """A date, or partial date.

    See more at: https://www.hl7.org/fhir/datatypes.html#date.
    """
    raise NotImplementedError('Subclasses *must* implement date_cls.')

  @property
  @abc.abstractmethod
  def date_time_cls(self) -> Type[message.Message]:
    """A date, date-time, or partial date.

    See more at: https://www.hl7.org/fhir/datatypes.html#datetime.
    """
    raise NotImplementedError('Subclasses *must* implement date_time_cls.')

  @property
  @abc.abstractmethod
  def decimal_cls(self) -> Type[message.Message]:
    """Rational numbers that have a decimal representation.

    See more at: https://www.hl7.org/fhir/datatypes.html#decimal.
    """
    raise NotImplementedError('Subclasses *must* implement decimal_cls.')

  @property
  @abc.abstractmethod
  def id_cls(self) -> Type[message.Message]:
    """64 character alphanumeric identifier with '-' and '.' allowed.

    See more at: https://www.hl7.org/fhir/datatypes.html#id.
    """
    raise NotImplementedError('Subclasses *must* implement id_cls.')

  @property
  @abc.abstractmethod
  def instant_cls(self) -> Type[message.Message]:
    """An instant in time.

    See more at: https://www.hl7.org/fhir/datatypes.html#instant.
    """
    raise NotImplementedError('Subclasses *must* implement instant_cls.')

  @property
  @abc.abstractmethod
  def integer_cls(self) -> Type[message.Message]:
    """A signed 32-bit integer.

    See more at: https://www.hl7.org/fhir/datatypes.html#integer.
    """
    raise NotImplementedError('Subclasses *must* implement integer_cls.')

  @property
  @abc.abstractmethod
  def markdown_cls(self) -> Type[message.Message]:
    """A FHIR String that may contain markdown syntax for optional processing.

    See more at: https://www.hl7.org/fhir/datatypes.html#markdown.
    """
    raise NotImplementedError('Subclasses *must* implement markdown_cls.')

  @property
  @abc.abstractmethod
  def oid_cls(self) -> Type[message.Message]:
    """An OID represented as a URI (https://www.ietf.org/rfc/rfc3001.txt).

    See more at: https://www.hl7.org/fhir/datatypes.html#oid.
    """
    raise NotImplementedError('Subclasses *must* implement oid_cls.')

  @property
  @abc.abstractmethod
  def positive_int_cls(self) -> Type[message.Message]:
    """An unsigned 32-bit integer.

    See more at: https://www.hl7.org/fhir/datatypes.html#positiveInt.
    """
    raise NotImplementedError('Subclasses *must* implement positive_int_cls.')

  @property
  @abc.abstractmethod
  def string_cls(self) -> Type[message.Message]:
    """A sequence of Unicode characters.

    See more at: https://www.hl7.org/fhir/datatypes.html#string.
    """
    raise NotImplementedError('Subclasses *must* implement string_cls.')

  @property
  @abc.abstractmethod
  def time_cls(self) -> Type[message.Message]:
    """A time during the day.

    See more at: https://www.hl7.org/fhir/datatypes.html#time.
    """
    raise NotImplementedError('Subclasses *must* implement time_cls.')

  @property
  @abc.abstractmethod
  def unsigned_int_cls(self) -> Type[message.Message]:
    """An unsigned 32-bit integer.

    See more at: https://www.hl7.org/fhir/datatypes.html#unsignedInt.
    """
    raise NotImplementedError('Subclasses *must* implement unsigned_int_cls.')

  @property
  @abc.abstractmethod
  def uri_cls(self) -> Type[message.Message]:
    """A Uniform Resource Identifier (https://tools.ietf.org/html/rfc3986).

    See more at: https://www.hl7.org/fhir/datatypes.html#uri.
    """
    raise NotImplementedError('Subclasses *must* implement uri_cls.')

  @property
  @abc.abstractmethod
  def url_cls(self) -> Type[message.Message]:
    """A Uniform Resource Locator (https://tools.ietf.org/html/rfc1738).

    See more at: https://www.hl7.org/fhir/datatypes.html#url.

    Returns:
      A FHIR R4 URL primitive class.
    """
    raise NotImplementedError('Subclasses *must* implement url_cls.')

  @property
  @abc.abstractmethod
  def uuid_cls(self) -> Type[message.Message]:
    """A UUID represented as a URI.

    See more at: https://www.hl7.org/fhir/datatypes.html#uuid.

    Returns:
      A FHIR R4 UUID primitive class.
    """
    raise NotImplementedError('Subclasses *must* implement uuid_cls.')

  @property
  @abc.abstractmethod
  def xhtml_cls(self) -> Type[message.Message]:
    """XHTML content."""
    raise NotImplementedError('Subclasses *must* implement xhtml_cls.')

  @abc.abstractmethod
  def get_primitive_wrapper_cls_for_primitive_cls(
      self, primitive_cls: Type[message.Message]
  ) -> Type[_primitive_wrappers.PrimitiveWrapper]:
    """Returns the primitive wrapper for the corresponding primitive_cls.

    Subclasses should override and provide logic to return their own wrappers
    for primitives specific to a particular version of FHIR. The base
    implementation provides handling for all primitives included in FHIR version
    >= stu3.

    Args:
      primitive_cls: The type of primitive for which to return a wrapper.

    Returns:
      A PrimitiveWrapper for parsing/printing types of primitive_cls.
    """
    raise NotImplementedError('Subclasses *must* implement '
                              'get_primitive_wrapper_cls_for_primitive_cls.')

  def new_base64_binary(
      self,
      value: bytes = b'',
      id_: Optional[message.Message] = None,
      extension: Optional[List[message.Message]] = None) -> message.Message:
    return cast(Any, self.base64_binary_cls)(
        value=value, id=id_, extension=extension)

  def new_boolean(
      self,
      value: bool = False,
      id_: Optional[message.Message] = None,
      extension: Optional[List[message.Message]] = None) -> message.Message:
    return cast(Any, self.boolean_cls)(value=value, id=id_, extension=extension)

  def new_canonical(
      self,
      value: str = '',
      id_: Optional[message.Message] = None,
      extension: Optional[List[message.Message]] = None) -> message.Message:
    return cast(Any, self.canonical_cls)(
        value=value, id=id_, extension=extension)

  def new_code(
      self,
      value: str = '',
      id_: Optional[message.Message] = None,
      extension: Optional[List[message.Message]] = None) -> message.Message:
    return cast(Any, self.code_cls)(value=value, id=id_, extension=extension)

  def new_contained_resource(self) -> message.Message:
    return self.contained_resource_cls()

  def new_date(
      self,
      value_us: int = 0,
      timezone: str = '',
      precision: int = 0,
      id_: Optional[message.Message] = None,
      extension: Optional[List[message.Message]] = None) -> message.Message:
    return cast(Any, self.date_cls)(
        value_us=value_us,
        timezone=timezone,
        precision=precision,
        id=id_,
        extension=extension)

  def new_date_time(
      self,
      value_us: int = 0,
      timezone: str = '',
      precision: int = 0,
      id_: Optional[message.Message] = None,
      extension: Optional[List[message.Message]] = None) -> message.Message:
    return cast(Any, self.date_time_cls)(
        value_us=value_us,
        timezone=timezone,
        precision=precision,
        id=id_,
        extension=extension)

  def new_decimal(
      self,
      value: str = '',
      id_: Optional[message.Message] = None,
      extension: Optional[List[message.Message]] = None) -> message.Message:
    return cast(Any, self.decimal_cls)(value=value, id=id_, extension=extension)

  def new_id(
      self,
      value: str = '',
      id_: Optional[message.Message] = None,
      extension: Optional[List[message.Message]] = None) -> message.Message:
    return cast(Any, self.id_cls)(value=value, id=id_, extension=extension)

  def new_instant(
      self,
      value_us: int = 0,
      timezone: str = '',
      precision: int = 0,
      id_: Optional[message.Message] = None,
      extension: Optional[List[message.Message]] = None) -> message.Message:
    return cast(Any, self.instant_cls)(
        value_us=value_us,
        timezone=timezone,
        precision=precision,
        id=id_,
        extension=extension)

  def new_integer(
      self,
      value: int = 0,
      id_: Optional[message.Message] = None,
      extension: Optional[List[message.Message]] = None) -> message.Message:
    return cast(Any, self.integer_cls)(value=value, id=id_, extension=extension)

  def new_markdown(
      self,
      value: str = '',
      id_: Optional[message.Message] = None,
      extension: Optional[List[message.Message]] = None) -> message.Message:
    return cast(Any, self.markdown_cls)(
        value=value, id=id_, extension=extension)

  def new_oid(
      self,
      value: str = '',
      id_: Optional[message.Message] = None,
      extension: Optional[List[message.Message]] = None) -> message.Message:
    return cast(Any, self.oid_cls)(value=value, id=id_, extension=extension)

  def new_positive_int(
      self,
      value: int = 0,
      id_: Optional[message.Message] = None,
      extension: Optional[List[message.Message]] = None) -> message.Message:
    return cast(Any, self.positive_int_cls)(
        value=value, id=id_, extension=extension)

  def new_string(
      self,
      value: str = '',
      id_: Optional[message.Message] = None,
      extension: Optional[List[message.Message]] = None) -> message.Message:
    return cast(Any, self.string_cls)(value=value, id=id_, extension=extension)

  def new_time(
      self,
      value_us: int = 0,
      precision: int = 0,
      id_: Optional[message.Message] = None,
      extension: Optional[List[message.Message]] = None) -> message.Message:
    return cast(Any, self.time_cls)(
        value_us=value_us, precision=precision, id=id_, extension=extension)

  def new_unsigned_int(
      self,
      value: int = 0,
      id_: Optional[message.Message] = None,
      extension: Optional[List[message.Message]] = None) -> message.Message:
    return cast(Any, self.unsigned_int_cls)(
        value=value, id=id_, extension=extension)

  def new_uri(
      self,
      value: str = '',
      id_: Optional[message.Message] = None,
      extension: Optional[List[message.Message]] = None) -> message.Message:
    return cast(Any, self.uri_cls)(value=value, id=id_, extension=extension)

  def new_url(
      self,
      value: str = '',
      id_: Optional[message.Message] = None,
      extension: Optional[List[message.Message]] = None) -> message.Message:
    return cast(Any, self.url_cls)(value=value, id=id_, extension=extension)

  def new_uuid(
      self,
      value: str = '',
      id_: Optional[message.Message] = None,
      extension: Optional[List[message.Message]] = None) -> message.Message:
    return cast(Any, self.uuid_cls)(value=value, id=id_, extension=extension)

  def new_xhtml(
      self,
      value: str = '',
      id_: Optional[message.Message] = None,
  ) -> message.Message:
    return cast(Any, self.xhtml_cls)(value=value, id=id_)

  @abc.abstractmethod
  def primitive_wrapper_from_primitive(
      self, primitive_message: message.Message
  ) -> _primitive_wrappers.PrimitiveWrapper:
    """Wraps the FHIR protobuf primitive_message to handle parsing/printing.

    The wrapped FHIR protobuf primitive provides necessary state for printing
    to the FHIR JSON spec.

    Args:
      primitive_message: The FHIR primitive to wrap.

    Raises:
      ValueError: In the event that primitive_message is not actually a
      primitive FHIR type.

    Returns: A wrapper around primitive_message.
    """
    raise NotImplementedError(
        'Subclasses *must* implement primitive_wrapper_from_primitive.')

  @abc.abstractmethod
  def primitive_wrapper_from_json_value(
      self,
      json_value: Optional[Any],
      primitive_cls: Type[message.Message],
      *,
      default_timezone: str = _primitive_time_utils.SIMPLE_ZULU
  ) -> _primitive_wrappers.PrimitiveWrapper:
    """Parses json_value into a FHIR protobuf primitive and wraps.

    The wrapper provides necessary information on how to parse json_value into
    a corresponding FHIR protobuf message. Afterwards, this is wrapped to
    provide stateful information to the parent parser and/or printer.

    Args:
      json_value: The FHIR json value to parse and wrap.
      primitive_cls: The type of FHIR primitive to parse json_value into.
      default_timezone: The default timezone string to use when parsing date/
        time-like primitives when there is no timezone information available.
        Defaults to 'Z'.

    Raises:
      ValueError: In the event that primitive_cls is not actually a primitive
      FHIR type.

    Returns:
      A wrapper around an instance of primitive_cls parsed from json_value.
    """
    raise NotImplementedError(
        'Subclasses *must* implement primitive_wrapper_from_json_value.')
