// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Package jsonpbhelper provides version agnostic utility functions for FHIR proto conversion.
package jsonpbhelper

import (
	"encoding/json"
	"errors"
	"fmt"
	"math"
	"regexp"
	"strconv"
	"strings"
	"sync"
	"time"
	"unicode/utf8"

	"log"
	"github.com/json-iterator/go"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/reflect/protoreflect"
	"bitbucket.org/creachadair/stringset"

	apb "github.com/google/fhir/go/proto/google/fhir/proto/annotations_go_proto"

	d4pb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/datatypes_go_proto"
	r4pb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/bundle_and_contained_resource_go_proto"
	d3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/datatypes_go_proto"
	r3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/resources_go_proto"
)

const (
	// SecondToMicro records the conversion between second and microsecond.
	SecondToMicro = int64(time.Second) / int64(time.Microsecond)
	// MicroToNano records the conversion between microsecond and nanosecond.
	MicroToNano = int64(time.Microsecond)
	// UTC represents the UTC timezone string.
	UTC = "Z"
	// DefaultAnalyticsRecurExpansionDepth indicates the default max depth for recursive expansion
	// in marshalling to analytics schema.
	DefaultAnalyticsRecurExpansionDepth = 2

	// LayoutYear for year layout.
	LayoutYear = "2006"
	// LayoutMonth for month layout.
	LayoutMonth = "2006-01"
	// LayoutDay for day layout.
	LayoutDay = "2006-01-02"
	// LayoutMinutesNoTZ for minute layout without timezone.
	LayoutMinutesNoTZ = "2006-01-02T15:04"
	// LayoutSecondsNoTZ for second layout without timezone.
	LayoutSecondsNoTZ = "2006-01-02T15:04:05"
	// LayoutMinutes for minute layout.
	LayoutMinutes = "2006-01-02T15:04-07:00"
	// LayoutSeconds for second layout.
	LayoutSeconds = "2006-01-02T15:04:05-07:00"
	// LayoutMillis for millisecond layout.
	LayoutMillis = "2006-01-02T15:04:05.000-07:00"
	// LayoutMicros for microsecond layout.
	LayoutMicros = "2006-01-02T15:04:05.000000-07:00"
	// LayoutMinutesUTC for minute layout plus UTC.
	LayoutMinutesUTC = "2006-01-02T15:04Z"
	// LayoutSecondsUTC for second layout plus UTC.
	LayoutSecondsUTC = "2006-01-02T15:04:05Z"
	// LayoutMillisUTC for millisecond layout plus UTC.
	LayoutMillisUTC = "2006-01-02T15:04:05.000Z"
	// LayoutMicrosUTC for microsecond layout plus UTC.
	LayoutMicrosUTC = "2006-01-02T15:04:05.000000Z"
	// LayoutTimeSecond for time layout.
	LayoutTimeSecond = "15:04:05"
	// LayoutTimeMilliSecond for millisecond time layout.
	LayoutTimeMilliSecond = "15:04:05.000"
	// LayoutTimeMicroSecond for microsecond time layout.
	LayoutTimeMicroSecond = "15:04:05.000000"
	// MidnightTimeStr for midnight time string.
	MidnightTimeStr = "00:00:00"

	// RefOneofName for oneof reference name.
	RefOneofName = "reference"
	// RefFieldSuffix for reference field suffix.
	RefFieldSuffix = "_id"
	// RefFragment for reference fragment.
	RefFragment = "fragment"
	// RefFragmentPrefix for reference fragment prefix.
	RefFragmentPrefix = "#"
	// RefHistory for reference history.
	RefHistory = "_history"
	// RefRawURI for reference raw URI.
	RefRawURI = "uri"
	// RefAnyResource is the reference ID type name that fits any FHIR resources.
	RefAnyResource = "Resource"

	// The canonical structure definition URL for extension.
	extStructDefURL = "http://hl7.org/fhir/StructureDefinition/Extension"

	// Base64BinarySeparatorStrideURL is the canonical structure definition URL
	// for internal extension Base64BinarySeparatorStride.
	Base64BinarySeparatorStrideURL = "https://g.co/fhir/StructureDefinition/base64Binary-separatorStride"
	// PrimitiveHasNoValueURL is the canonical structure definition URL
	// for internal extension PrimitiveHasNoValue.
	PrimitiveHasNoValueURL = "https://g.co/fhir/StructureDefinition/primitiveHasNoValue"

	// FHIR spec limits strings to 1 MB.
	maxStringSize = 1024 * 1024
)

var (

	// SearchDateCompiledRegex for date regex used within searches only. Does
	// not have the correct precision for Date, DateTime, Time or Instant types.
	SearchDateCompiledRegex *regexp.Regexp
	// DateCompiledRegex for date regex.
	DateCompiledRegex *regexp.Regexp
	// DateTimeCompiledRegex for datetime regex.
	DateTimeCompiledRegex *regexp.Regexp
	// TimeCompiledRegex for time regex.
	TimeCompiledRegex *regexp.Regexp
	// InstantCompiledRegex for instant regex.
	InstantCompiledRegex *regexp.Regexp
	// PositiveIntCompiledRegex for positive integer regex.
	PositiveIntCompiledRegex *regexp.Regexp
	// UnsignedIntCompiledRegex for unsigned integer regex.
	UnsignedIntCompiledRegex *regexp.Regexp
	// JSP for JSP regex.
	JSP jsoniter.API

	// Regex for determining if datetimes are sub millisecond-level precision.
	subMilliDateCompiledRegex *regexp.Regexp
	// Regex for determining if times are sub millisecond-level precision.
	subMilliTimeCompiledRegex *regexp.Regexp
	// Regex for determining if datetimes are sub second-level precision.
	subSecondDateCompiledRegex *regexp.Regexp
	// Regex for determining if times are sub second-level precision.
	subSecondTimeCompiledRegex *regexp.Regexp

	// BigQuery column name must contain only letters, numbers, or underscores.
	invalidBQChar = regexp.MustCompile("[^a-zA-Z0-9_]+")

	// BigQuery column name must start with a letter or underscore.
	invalidBQStart = regexp.MustCompile("^[^a-zA-Z_]")

	// FHIR strings cannot contain code points below 32 except tab (\x09),
	// newline (\x0A), and carriage return (\x0D).
	invalidStringChars = regexp.MustCompile("[\x00-\x08\x0B\x0C\x0E-\x1F]")

	// requiredFields stores the proto message full names and the field numbers
	// of their required fields. This map is supposed to be populated during
	// initialization (i.e.: func init()), once initialization is done, it should
	// not be modified anymore.
	requiredFields map[protoreflect.FullName][]protoreflect.FieldNumber

	referenceFieldToType = map[protoreflect.Name]string{}
	referenceTypeToField = map[string]protoreflect.Name{}

	messageFieldsMutex = sync.RWMutex{}
	messageFields      = map[protoreflect.MessageDescriptor]map[string]protoreflect.FieldDescriptor{}

	// RegexValues stores the proto message full names and the regex validation
	// for its value fields. This map is supposed to be populated during
	// initialization (i.e.: func init()), once initialization is done, it should
	// not be modified anymore.
	RegexValues map[protoreflect.FullName]*regexp.Regexp
)

// Precision is used to indicate the precision of the ValueUs field of a Time.
type Precision int32

const (
	// PrecisionUnspecified indicates that the precision of the ValueUs field in a
	// Time is unknown.
	PrecisionUnspecified Precision = 0
	// PrecisionSecond indicates that the precision of the ValueUs field in a Time
	// is to the second.
	PrecisionSecond Precision = 1
	// PrecisionMillisecond indicates that the precision of the ValueUs field in a
	// Time is to the millisecond.
	PrecisionMillisecond Precision = 2
	// PrecisionMicrosecond indicates that the precision of the ValueUs field in a
	// Time is to the microsecond.
	PrecisionMicrosecond Precision = 3
)

// Time contains the result of a parsed FHIR time. The fields correspond to the
// version-specific fields of a DateTime proto.
type Time struct {
	Precision Precision
	ValueUs   int64
}

// ErrorType is the type of validation error.
type ErrorType string

const (
	// ReferenceTypeError is the error occurred during reference type validation
	ReferenceTypeError = ErrorType("ReferenceTypeError")
	// RequiredFieldError is the error occurred during required field validation
	RequiredFieldError = ErrorType("RequiredFieldError")
	// ParsingError is the error occurred during json parsing
	ParsingError = ErrorType("ParsingError")
)

// ErrorSeverity represents different UnmarshalError severity levels.
type ErrorSeverity string

// Values for IssueSeverity.
const (
	ErrorSeverityInformation = ErrorSeverity("informational")
	ErrorSeverityWarning     = ErrorSeverity("warning")
	ErrorSeverityError       = ErrorSeverity("error")
)

// UnmarshalError is a public error message for an error that occurred during unmarshaling.
// This type allows us to return detailed error information without exposing user data.
type UnmarshalError struct {
	// Path is the location where the error occurred.
	Path string
	// Details is a high level message about what the error was. This value should
	// not come from other libraries to ensure no PHI is reported.
	Details string
	// Diagnostics contains additional debugging information that will be appended to the end of
	// `Details`. This may include PHI and should not be reported where PHI is prohibited. For
	// example, a response is fine, but logs are not.
	Diagnostics string
	// Type is the type of the error occurred during validation.
	Type ErrorType
	// Severity represents different UnmarshalError severity levels.
	Severity ErrorSeverity
}

func (e *UnmarshalError) Error() string {
	var msg string
	if e.Path != "" {
		msg = fmt.Sprintf("error at %q: ", e.Path)
	}
	return msg + e.Details
}

// UnmarshalErrorList is a list of UnmarshalError that implements the Error
// interface itself.
type UnmarshalErrorList []*UnmarshalError

func (el UnmarshalErrorList) Error() string {
	var msgs []string
	for _, e := range el {
		msgs = append(msgs, e.Error())
	}
	return strings.Join(msgs, "\n")
}

// IsUnmarshalError returns true if the provided error is an UnmarshalError or
// UnmarshalErrorList.
func IsUnmarshalError(err error) bool {
	var umErr *UnmarshalError
	var el UnmarshalErrorList
	return errors.As(err, &umErr) || errors.As(err, &el)
}

// AnnotateUnmarshalErrorWithPath to help the user in debugging what field
// caused the error.
func AnnotateUnmarshalErrorWithPath(err error, jsonPath string) error {
	switch umErr := err.(type) {
	case *UnmarshalError:
		umErr.Path = jsonPath
	case UnmarshalErrorList:
		for _, err := range umErr {
			err.Path = jsonPath
		}
	}
	return err
}

// AnnotateUnmarshalErrorWithSeverity to help the user distinguish different
// error severity levels.
func AnnotateUnmarshalErrorWithSeverity(err error, severity ErrorSeverity) error {
	switch umErr := err.(type) {
	case *UnmarshalError:
		umErr.Severity = severity
	case UnmarshalErrorList:
		for _, err := range umErr {
			err.Severity = severity
		}
	}
	return err
}

// AppendUnmarshalError to the current error list, or ignore it if another type
// of fatal error has occurred.
func AppendUnmarshalError(el *UnmarshalErrorList, newErr error) error {
	switch umErr := newErr.(type) {
	case *UnmarshalError:
		*el = append(*el, umErr)
	case UnmarshalErrorList:
		*el = append(*el, umErr...)
	default:
		return newErr
	}
	return nil
}

func unmarshalErrorString(umErr *UnmarshalError) string {
	details := umErr.Details
	if umErr.Diagnostics != "" {
		// Strip invalid UTF-8 from the details to make sure we can marshal the error.
		// The eventual OperationOutcome has to be valid UTF-8.
		details += ": " + strings.ToValidUTF8(umErr.Diagnostics, "")
	}
	if umErr.Path != "" {
		details = fmt.Sprintf("at %s: %s", umErr.Path, details)
	}
	return details
}

// PrintUnmarshalError to a string. May contain user data. limit controls the
// number of error messages that will be returned if err is an
// UnmarshalErrorList. Use -1 for no limit. If the error is not an
// UnmarshalError then Error() will be called.
func PrintUnmarshalError(err error, limit int) string {
	switch umErr := err.(type) {
	case *UnmarshalError:
		return unmarshalErrorString(umErr)
	case UnmarshalErrorList:
		var msgs []string
		for i, umSubErr := range umErr {
			if limit >= 0 && i >= limit {
				msgs = append(msgs, fmt.Sprintf("and %d other issue(s)", len(umErr)-limit))
				break
			}
			msgs = append(msgs, unmarshalErrorString(umSubErr))
		}
		return strings.Join(msgs, "\n")
	default:
		return err.Error()
	}
}

func init() {
	compileOrDie := func(expr string) *regexp.Regexp {
		r, err := regexp.Compile(expr)
		if err != nil {
			log.Fatalf("Failed to compile regex '%v': %v", expr, err)
		}
		return r
	}
	subMilliRegex := "([01][0-9]|2[0-3]):[0-5][0-9]:[0-5][0-9]\\.[0-9]{3}[0-9]+"
	subMilliDateCompiledRegex = compileOrDie("T" + subMilliRegex)
	subMilliTimeCompiledRegex = compileOrDie(subMilliRegex)
	subSecondRegex := "([01][0-9]|2[0-3]):[0-5][0-9]:[0-5][0-9]\\.[0-9]+"
	subSecondDateCompiledRegex = compileOrDie("T" + subSecondRegex)
	subSecondTimeCompiledRegex = compileOrDie(subSecondRegex)
	SearchDateCompiledRegex = compileOrDie(`^-?[0-9]{4}(-(0[1-9]|1[0-2])(-(0[0-9]|[1-2][0-9]|3[0-1])(T([01][0-9]|2[0-3]):[0-5][0-9](:[0-5][0-9](\.[0-9]+)?)?(Z|(\+|-)((0[0-9]|1[0-3]):[0-5][0-9]|14:00))?)?)?)?$`)
	DateCompiledRegex = compileOrDie(`^-?[0-9]{4}(-(0[1-9]|1[0-2])(-(0[0-9]|[1-2][0-9]|3[0-1]))?)?$`)
	DateTimeCompiledRegex = compileOrDie(`^-?[0-9]{4}(-(0[1-9]|1[0-2])(-(0[0-9]|[1-2][0-9]|3[0-1])(T([01][0-9]|2[0-3]):[0-5][0-9]:[0-5][0-9](\.[0-9]+)?(Z|(\+|-)((0[0-9]|1[0-3]):[0-5][0-9]|14:00)))?)?)?$`)
	TimeCompiledRegex = compileOrDie(`^([01][0-9]|2[0-3]):[0-5][0-9]:[0-5][0-9](\.[0-9]+)?$`)
	InstantCompiledRegex = compileOrDie(`^-?[0-9]{4}-(0[1-9]|1[0-2])-(0[0-9]|[1-2][0-9]|3[0-1])T([01][0-9]|2[0-3]):[0-5][0-9]:[0-5][0-9](\.[0-9]+)?(Z|(\+|-)((0[0-9]|1[0-3]):[0-5][0-9]|14:00))$`)
	PositiveIntCompiledRegex = compileOrDie(`^[+]?[1-9][0-9]*$`)
	UnsignedIntCompiledRegex = compileOrDie(`^(0|([1-9][0-9]*))$`)
	JSP = jsoniter.ConfigCompatibleWithStandardLibrary

	// populate the required fields map.
	requiredFields = make(map[protoreflect.FullName][]protoreflect.FieldNumber)

	var emptyCR proto.Message
	// populate STU3 required fields
	emptyCR = &r3pb.ContainedResource{}
	findAllReferencedMessageTypes(
		emptyCR.ProtoReflect().Descriptor(),
		func(node protoreflect.MessageDescriptor) { collectDirectRequiredFields(node, requiredFields) },
	)
	// populate R4 required fields
	emptyCR = &r4pb.ContainedResource{}
	findAllReferencedMessageTypes(
		emptyCR.ProtoReflect().Descriptor(),
		func(node protoreflect.MessageDescriptor) { collectDirectRequiredFields(node, requiredFields) },
	)

	RegexValues = make(map[protoreflect.FullName]*regexp.Regexp)
	primitivesWithRegex := []protoreflect.Message{
		(&d3pb.Decimal{}).ProtoReflect(),
		(&d3pb.Oid{}).ProtoReflect(),
		(&d3pb.Id{}).ProtoReflect(),
		(&d3pb.Uuid{}).ProtoReflect(),
		(&d3pb.Code{}).ProtoReflect(),

		(&d4pb.Decimal{}).ProtoReflect(),
		(&d4pb.Oid{}).ProtoReflect(),
		(&d4pb.Id{}).ProtoReflect(),
		(&d4pb.Uuid{}).ProtoReflect(),
		(&d4pb.Code{}).ProtoReflect(),
	}
	for _, p := range primitivesWithRegex {
		p.Descriptor().Options().ProtoReflect().Range(func(f protoreflect.FieldDescriptor, v protoreflect.Value) bool {
			if f.Number() == apb.E_ValueRegex.TypeDescriptor().Number() {
				// Found the regex extension.
				RegexValues[p.Descriptor().FullName()] = compileOrDie(fmt.Sprintf("^%s$", v.String()))
			}
			return true
		})
	}

	initReferenceTypes()
}

func initReferenceTypes() {
	for _, refPB := range []proto.Message{
		&d3pb.Reference{},
		&d4pb.Reference{},
	} {
		refOneOf := refPB.ProtoReflect().Descriptor().Oneofs().ByName("reference")
		options := refOneOf.Fields()
		for i := 0; i < options.Len(); i++ {
			f := options.Get(i)
			var refType string
			ext := f.Options().ProtoReflect().Get(apb.E_ReferencedFhirType.TypeDescriptor())
			if ext.IsValid() {
				refType = ext.String()
			}
			if refType == "" {
				continue
			}

			if existing, loaded := referenceFieldToType[f.Name()]; loaded && existing != refType {
				panic("conflicting field types")
			}
			referenceFieldToType[f.Name()] = refType

			if existing, loaded := referenceTypeToField[refType]; loaded && existing != f.Name() {
				panic("conflicting field names")
			}
			referenceTypeToField[refType] = f.Name()
		}
	}
}

// ExtractTimezone returns the timezone from time.
func ExtractTimezone(t time.Time) string {
	_, offset := t.Zone()
	sign := "+"
	if offset < 0 {
		sign = "-"
		offset = -offset
	}
	hour := offset / 3600
	minute := offset % 3600 / 60
	return fmt.Sprintf("%s%02d:%02d", sign, hour, minute)
}

// ExtractTimezoneFromLoc returns the timezone from locale.
func ExtractTimezoneFromLoc(tz *time.Location) string {
	return ExtractTimezone(time.Now().In(tz))
}

// IsSubmilli checks if the date input is sub-millisecond.
func IsSubmilli(s string) bool {
	return subMilliDateCompiledRegex.MatchString(s)
}

// IsSubmilliTime checks if the time input is sub-millisecond.
func IsSubmilliTime(s string) bool {
	return subMilliTimeCompiledRegex.MatchString(s)
}

// IsSubsecond checks if the input is sub-second.
func IsSubsecond(s string) bool {
	return subSecondDateCompiledRegex.MatchString(s)
}

// IsSubsecondTime checks if the time input is sub-second.
func IsSubsecondTime(s string) bool {
	return subSecondTimeCompiledRegex.MatchString(s)
}

func offsetToSeconds(offset string) (int, error) {
	if offset == "" || offset == UTC {
		return 0, nil
	}
	sign := offset[0]
	if sign != '+' && sign != '-' {
		return 0, fmt.Errorf("invalid timezone offset: %v", offset)
	}
	arr := strings.Split(offset[1:], ":")
	if len(arr) != 2 {
		return 0, fmt.Errorf("invalid timezone offset: %v", offset)
	}
	hour, err := strconv.Atoi(arr[0])
	if err != nil {
		return 0, fmt.Errorf("invalid hour in timezone offset %v: %v", offset, err)
	}
	minute, err := strconv.Atoi(arr[1])
	if err != nil {
		return 0, fmt.Errorf("invalid minute in timezone offset %v: %v", offset, err)
	}
	if sign == '-' {
		return -hour*3600 - minute*60, nil
	}
	return hour*3600 + minute*60, nil
}

// GetLocation parses tz as an IANA location or a UTC offset.
func GetLocation(tz string) (*time.Location, error) {
	if tz == UTC {
		return time.UTC, nil
	}
	l, err := time.LoadLocation(tz)
	if err != nil {
		offset, err := offsetToSeconds(tz)
		if err != nil {
			return nil, err
		}
		return time.FixedZone(tz, offset), nil
	}
	return l, nil
}

// GetTimestampUsec converts t to a unix timestamp.
func GetTimestampUsec(t time.Time) int64 {
	s := t.Unix()
	if s < math.MaxInt64/int64(time.Second) && s > math.MinInt64/int64(time.Second) {
		return t.Unix()*1e6 + int64(t.Nanosecond()/1000)
	}
	return s * SecondToMicro
}

// GetTimeFromUsec generates a time.Time object from the given usec and timezone.
func GetTimeFromUsec(us int64, tz string) (time.Time, error) {
	l, err := GetLocation(tz)
	if err != nil {
		return time.Time{}, err
	}
	return time.Unix(us/1e6, (us%1e6)*1000).In(l), nil
}

// IsPrimitiveType returns true iff the message type d is a primitive FHIR data type.
func IsPrimitiveType(d protoreflect.MessageDescriptor) bool {
	ext := proto.GetExtension(d.Options(), apb.E_StructureDefinitionKind).(apb.StructureDefinitionKindValue)
	return ext == apb.StructureDefinitionKindValue_KIND_PRIMITIVE_TYPE
}

// IsResourceType returns true iff the message type d is a FHIR resource type.
func IsResourceType(d protoreflect.MessageDescriptor) bool {
	ext := proto.GetExtension(d.Options(), apb.E_StructureDefinitionKind).(apb.StructureDefinitionKindValue)
	return ext == apb.StructureDefinitionKindValue_KIND_RESOURCE
}

// IsChoice returns true iff the message type d is a FHIR choice type.
func IsChoice(d protoreflect.MessageDescriptor) bool {
	return d != nil && proto.HasExtension(d.Options(), apb.E_IsChoiceType)
}

// GetExtensionFieldDesc returns the extension field descriptor.
func GetExtensionFieldDesc(d protoreflect.MessageDescriptor) (protoreflect.FieldDescriptor, error) {
	f := d.Fields().ByName("extension")
	if f == nil {
		return nil, fmt.Errorf("no extension field found in %v", d.FullName())
	}
	if f.Cardinality() != protoreflect.Repeated || f.IsMap() {
		return nil, fmt.Errorf("extension field of %v is not repeated", d.FullName())
	}
	if f.Message() == nil {
		return nil, fmt.Errorf("extension field of %v has non-message type %v", d.FullName(), f.Kind())
	}
	return f, nil
}

// ParseTime parses a time into a struct that provides the time from midnight
// in microseconds and the precision of the original time.
func ParseTime(rm []byte) (Time, error) {
	var input string
	if err := JSP.Unmarshal(rm, &input); err != nil {
		return Time{}, err
	}
	// Time regular expression definition from https://www.hl7.org/fhir/datatypes.html
	if matched := TimeCompiledRegex.MatchString(input); !matched {
		return Time{}, fmt.Errorf("invalid time")
	}
	midnight, err := time.Parse(LayoutTimeSecond, MidnightTimeStr)
	if err != nil {
		return Time{}, fmt.Errorf("failed to parse midnight string %v", MidnightTimeStr)
	}
	precision := PrecisionSecond
	if IsSubmilliTime(input) {
		precision = PrecisionMicrosecond
	} else if IsSubsecondTime(input) {
		precision = PrecisionMillisecond
	}
	if t, err := time.Parse(LayoutTimeSecond, input); err == nil {
		return Time{
			Precision: precision,
			ValueUs:   t.Sub(midnight).Nanoseconds() / MicroToNano,
		}, nil
	}
	return Time{}, fmt.Errorf("invalid time layout: %v", input)
}

// SerializeTime serializes the values from a Time proto message to a JSON string.
func SerializeTime(us int64, precision Precision) (string, error) {
	ts, err := GetTimeFromUsec(us, UTC)
	if err != nil {
		return "", fmt.Errorf("in GetTimeFromUsec(): %v", err)
	}
	var tstr string
	switch precision {
	case PrecisionSecond:
		tstr = ts.Format(LayoutTimeSecond)
	case PrecisionMillisecond:
		tstr = ts.Format(LayoutTimeMilliSecond)
	case PrecisionMicrosecond:
		tstr = ts.Format(LayoutTimeMicroSecond)
	default:
		return "", fmt.Errorf("invalid time precision %v", precision)
	}
	return tstr, nil
}

// GetOneofField returns the oneof field, ensuring the given field is part of the given oneof.
func GetOneofField(messageType protoreflect.MessageDescriptor, oneofName, fieldName protoreflect.Name) (protoreflect.FieldDescriptor, error) {
	oneofDesc := messageType.Oneofs().ByName(oneofName)
	if oneofDesc == nil {
		return nil, fmt.Errorf("oneof field not found: %v", oneofName)
	}
	field := oneofDesc.Fields().ByName(fieldName)
	if field == nil {
		return nil, fmt.Errorf("field %v not found in oneof %v", fieldName, oneofName)
	}
	return field, nil
}

// HasInternalExtension returns true iff the proto message pb contains a Google-internal FHIR
// extension ext. Only the extension type is used, and its values, if populated, in the input
// extension is ignored.
func HasInternalExtension(pb, ext proto.Message) bool {
	if pb == nil || ext == nil {
		return false
	}
	url, err := InternalExtensionURL(ext.ProtoReflect().Descriptor())
	if err != nil {
		return false
	}
	return HasExtension(pb, url)
}

// AddInternalExtension adds a Google-internal extension ext if it is not existing in proto pb.
func AddInternalExtension(pb, ext proto.Message) error {
	m := pb.ProtoReflect()
	extField, err := GetExtensionFieldDesc(m.Descriptor())
	if err != nil {
		return err
	}
	list := m.Mutable(extField).List()
	for i := 0; i < list.Len(); i++ {
		if proto.Equal(list.Get(i).Message().Interface().(proto.Message), ext) {
			// Extension already exists.
			return nil
		}
	}
	list.Append(protoreflect.ValueOf(ext.ProtoReflect()))
	return nil
}

// RemoveInternalExtension removes extension ext from proto pb if it exists. Only the extension type
// is used, and its values, if populated, in the input extension is ignored.
func RemoveInternalExtension(pb, ext proto.Message) error {
	url, err := InternalExtensionURL(ext.ProtoReflect().Descriptor())
	if err != nil {
		return err
	}
	return RemoveExtension(pb, url)
}

// GetInternalExtension returns the first extension in pb whose URL matches that of extension ext
// from proto pb, and nil if there is no match. Only the extension type is used, and its values, if
// populated, in the input extension is ignored.
func GetInternalExtension(pb, ext proto.Message) (proto.Message, error) {
	url, err := InternalExtensionURL(ext.ProtoReflect().Descriptor())
	if err != nil {
		return nil, err
	}
	return GetExtension(pb, url)
}

// InternalExtensionURL returns the internal extension URL.
func InternalExtensionURL(desc protoreflect.MessageDescriptor) (string, error) {
	baseStrs := proto.GetExtension(desc.Options(), apb.E_FhirProfileBase).([]string)
	if len(baseStrs) == 0 {
		return "", fmt.Errorf("unable to get fhir_profile_base strings")
	}
	found := false
	for _, baseStr := range baseStrs {
		if baseStr == extStructDefURL {
			found = true
		}
	}
	if !found {
		return "", fmt.Errorf("message does not have Extension as a profile base")
	}
	if !proto.HasExtension(desc.Options(), apb.E_FhirStructureDefinitionUrl) {
		return "", fmt.Errorf("missing required fhir_structure_definition extension")
	}
	url := proto.GetExtension(desc.Options(), apb.E_FhirStructureDefinitionUrl).(string)
	if url == "" {
		return "", fmt.Errorf("unable to get fhir_structure_definition string")
	}
	return url, nil
}

// GetExtension returns the first extension in pb whose URL matches the given url,
// and nil if there is no match.
func GetExtension(pb proto.Message, url string) (proto.Message, error) {
	if pb == nil {
		return nil, nil
	}
	m := pb.ProtoReflect()
	extField, err := GetExtensionFieldDesc(m.Descriptor())
	if err != nil {
		return nil, err
	}
	list := m.Get(extField).List()
	for i := 0; i < list.Len(); i++ {
		m := list.Get(i).Message()
		if extensionHasURL(m, url) {
			return m.Interface().(proto.Message), nil
		}
	}
	return nil, nil
}

// HasExtension returns true iff the proto message has an extension with the given url
func HasExtension(pb proto.Message, url string) bool {
	if pb == nil {
		return false
	}
	m := pb.ProtoReflect()
	extField, err := GetExtensionFieldDesc(m.Descriptor())
	if err != nil {
		return false
	}
	list := m.Get(extField).List()
	for i := 0; i < list.Len(); i++ {
		if extensionHasURL(list.Get(i).Message(), url) {
			return true
		}
	}
	return false
}

// RemoveExtension removes extension with given url from proto pb if it exists.
func RemoveExtension(pb proto.Message, url string) error {
	if pb == nil {
		return nil
	}
	m := pb.ProtoReflect()
	extField, err := GetExtensionFieldDesc(m.Descriptor())
	if err != nil {
		return err
	}
	list := m.Mutable(extField).List()
	found := false
	var filtered []protoreflect.Message
	for i := 0; i < list.Len(); i++ {
		m := list.Get(i).Message()
		if extensionHasURL(m, url) {
			found = true
		} else {
			filtered = append(filtered, m)
		}
	}
	if found {
		list.Truncate(0)
		for _, m := range filtered {
			list.Append(protoreflect.ValueOf(m))
		}
	}
	return nil
}

// ExtensionURL returns the extension URL value.
func ExtensionURL(pb protoreflect.Message) (string, error) {
	url := pb.Descriptor().Fields().ByName("url")
	if url == nil {
		return "", fmt.Errorf("extension type %v has no url field", pb.Descriptor().FullName())
	}
	urlDesc := url.Message()
	if urlDesc == nil {
		return "", fmt.Errorf("url field of %v has non-message type %v", pb.Descriptor().FullName(), url.Kind())
	}
	urlValue := urlDesc.Fields().ByName("value")
	if urlValue == nil {
		return "", fmt.Errorf("url type %v has no value field", urlDesc.FullName())
	}
	if kind := urlValue.Kind(); kind != protoreflect.StringKind {
		return "", fmt.Errorf("url type %v has wrong type for value field: %v", urlDesc.FullName(), kind)
	}

	if !pb.Has(url) {
		return "", fmt.Errorf("url is not set for extension: %v", pb.Interface())
	}

	return pb.Get(url).Message().Get(urlValue).String(), nil
}

// ExtensionValue returns the extension value proto.
func ExtensionValue(pb protoreflect.Message) (protoreflect.Message, error) {
	value := pb.Descriptor().Fields().ByName("value")
	if value == nil {
		return nil, fmt.Errorf("extension of type %v has no value field", pb.Descriptor().FullName())
	}
	if value.Message() == nil {
		return nil, fmt.Errorf("extension of type %v has value field of kind %v; want message", pb.Descriptor().FullName(), value.Kind())
	}
	if value.Cardinality() == protoreflect.Repeated {
		return nil, fmt.Errorf("extension of type %v has repeated value field", pb.Descriptor().FullName())
	}
	if val := pb.Get(value); pb.Has(value) {
		return val.Message(), nil
	}
	return nil, nil
}

// ExtensionHasURL checks if an extension proto has the given URL.
func ExtensionHasURL(pb proto.Message, url string) bool {
	return extensionHasURL(pb.ProtoReflect(), url)
}

func extensionHasURL(pb protoreflect.Message, url string) bool {
	urlValue, err := ExtensionURL(pb)
	if err != nil {
		return false
	}
	return urlValue == url
}

// ExtensionFieldName takes the extension url and returns the field name for the extension.
// The new field name will be the substring after the last occurrence of "/" in the url, if it
// is empty, the field name will be the substring between the last two occurrences of "/" in the
// url. Any invalid BigQuery field name characters will be replaced with an underscore, and an
// underscore prefix will be added if the field name does not start with a letter or underscore.
func ExtensionFieldName(url string) string {
	url = strings.TrimSuffix(url, "/")
	parts := strings.Split(url, "/")
	fieldName := parts[len(parts)-1]
	return sanitizeFieldName(fieldName)
}

func sanitizeFieldName(fieldName string) string {
	// Replace all invalid characters with an underscore.
	fieldName = invalidBQChar.ReplaceAllString(fieldName, "_")

	// if the field name does not start with a letter or underscore, add an underscore as prefix.
	if invalidBQStart.MatchString(fieldName) {
		fieldName = fmt.Sprintf("_%s", fieldName)
	}

	return fieldName
}

// FullExtensionFieldName uses the url to construct a full extension field name that's compliant
// with BigQuery field name requirement.
func FullExtensionFieldName(url string) string {
	fieldName := strings.TrimPrefix(url, "http://")
	fieldName = strings.TrimPrefix(fieldName, "https://")
	return sanitizeFieldName(fieldName)
}

// ValidateString returns an error is the string does not conform to FHIR
// requirements for size and control characters.
func ValidateString(s string) error {
	if len(s) > maxStringSize {
		return &UnmarshalError{
			Details: "string exceeds maximum size of 1 MB",
		}
	}
	if matches := invalidStringChars.FindStringSubmatch(s); matches != nil {
		return &UnmarshalError{
			Details: fmt.Sprintf("string contains invalid characters: %U", matches[0][0]),
		}
	}
	return nil
}

// ResourceIDField returns the resource-typed field that is populated in this
// reference, or nil if this is another type of reference.
func ResourceIDField(ref protoreflect.Message) (protoreflect.FieldDescriptor, error) {
	od := ref.Descriptor().Oneofs().ByName(RefOneofName)
	if od == nil {
		return nil, &UnmarshalError{Details: "unexpected reference"}
	}
	f := ref.WhichOneof(od)
	if f == nil {
		// Identifier/text reference, can't validate this.
		return nil, nil
	}
	if !strings.HasSuffix(string(f.Name()), RefFieldSuffix) {
		return nil, nil
	}

	return f, nil
}

// ResourceTypeForReference returns the resource type that is associated with
// this reference field.
func ResourceTypeForReference(resField protoreflect.Name) (string, bool) {
	resType, ok := referenceFieldToType[resField]
	return resType, ok
}

// ReferenceFieldForType returns the reference field that should be populated
// for the supplied resource type.
func ReferenceFieldForType(resType string) (protoreflect.Name, bool) {
	f, ok := referenceTypeToField[resType]
	return f, ok
}

// ValidateReferenceType returns an error is `ref` is a strongly typed
// reference that is not compatible with the types allowed by `msgField`.
// References should be normalized before being passed to this function.
func ValidateReferenceType(msgField protoreflect.FieldDescriptor, ref protoreflect.Message) error {
	var validRefTypes []string
	ext := msgField.Options().ProtoReflect().Get(apb.E_ValidReferenceType.TypeDescriptor())
	if ext.IsValid() {
		validRefTypes = apb.E_ValidReferenceType.InterfaceOf(ext).([]string)
	} else {
		validRefTypes = nil
	}

	if len(validRefTypes) == 0 {
		return nil
	}

	f, err := ResourceIDField(ref)
	if err != nil {
		return err
	}
	if f == nil {
		return nil
	}
	refType, ok := ResourceTypeForReference(f.Name())
	if !ok {
		return nil
	}

	for _, validRefType := range validRefTypes {
		if validRefType == RefAnyResource {
			return nil
		}
		if refType == validRefType {
			return nil
		}
	}
	return &UnmarshalError{
		// refType must be one of the spec error types because it was in the Oneof. This means it can't
		// contain any sensitive information.
		Type:    ReferenceTypeError,
		Details: fmt.Sprintf("invalid reference to a %v resource, want %v", refType, strings.Join(validRefTypes, ", ")),
	}
}

// ValidateRequiredFields returns an error if any field isn't populated in pb
// that should be, according to the ValidationRequirement annotation.
func ValidateRequiredFields(pb protoreflect.Message) error {
	var el UnmarshalErrorList
	for _, requiredField := range requiredFields[pb.Descriptor().FullName()] {
		field := pb.Descriptor().Fields().ByNumber(requiredField)
		if !pb.Has(field) {
			el = append(el, &UnmarshalError{
				Type:    RequiredFieldError,
				Details: fmt.Sprintf("missing required field %q", field.JSONName()),
			})
		}
	}
	if len(el) > 0 {
		return el
	}
	return nil
}

// collectDirectRequiredFields checks all the fields in the given message descriptor, collect the field numbers
// of the fields that are required according to FHIR spec, cache the collected numbers in the sink map, indexed
// by the message's full name.
func collectDirectRequiredFields(msgDesc protoreflect.MessageDescriptor, sink map[protoreflect.FullName][]protoreflect.FieldNumber) {
	fields := msgDesc.Fields()
	required := []protoreflect.FieldNumber{}
	for i := 0; i < fields.Len(); i++ {
		f := fields.Get(i)
		ext := proto.GetExtension(f.Options(), apb.E_ValidationRequirement).(apb.Requirement)
		if ext == apb.Requirement_REQUIRED_BY_FHIR {
			required = append(required, f.Number())
		}
	}
	sink[msgDesc.FullName()] = required
}

// findAllReferencedMessageTypes does a BFS traversal of the message
// descriptors fields starting from the provided root. `onVisit` is called once
// for each unique message type that is found.
func findAllReferencedMessageTypes(root protoreflect.MessageDescriptor, onVisit func(node protoreflect.MessageDescriptor)) {
	visited := stringset.New()
	worklist := []protoreflect.MessageDescriptor{root}
	for len(worklist) > 0 {
		node := worklist[0]
		worklist = worklist[1:]
		if visited.Contains(string(node.FullName())) {
			continue
		}

		onVisit(node)

		visited.Add(string(node.FullName()))
		fields := node.Fields()
		for i := 0; i < fields.Len(); i++ {
			f := fields.Get(i)
			if f.Kind() != protoreflect.MessageKind {
				continue
			}
			worklist = append(worklist, f.Message())
		}
	}
}

// UnmarshalCode interprets `rm` as a value of the enum that is the same type
// as `in`. `in` will not be modified.
func UnmarshalCode(jsonPath string, in protoreflect.Message, rm json.RawMessage) (proto.Message, error) {
	d := in.Descriptor()
	f := d.Fields().ByName("value")
	if f == nil {
		return nil, fmt.Errorf("value field not found in proto: %s", d.Name())
	}
	if !utf8.Valid(rm) {
		return nil, &UnmarshalError{
			Path:        jsonPath,
			Details:     "expected UTF-8 encoding",
			Diagnostics: fmt.Sprintf("found %q", rm),
		}
	}
	var val string
	if err := json.Unmarshal([]byte(rm), &val); err != nil {
		return nil, &UnmarshalError{
			Path:        jsonPath,
			Details:     "expected code",
			Diagnostics: fmt.Sprintf("found %s", rm),
		}
	}
	// Create an empty instance of the same type as input proto.
	pb := in.New()
	switch f.Kind() {
	case protoreflect.StringKind:
		pb.Set(f, protoreflect.ValueOf(val))
		return pb.Interface().(proto.Message), nil
	case protoreflect.EnumKind:
		enum := strings.Replace(strings.ToUpper(val), "-", "_", -1)
		if v := f.Enum().Values().ByName(protoreflect.Name(enum)); v != nil && v.Number() != 0 {
			pb.Set(f, protoreflect.ValueOf(v.Number()))
			return pb.Interface().(proto.Message), nil
		}
		// Try again, explicitly looking for original codes.
		values := f.Enum().Values()
		for i := 0; i < values.Len(); i++ {
			ev := values.Get(i)
			origCode := proto.GetExtension(ev.Options(), apb.E_FhirOriginalCode).(string)
			if origCode == val {
				pb.Set(f, protoreflect.ValueOf(ev.Number()))
				return pb.Interface().(proto.Message), nil
			}
		}
		typeName := f.Enum().FullName().Parent().Name()
		return nil, &UnmarshalError{
			Path:        jsonPath,
			Details:     "code type mismatch",
			Diagnostics: fmt.Sprintf("%q is not a %s", val, typeName),
		}
	default:
		return nil, fmt.Errorf("unexpected field kind %v, want enum", f.Kind())
	}
}

// FieldMap returns a lookup table for a message's fields from the FHIR JSON
// field names. Choice fields map to the choice message type.
func FieldMap(desc protoreflect.MessageDescriptor) map[string]protoreflect.FieldDescriptor {
	messageFieldsMutex.RLock()
	fieldMap, ok := messageFields[desc]
	if ok {
		messageFieldsMutex.RUnlock()
		return fieldMap
	}
	messageFieldsMutex.RUnlock()

	messageFieldsMutex.Lock()
	defer messageFieldsMutex.Unlock()
	fieldMap = buildFieldMap(desc)
	messageFields[desc] = fieldMap
	return fieldMap
}

func buildFieldMap(desc protoreflect.MessageDescriptor) map[string]protoreflect.FieldDescriptor {
	fields := desc.Fields()
	fieldMap := map[string]protoreflect.FieldDescriptor{}
	for i := 0; i < fields.Len(); i++ {
		f := fields.Get(i)
		if IsChoice(f.Message()) {
			choiceFields := buildFieldMap(f.Message())
			for name := range choiceFields {
				if strings.HasPrefix(name, "_") {
					name = "_" + f.JSONName() + strings.Title(name[1:])
				} else {
					name = f.JSONName() + strings.Title(name)
				}
				fieldMap[name] = f
			}
		} else {
			fieldMap[f.JSONName()] = f
			if f.Kind() == protoreflect.MessageKind && IsPrimitiveType(f.Message()) {
				fieldMap["_"+f.JSONName()] = f
			}
		}
	}
	return fieldMap
}

// AddFieldToPath extends a JSON path with another field.
func AddFieldToPath(jsonPath, field string) string {
	if jsonPath == "" {
		return field
	}
	return strings.Join([]string{jsonPath, field}, ".")
}

// AddIndexToPath extends a JSON path with an index.
func AddIndexToPath(jsonPath string, index int) string {
	return jsonPath + "[" + strconv.Itoa(index) + "]"
}

// IsTimeLike FHIR data type. These are Date, DateTime, Time and Instant.
func IsTimeLike(pb proto.Message) bool {
	name := pb.ProtoReflect().Descriptor().Name()
	return name == "Date" || name == "DateTime" || name == "Time" || name == "Instant"
}
