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
"""Functionality for printing FHIR protobuf representations into FHIR JSON."""

import abc
import copy
import enum
from typing import cast, Any, Callable, List, Optional

from google.protobuf import any_pb2
from google.protobuf import descriptor
from google.protobuf import message
from google.fhir import primitive_handler
from google.fhir import references
from google.fhir.utils import annotation_utils
from google.fhir.utils import fhir_types
from google.fhir.utils import proto_utils


class _FhirJsonFormat(enum.Enum):
  """The format of FHIR JSON to print.

  Cases:
    * PURE: Lossless JSON representation of the FHIR proto.
    * ANALYTIC: for use in SQL-like analytics.  See:
      https://github.com/FHIR/sql-on-fhir/blob/master/sql-on-fhir.md.
  """
  PURE = 1
  ANALYTIC = 2


class _JsonTextGenerator(abc.ABC):
  """An abstract base class defining how to generate JSON text."""

  def __init__(self):
    self._output = []

  @abc.abstractmethod
  def indent(self):
    raise NotImplementedError('Subclasses *must* implement indent().')

  @abc.abstractmethod
  def outdent(self):
    raise NotImplementedError('Subclasses *must* implement outdent().')

  @abc.abstractmethod
  def add_newline(self):
    raise NotImplementedError('Subclasses *must* implement add_newline().')

  def push(self, value: str):
    self._output.append(value)

  def add_field(self, field_name: str, value: Optional[str] = None):
    self.push(f'"{field_name}": ')
    if value is not None:
      self.push(value)

  def open_json_object(self):
    self.push('{')
    self.indent()
    self.add_newline()

  def close_json_object(self):
    self.outdent()
    self.add_newline()
    self.push('}')

  def open_json_list(self):
    self.push('[')
    self.indent()
    self.add_newline()

  def close_json_list(self):
    self.outdent()
    self.add_newline()
    self.push(']')

  def dump(self) -> str:
    """Returns the contents of the output as a string."""
    return ''.join(self._output)

  def clear(self):
    """Resets the state of the receiver."""
    self._output.clear()


class _PrettyJsonTextGenerator(_JsonTextGenerator):
  """A JSON text generator with a space delimiter indent and newlines."""

  def __init__(self, indent_size: int):
    super().__init__()
    self._indent_size = indent_size
    self._current_indent = 0

  def indent(self):
    self._current_indent += self._indent_size

  def outdent(self):
    self._current_indent -= self._indent_size

  def add_newline(self):
    self.push('\n')
    self.push(' ' * self._current_indent)

  def clear(self):
    super().clear()
    self._current_indent = 0


class _CompactJsonTextGenerator(_JsonTextGenerator):
  """A JSON text generator with no indentation or newlines."""

  def indent(self):
    pass  # No-op

  def outdent(self):
    pass  # No-op

  def add_newline(self):
    pass  # No-op


class JsonPrinter:
  """Prints a FHIR JSON representation of a provided FHIR proto."""

  @classmethod
  def pretty_printer(cls,
                     primitive_handler_: primitive_handler.PrimitiveHandler,
                     indent_size: int):
    """Returns a printer for FHIR JSON with spaces and newlines.

    Args:
      primitive_handler_: Responsible for returning PrimitiveWrappers.
      indent_size: The size of space indentation for lexical scoping.
    """
    return cls(primitive_handler_, _PrettyJsonTextGenerator(indent_size),
               _FhirJsonFormat.PURE)

  @classmethod
  def compact_printer(cls,
                      primitive_handler_: primitive_handler.PrimitiveHandler):
    """Returns a printer for FHIR JSON with no spaces or newlines."""
    return cls(primitive_handler_, _CompactJsonTextGenerator(),
               _FhirJsonFormat.PURE)

  @classmethod
  def pretty_printer_for_analytics(
      cls, primitive_handler_: primitive_handler.PrimitiveHandler,
      indent_size: int):
    """Returns a printer for Analytic FHIR JSON with spaces and newlines.

    Args:
      primitive_handler_: Responsible for returning PrimitiveWrappers.
      indent_size: The size of space indentation for lexical scoping.
    """
    return cls(primitive_handler_, _PrettyJsonTextGenerator(indent_size),
               _FhirJsonFormat.ANALYTIC)

  @classmethod
  def compact_printer_for_analytics(
      cls, primitive_handler_: primitive_handler.PrimitiveHandler):
    """Returns a printer for Analytic FHIR JSON with no spaces or newlines."""
    return cls(primitive_handler_, _CompactJsonTextGenerator(),
               _FhirJsonFormat.ANALYTIC)

  def __init__(self, primitive_handler_: primitive_handler.PrimitiveHandler,
               generator: _JsonTextGenerator, json_format: _FhirJsonFormat):
    """Creates a new instance of JsonPrinter.

    Note that this is for *internal-use* only. External clients should leverage
    one of the available class constructors such as:
    `JsonPrinter.pretty_printer(...)`, `JsonPrinter.compact_printer()`, etc.

    Args:
      primitive_handler_: Responsible for returning PrimitiveWrappers.
      generator: The type of _JsonTextGenerator used to handle whitespace and
        newline additions.
      json_format: The style of FHIR JSON to output.
    """
    self.primitive_handler = primitive_handler_
    self.generator = generator
    self.json_format = json_format

  def _print_list(self, values: List[Any], print_func: Callable[[Any], None]):
    """Adds the printed JSON list representation of values to _output.

    Args:
      values: The values to print as a JSON list.
      print_func: A function responsible for printing a single value.
    """
    self.generator.open_json_list()

    field_size = len(values)
    for i in range(field_size):
      print_func(values[i])
      if i < (field_size - 1):
        self.generator.push(',')
        self.generator.add_newline()

    self.generator.close_json_list()

  def _print_choice_field(self, field_name: str,
                          field: descriptor.FieldDescriptor,
                          choice_container: message.Message):
    """Prints a FHIR choice field.

    This field is expected to have one valid oneof set.

    Args:
      field_name: The name of the field.
      field: The FieldDescriptor whose contents to print.
      choice_container: The value present at field, which should be a oneof with
        a single value set.
    """
    if len(choice_container.DESCRIPTOR.oneofs) != 1:
      raise ValueError(f'Invalid value for choice field {field_name}: '
                       f'{choice_container}.')
    oneof_group = choice_container.DESCRIPTOR.oneofs[0]
    set_oneof_name = choice_container.WhichOneof(oneof_group.name)
    if set_oneof_name is None:
      raise ValueError('Oneof not set on choice type: '
                       f'{choice_container.DESCRIPTOR.full_name}.')
    value_field = choice_container.DESCRIPTOR.fields_by_name[set_oneof_name]
    oneof_field_name = value_field.json_name
    oneof_field_name = oneof_field_name[0].upper() + oneof_field_name[1:]

    value = proto_utils.get_value_at_field(choice_container, value_field)
    if annotation_utils.is_primitive_type(value_field.message_type):
      self._print_primitive_field(field_name + oneof_field_name, value_field,
                                  value)
    else:
      self._print_message_field(field_name + oneof_field_name, value_field,
                                value)

  def _print_primitive_field(self, field_name: str,
                             field: descriptor.FieldDescriptor, value: Any):
    """Prints the primitive field.

    Args:
      field_name: The name of the field.
      field: The FielDescriptor whose contents to print.
      value: The value present at field to print.
    """
    if proto_utils.field_is_repeated(field):
      string_values = []
      elements = []
      extensions_found = False
      nonnull_values_found = False
      for primitive in value:
        wrapper = self.primitive_handler.primitive_wrapper_from_primitive(
            primitive)
        string_values.append(wrapper.json_value())
        elements.append(wrapper.get_element())
        nonnull_values_found = nonnull_values_found or wrapper.has_value()
        extensions_found = extensions_found or wrapper.has_element()

      # print string primitive representations
      if nonnull_values_found:
        self.generator.add_field(field_name)
        self._print_list(string_values, self.generator.push)

      # print Element representations
      if extensions_found:
        if nonnull_values_found:
          self.generator.push(',')
          self.generator.add_newline()
        self.generator.add_field(f'_{field_name}')
        self._print_list(elements, self._print)
    else:  # Singular field
      # TODO: Detect ReferenceId using an annotation
      if (self.json_format == _FhirJsonFormat.ANALYTIC and
          field.message_type.name == 'ReferenceId'):
        str_value = proto_utils.get_value_at_field(value, 'value')
        self.generator.add_field(field_name, f'"{str_value}"')
      else:  # Wrap and print primitive value and (optionally), its element
        wrapper = self.primitive_handler.primitive_wrapper_from_primitive(value)
        if wrapper.has_value():
          self.generator.add_field(field_name, wrapper.json_value())
        if wrapper.has_element() and self.json_format == _FhirJsonFormat.PURE:
          if wrapper.has_value():
            self.generator.push(',')
            self.generator.add_newline()
          self.generator.add_field(f'_{field_name}')
          self._print(wrapper.get_element())

  def _print_message_field(self, field_name: str,
                           field: descriptor.FieldDescriptor, value: Any):
    """Prints singular and repeated fields from a message."""
    self.generator.add_field(field_name)
    if proto_utils.field_is_repeated(field):
      self._print_list(cast(List[Any], value), self._print)
    else:
      self._print(cast(message.Message, value))

  def _print_extension(self, extension: message.Message):
    """Pushes the Extension into the JSON text generator.

    If the _FhirJsonFormat is set to ANALYTIC, this method only prints the url.

    Args:
      extension: The Extension to print.
    """
    if not fhir_types.is_type_or_profile_of_extension(extension):
      raise ValueError(f'Message of type: {extension.DESCRIPTOR.full_name} is '
                       'not a FHIR Extension.')
    if self.json_format == _FhirJsonFormat.ANALYTIC:
      self.generator.push(f'"{cast(Any, extension).url.value}"')
    else:
      self._print_message(extension)

  def _print_contained_resource(self, contained_resource: message.Message):
    """Prints the set fields of the contained resource.

    If the _FhirJsonFormat is set to ANALYTIC, this method only prints the url.

    Args:
      contained_resource: The contained resource to iterate over and print.
    """
    for (_, set_field_value) in contained_resource.ListFields():
      if self.json_format == _FhirJsonFormat.ANALYTIC:
        structure_definition_url = (
            annotation_utils.get_structure_definition_url(set_field_value))
        self.generator.push(f'"{structure_definition_url}"')
      else:  # print the entire contained resource...
        self._print(set_field_value)

  def _print_reference(self, reference: message.Message):
    """Standardizes and prints the provided reference.

    Note that "standardization" in the case of PURE FHIR JSON refers to
    un-typing the typed-reference prior to printing.

    Args:
      reference: The reference to print.
    """
    set_oneof = reference.WhichOneof('reference')
    if (self.json_format == _FhirJsonFormat.PURE and set_oneof is not None and
        set_oneof != 'uri'):
      # In pure FHIR mode, we have to serialize structured references
      # into FHIR uri strings.
      standardized_reference = copy.copy(reference)

      # Setting the new URI field will overwrite the original oneof
      new_uri = proto_utils.get_value_at_field(standardized_reference, 'uri')
      proto_utils.set_value_at_field(new_uri, 'value',
                                     references.reference_to_string(reference))
      self._print_message(standardized_reference)
    else:
      self._print_message(reference)

  def _print_message(self, msg: message.Message):
    """Prints the representation of message."""
    self.generator.open_json_object()

    # Add the resource type preamble if necessary
    set_fields = msg.ListFields()
    if (annotation_utils.is_resource(msg) and
        self.json_format == _FhirJsonFormat.PURE):
      self.generator.add_field('resourceType', f'"{msg.DESCRIPTOR.name}"')
      if set_fields:
        self.generator.push(',')
        self.generator.add_newline()

    # Print all fields
    for (i, (set_field, value)) in enumerate(set_fields):
      if (annotation_utils.is_choice_type_field(set_field) and
          self.json_format == _FhirJsonFormat.PURE):
        self._print_choice_field(set_field.json_name, set_field, value)
      elif annotation_utils.is_primitive_type(set_field.message_type):
        self._print_primitive_field(set_field.json_name, set_field, value)
      else:
        self._print_message_field(set_field.json_name, set_field, value)

      if i < (len(set_fields) - 1):
        self.generator.push(',')
        self.generator.add_newline()

    self.generator.close_json_object()

  def _print(self, msg: message.Message):
    """Prints the JSON representation of message to the underlying generator."""
    # TODO: Identify ContainedResource with an annotation
    if msg.DESCRIPTOR.name == 'ContainedResource':
      self._print_contained_resource(msg)
    elif msg.DESCRIPTOR.full_name == any_pb2.Any.DESCRIPTOR.full_name:
      contained_resource = self.primitive_handler.new_contained_resource()
      if not cast(any_pb2.Any, msg).Unpack(contained_resource):
        # If we can't unpack the Any, drop it.
        # TODO: Use a registry to determine the correct
        # ContainedResource to unpack to
        return
      self._print_contained_resource(contained_resource)
    elif fhir_types.is_extension(msg):
      self._print_extension(msg)
    elif annotation_utils.is_reference(msg):
      self._print_reference(msg)
    else:
      self._print_message(msg)

  def print(self, msg: message.Message) -> str:
    """Returns the contents of message as a FHIR JSON string."""
    self.generator.clear()

    # If provided a primitive message, simply wrap and return the JSON value
    if annotation_utils.is_primitive_type(msg):
      wrapper = self.primitive_handler.primitive_wrapper_from_primitive(msg)
      return wrapper.json_value()

    self._print(msg)
    return self.generator.dump()
