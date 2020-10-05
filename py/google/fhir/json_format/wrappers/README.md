# FHIR Primitive Wrappers

Primitive wrappers act as stateful encoding/decoding functions to translate from
some HL7 FHIR primitive JSON to a structured protobuf representation (parsing)
and vice-versa (printing). The wrappers subpackage implements a suite of wrapper
abstract classes and concrete subclasses for parsing/printing any HL7 FHIR
primitive datatypes found in versions [STU3](https://www.hl7.org/fhir/STU3/datatypes.html#primitive)
or [R4](https://www.hl7.org/fhir/datatypes.html).

## Primitive Wrapper Mapping

Each concrete wrapper implementation adheres to the
`primitive_wrappers.PrimitiveWrapper` abstract base class. Each FHIR version-
specific target (e.g. `//py/google/fhir/jsonformat/stu3`,
`//py/google/fhir/jsonformat/r4`) provides a wrapper implementation for an
encountered FHIR primitive at runtime via the
`primitive_handler.PrimitiveHandler` class. The vast majority of FHIR primitives
are "wrapped" using a `StringWrapper` implementation. Those primitives requiring
extra logic for parsing/printing (e.g. custom functions/additional state
information) are separated into their own modules within `wrappers/` for
readability.

## Primitive Wrapper Context

Certain FHIR wrappers require additional stateful information at the time of
wrapping. This information is typically used to alter the appearance of a
printed primitive (e.g. the type of "separator" used in a base64 binary string),
to infer intent in the face of ambiguity (e.g. the "default timezone" when a
date/time primitive has no associated timezone information), or is simply
information specific to the version of FHIR being parsed/printed (e.g. the
"generic" `Code` protobuf to use). This extra information is passed to the
wrapper via an instance of `primitive_wrappers.Context` class, which is provided
by the FHIR version-specific target `primitive_handler.PrimitiveHandler`
implementation.

## Testing

Testing necessitates parsing/printing specific FHIR versions of primitive
protobufs. As such, each FHIR version-specific target (e.g.
`//py/google/fhir/jsonformat/stu3`, `//py/google/fhir/jsonformat/r4`) tests the
primitive wrapper implementations via the `primitive_handler_test.py` module by
providing a specific implementation of the `primitive_handler.PrimitiveHandler`
class.

## Example Usage

### Parsing

```
datetime_str = '1970-01-01'
expected = datatypes_pb2.DateTime(
  value_us=0,
  precision=datatypes_pb2.DateTime.Precision.DAY,
  timezone='Z')
wrapper = _date_time.DateTimeWrapper.from_json_value(datetime_str)
assert(expected == wrapper.wrapped) # True
```

### Printing

```
primitive = datatypes_pb2.DateTime(
  value_us=-36000000000,
  precision=datatypes_pb2.DateTime.Precision.MONTH,
  timezone='Australia/Sydney')
wrapper = _date_time.DateTimeWrapper.from_primitive(primitive)
assert(wrapper.string_value() == '1970-01') # True
```

## References

See the official [hl7.org FHIR specification](https://www.hl7.org/fhir/) for
more information.  Information surrounding version-specific primitive datatypes
can be found at the following links:

* [STU3](https://www.hl7.org/fhir/STU3/datatypes.html)
* [R4](https://www.hl7.org/fhir/datatypes.html)
