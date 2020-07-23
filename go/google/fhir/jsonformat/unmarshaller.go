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

package jsonformat

import (
	"bytes"
	"encoding/json"
	"fmt"
	"math"
	"net/url"
	"strconv"
	"strings"
	"time"
	"unicode/utf8"

	"google/fhir/jsonformat/internal/accessor/accessor"
	"google/fhir/jsonformat/internal/jsonpbhelper/jsonpbhelper"
	"github.com/json-iterator/go"
	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes"
	"google.golang.org/protobuf/reflect/protoreflect"

	anypb "google.golang.org/protobuf/types/known/anypb"
	descpb "google.golang.org/protobuf/types/descriptorpb"
	apb "google/fhir/proto/annotations_go_proto"
)

var (
	jsp jsoniter.API
)

func init() {
	jsp = jsoniter.Config{
		ValidateJsonRawMessage: true,
	}.Froze()
}

// Unmarshaller is an object for converting a JSON object to protocol buffer.
type Unmarshaller struct {
	TimeZone *time.Location
	// MaxNestingDepth is the maximum number of levels a field can have. The unmarshaller will
	// return an error when a resource has a field exceeding this limit. If the value is negative
	// or 0, then the maximum nesting depth is unbounded.
	MaxNestingDepth  int
	enableValidation bool
	cfg              config
}

// NewUnmarshaller returns an Unmarshaller that performs resource validation.
func NewUnmarshaller(tz string, ver Version) (*Unmarshaller, error) {
	return newUnmarshaller(tz, ver, true /*enableValidation*/)
}

// NewUnmarshallerWithoutValidation returns an Unmarshaller that doesn't perform resource validation.
func NewUnmarshallerWithoutValidation(tz string, ver Version) (*Unmarshaller, error) {
	return newUnmarshaller(tz, ver, false /*enableValidation*/)
}

func newUnmarshaller(tz string, ver Version, enableValidation bool) (*Unmarshaller, error) {
	cfg, err := getConfig(ver)
	if err != nil {
		return nil, err
	}
	l, err := time.LoadLocation(tz)
	if err != nil {
		return nil, err
	}

	return &Unmarshaller{
		TimeZone:         l,
		cfg:              cfg,
		enableValidation: enableValidation,
	}, nil
}

// Unmarshal returns the corresponding protobuf message given a serialized FHIR JSON object
func (u *Unmarshaller) Unmarshal(in []byte) (proto.Message, error) {
	// Decode the JSON object into a map.
	var decoded map[string]json.RawMessage
	if err := jsp.Unmarshal(in, &decoded); err != nil {
		return nil, &jsonpbhelper.UnmarshalError{Details: "invalid JSON", Diagnostics: err.Error()}
	}
	return u.parseContainedResource("", decoded)
}

func addFieldToPath(jsonPath, field string) string {
	if jsonPath == "" {
		return field
	}
	return strings.Join([]string{jsonPath, field}, ".")
}

func addIndexToPath(jsonPath string, index int) string {
	return jsonPath + "[" + strconv.Itoa(index) + "]"
}

func (u *Unmarshaller) checkCurrentDepth(jsonPath string) error {
	if u.MaxNestingDepth <= 0 {
		return nil
	}
	depth := strings.Count(jsonPath, ".")
	if depth > u.MaxNestingDepth {
		return &jsonpbhelper.UnmarshalError{
			Path:    jsonPath,
			Details: fmt.Sprintf("field exceeded the maximum nesting depth %d", u.MaxNestingDepth),
		}
	}
	return nil
}

func lastFieldInPath(jsonPath string) string {
	sp := strings.Split(jsonPath, ".")
	s := sp[len(sp)-1]
	return strings.Split(s, "[")[0]
}

func annotateUnmarshalErrorWithPath(err error, jsonPath string) error {
	if umErr, ok := err.(*jsonpbhelper.UnmarshalError); ok {
		umErr.Path = jsonPath
		return umErr
	}
	return err
}

func (u *Unmarshaller) parseContainedResource(jsonPath string, decmap map[string]json.RawMessage) (proto.Message, error) {
	// Determine the type of the resource.
	rt, ok := decmap[jsonpbhelper.ResourceTypeField]
	if !ok {
		return nil, &jsonpbhelper.UnmarshalError{
			Path:    jsonPath,
			Details: fmt.Sprintf("missing required field %q", jsonpbhelper.ResourceTypeField),
		}
	}
	var rtstr string
	if err := jsp.Unmarshal(rt, &rtstr); err != nil {
		return nil, &jsonpbhelper.UnmarshalError{
			Path:        jsonPath,
			Details:     "invalid resource type",
			Diagnostics: string(rt),
		}
	}
	delete(decmap, jsonpbhelper.ResourceTypeField)
	jsonPath = addFieldToPath(jsonPath, rtstr)

	// Populate the fields in the protobuf message to return.
	// Encapsulate in a ContainedResource.
	cr := u.cfg.newEmptyContainedResource()
	rcr := proto.MessageReflect(cr)
	pbdesc := rcr.Descriptor()
	oneofDesc := pbdesc.Oneofs().ByName(jsonpbhelper.OneofName)
	if oneofDesc == nil {
		return nil, fmt.Errorf("oneof field not found: %v", jsonpbhelper.OneofName)
	}
	for i := 0; i < oneofDesc.Fields().Len(); i++ {
		f := oneofDesc.Fields().Get(i)
		if f.Message() != nil && string(f.Message().Name()) == rtstr {
			if err := u.mergeMessage(jsonPath, decmap, rcr.Mutable(f).Message()); err != nil {
				return nil, err
			}
			return cr, nil
		}
	}
	return nil, &jsonpbhelper.UnmarshalError{
		Path:        jsonPath,
		Details:     fmt.Sprintf("unknown resource type"),
		Diagnostics: strconv.Quote(rtstr),
	}
}

func (u *Unmarshaller) mergeRawMessage(jsonPath string, rm json.RawMessage, pb protoreflect.Message) error {
	var decmap map[string]json.RawMessage
	if err := jsp.Unmarshal(rm, &decmap); err != nil {
		return &jsonpbhelper.UnmarshalError{
			Path:        jsonPath,
			Details:     "invalid JSON",
			Diagnostics: fmt.Sprintf("%.50s", rm),
		}
	}
	return u.mergeMessage(jsonPath, decmap, pb)
}

func (u *Unmarshaller) mergeMessage(jsonPath string, decmap map[string]json.RawMessage, pb protoreflect.Message) error {
	if err := u.checkCurrentDepth(jsonPath); err != nil {
		return err
	}
	if pb == nil {
		return fmt.Errorf("nil message for json input: %v", decmap)
	}
	pbdesc := pb.Descriptor()
	if pbdesc.Name() == containedResourceProtoName(u.cfg) {
		// Special handling of ContainedResource.
		cr, err := u.parseContainedResource(jsonPath, decmap)
		if err != nil {
			return err
		}
		proto.Merge(protoMessage(pb), cr)
		return nil
	}
	if pbdesc.Name() == protoName(&anypb.Any{}) && lastFieldInPath(jsonPath) == jsonpbhelper.ContainedField {
		// Special handling of inlined resources, with 'contained' JSON field name and Any proto type.
		cr, err := u.parseContainedResource(jsonPath, decmap)
		if err != nil {
			return err
		}
		any, err := ptypes.MarshalAny(cr)
		if err != nil {
			return err
		}
		proto.Merge(protoMessage(pb), any)
		return nil
	}
	fieldMap := jsonpbhelper.FieldMap(pbdesc)
	// Iterate through all fields, and merge to the proto.
	for k, v := range decmap {
		if u.cfg.keysToSkip().Contains(k) {
			continue
		}

		// TODO: reject upper camel case fields names after suitable deprecation warning.
		var normalizedFieldName string
		if strings.HasPrefix(k, "_") {
			normalizedFieldName = "_" + lowerFirst(k[1:])
		} else {
			normalizedFieldName = lowerFirst(k)
		}

		f, ok := fieldMap[normalizedFieldName]
		if !ok {
			return &jsonpbhelper.UnmarshalError{
				Path:        jsonPath,
				Details:     "unknown field",
				Diagnostics: strconv.Quote(k),
			}
		}
		if jsonpbhelper.IsChoice(f.Message()) {
			if err := u.mergeChoiceField(jsonPath, f, k, v, pb); err != nil {
				return err
			}
		} else if err := u.mergeField(addFieldToPath(jsonPath, k), f, v, pb); err != nil {
			return err
		}
	}
	if u.enableValidation {
		if err := jsonpbhelper.ValidateRequiredFields(pb); err != nil {
			return annotateUnmarshalErrorWithPath(err, jsonPath)
		}
	}
	return nil
}

// returns a copy of the input string with a lower case first character.
func lowerFirst(s string) string {
	if len(s) == 0 {
		return s
	}
	return strings.ToLower(s[0:1]) + s[1:]
}

func (u *Unmarshaller) mergeChoiceField(jsonPath string, f protoreflect.FieldDescriptor, k string, v json.RawMessage, pb protoreflect.Message) error {
	fieldMap := jsonpbhelper.FieldMap(f.Message())

	// TODO: reject upper camel case fields names after suitable deprecation warning.
	var choiceFieldName string
	if strings.HasPrefix(k, "_") {
		// Convert ex: "_valueString" and "_ValueString" interchangeably to "_string".
		choiceFieldName = "_" + lowerFirst(k[1:])
		choiceFieldName = strings.TrimPrefix(choiceFieldName, "_"+f.JSONName())
		choiceFieldName = "_" + lowerFirst(choiceFieldName)
	} else {
		// Convert ex: "ValueString" and "valueString" interchangeably to "string".
		choiceFieldName = lowerFirst(k)
		choiceFieldName = lowerFirst(strings.TrimPrefix(choiceFieldName, f.JSONName()))
	}

	choiceField, ok := fieldMap[choiceFieldName]
	if !ok {
		return &jsonpbhelper.UnmarshalError{
			Path:        jsonPath,
			Details:     "unknown field",
			Diagnostics: strconv.Quote(k),
		}
	}
	return u.mergeField(addFieldToPath(jsonPath, k), choiceField, v, pb.Mutable(f).Message())
}

func (u *Unmarshaller) mergeField(jsonPath string, f protoreflect.FieldDescriptor, v json.RawMessage, pb protoreflect.Message) error {
	if err := u.checkCurrentDepth(jsonPath); err != nil {
		return err
	}
	switch f.Cardinality() {
	case protoreflect.Optional:
		if pb.Has(f) {
			if !jsonpbhelper.IsPrimitiveType(f.Message()) {
				return &jsonpbhelper.UnmarshalError{
					Path:    jsonPath,
					Details: "invalid extension field",
				}
			}
			p, err := u.parsePrimitiveType(jsonPath, pb.Get(f).Message(), v)
			if err != nil {
				return err
			}
			return u.mergePrimitiveType(protoMessage(pb.Mutable(f).Message()), p)
		}
		if err := u.mergeSingleField(jsonPath, f, v, pb.Mutable(f).Message()); err != nil {
			return err
		}
	case protoreflect.Repeated:
		var rms []json.RawMessage
		if err := jsp.Unmarshal(v, &rms); err != nil {
			return &jsonpbhelper.UnmarshalError{
				Path:    jsonPath,
				Details: "expected array",
			}
		}
		if err := u.mergeRepeatedField(jsonPath, f, rms, pb); err != nil {
			return err
		}
	default:
		return fmt.Errorf("unsupported cardinality %v: %v", f.Cardinality(), f.Name())
	}
	return nil
}

func (u *Unmarshaller) mergeRepeatedField(jsonPath string, f protoreflect.FieldDescriptor, rms []json.RawMessage, pb protoreflect.Message) error {
	rf := pb.Mutable(f).List()
	switch rf.Len() {
	case 0:
		for i, e := range rms {
			msg := rf.AppendMutable().Message()
			if err := u.mergeSingleField(addIndexToPath(jsonPath, i), f, e, msg); err != nil {
				return err
			}
		}
	case len(rms):
		for i, e := range rms {
			if err := u.mergeSingleField(addIndexToPath(jsonPath, i), f, e, rf.Get(i).Message()); err != nil {
				return err
			}
		}
	default:
		return &jsonpbhelper.UnmarshalError{
			Path:    jsonPath,
			Details: fmt.Sprintf("array length mismatch, expected %d, found %d", rf.Len(), len(rms)),
		}
	}
	return nil
}

func (u *Unmarshaller) mergeSingleField(jsonPath string, f protoreflect.FieldDescriptor, rm json.RawMessage, pb protoreflect.Message) error {
	d := f.Message()
	if jsonpbhelper.IsPrimitiveType(d) {
		p, err := u.parsePrimitiveType(jsonPath, pb, rm)
		if err != nil {
			return err
		}
		return u.mergePrimitiveType(protoMessage(pb), p)
	}
	if !proto.HasExtension(d.Options().(*descpb.MessageOptions), apb.E_FhirReferenceType) {
		return u.mergeRawMessage(jsonPath, rm, pb)
	}

	if err := u.mergeReference(jsonPath, rm, pb); err != nil {
		return err
	}
	if u.enableValidation {
		if err := jsonpbhelper.ValidateReferenceType(f, pb); err != nil {
			return annotateUnmarshalErrorWithPath(err, jsonPath)
		}
	}
	return nil
}

func (u *Unmarshaller) mergeReference(jsonPath string, rm json.RawMessage, pb protoreflect.Message) error {
	if err := u.mergeRawMessage(jsonPath, rm, pb); err != nil {
		return err
	}
	return NormalizeReference(protoMessage(pb))
}

func (u *Unmarshaller) mergePrimitiveType(dst, src proto.Message) error {
	if proto.Size(src) == 0 {
		// No merging necessary.
		return nil
	}
	if proto.Size(dst) == 0 {
		proto.Merge(dst, src)
		return nil
	}
	nv := u.cfg.newPrimitiveHasNoValue(true)
	dnv := jsonpbhelper.HasInternalExtension(dst, nv)
	if dnv {
		if err := jsonpbhelper.RemoveInternalExtension(dst, nv); err != nil {
			return err
		}
	}
	snv := jsonpbhelper.HasInternalExtension(src, nv)
	proto.Merge(dst, src)
	if !dnv && snv {
		// Remove the HasNoValue extension if dst actually has value.
		if err := jsonpbhelper.RemoveInternalExtension(dst, nv); err != nil {
			return err
		}
	}
	return nil
}

func (u *Unmarshaller) parsePrimitiveType(jsonPath string, in protoreflect.Message, rm json.RawMessage) (proto.Message, error) {
	// jsoniter doesn't remove the whitespace between an object property and its
	// value when unmarshaling into a RawMessage. As a result, in {"foo":     "bar"},
	// rm will contain "    \"bar\"". Trimming does not change the value itself.
	rm = bytes.TrimSpace(rm)
	if len(rm) > 0 && (rm[0] == '{' || rm[0] == '[') {
		// The raw message is a JsonObject, this is a special case for primitive type extensions.
		// Create an empty instance of the same type as input proto.
		pb := in.New()
		if err := u.mergeRawMessage(jsonPath, rm, pb); err != nil {
			return nil, err
		}
		extListInPb, err := accessor.GetList(pb, "extension")
		if err != nil {
			return nil, fmt.Errorf("get repeated field: extension failed, err: %v", err)
		}
		ext := extListInPb.NewElement().Message().Interface().(proto.Message)
		if err := protoToExtension(u.cfg.newPrimitiveHasNoValue(true), ext); err != nil {
			return nil, err
		}
		if err := jsonpbhelper.AddInternalExtension(protoMessage(pb), ext); err != nil {
			return nil, err
		}
		return protoMessage(pb), nil
	}
	d := in.Descriptor()
	createAndSetValue := func(val interface{}) (proto.Message, error) {
		rpb := in.New()
		if err := accessor.SetValue(rpb, val, "value"); err != nil {
			return nil, err
		}
		return rpb.Interface().(proto.Message), nil
	}
	// Make sure string fields have valid UTF-8 encoding.
	switch d.Name() {
	case "Code", "Id", "Oid", "String", "Url", "Uri", "Canonical", "Markdown", "Xhtml", "Uuid":
		if !utf8.Valid(rm) {
			return nil, &jsonpbhelper.UnmarshalError{
				Path:        jsonPath,
				Details:     "expected UTF-8 encoding",
				Diagnostics: fmt.Sprintf("found %q", rm),
			}
		}
	}
	switch d.Name() {
	case "Base64Binary":
		m := in.New().Interface().(proto.Message)
		if err := parseBinary(rm, m, u.cfg.newBase64BinarySeparatorStride); err != nil {
			return nil, &jsonpbhelper.UnmarshalError{
				Path:        jsonPath,
				Details:     "expected binary data",
				Diagnostics: fmt.Sprintf("found %s", rm),
			}
		}
		return m, nil
	case "Boolean":
		var val bool
		if err := jsp.Unmarshal(rm, &val); err != nil {
			return nil, &jsonpbhelper.UnmarshalError{
				Path:        jsonPath,
				Details:     "expected boolean",
				Diagnostics: fmt.Sprintf("found %s", rm),
			}
		}
		return createAndSetValue(val)
	case "Code":
		var val string
		if err := jsp.Unmarshal(rm, &val); err != nil {
			return nil, &jsonpbhelper.UnmarshalError{
				Path:        jsonPath,
				Details:     "expected code",
				Diagnostics: fmt.Sprintf("found %s", rm),
			}
		}
		// Code cannot start or end with a blank.
		if matched := jsonpbhelper.CodeCompiledRegex.MatchString(val); !matched {
			return nil, &jsonpbhelper.UnmarshalError{
				Path:        jsonPath,
				Details:     "invalid code",
				Diagnostics: fmt.Sprintf("found %s", rm),
			}
		}
		return createAndSetValue(val)
	case "Date":
		m := in.New().Interface().(proto.Message)
		if err := parseDateFromJSON(rm, u.TimeZone, m); err != nil {
			return nil, &jsonpbhelper.UnmarshalError{
				Path:        jsonPath,
				Details:     "expected date",
				Diagnostics: fmt.Sprintf("found %s", rm),
			}
		}
		return m, nil
	case "DateTime":
		m := in.New().Interface().(proto.Message)
		if err := ParseDateTimeFromJSON(rm, u.TimeZone, m); err != nil {
			return nil, &jsonpbhelper.UnmarshalError{
				Path:        jsonPath,
				Details:     "expected datetime",
				Diagnostics: fmt.Sprintf("found %s", rm),
			}
		}
		return m, nil
	case "Decimal":
		m := in.New().Interface().(proto.Message)
		if err := parseDecimal(rm, m); err != nil {
			return nil, &jsonpbhelper.UnmarshalError{
				Path:        jsonPath,
				Details:     "expected decimal",
				Diagnostics: fmt.Sprintf("found %s", rm),
			}
		}
		return m, nil
	case "Id":
		var val string
		if err := jsp.Unmarshal(rm, &val); err != nil {
			return nil, &jsonpbhelper.UnmarshalError{
				Path:        jsonPath,
				Details:     "expected ID",
				Diagnostics: fmt.Sprintf("found %s", rm),
			}
		}
		// FHIR id's must be no longer than 64 characters and contain only letters, numbers,
		// dashes (-) and dots (.).
		matched := jsonpbhelper.IDCompiledRegex.MatchString(val)
		if !matched {
			return nil, &jsonpbhelper.UnmarshalError{
				Path:        jsonPath,
				Details:     "invalid ID",
				Diagnostics: fmt.Sprintf("found %s", rm),
			}
		}
		return createAndSetValue(val)
	case "Instant":
		m := in.New().Interface().(proto.Message)
		if err := parseInstant(rm, m); err != nil {
			return nil, &jsonpbhelper.UnmarshalError{
				Path:        jsonPath,
				Details:     "expected instant",
				Diagnostics: fmt.Sprintf("found %s", rm),
			}
		}
		return m, nil
	case "Integer":
		var val int32
		if err := jsp.Unmarshal(rm, &val); err != nil {
			return nil, &jsonpbhelper.UnmarshalError{
				Path:        jsonPath,
				Details:     "expected integer",
				Diagnostics: fmt.Sprintf("found %s", rm),
			}
		}
		return createAndSetValue(val)
	case "Oid":
		var val string
		if err := jsp.Unmarshal(rm, &val); err != nil {
			return nil, &jsonpbhelper.UnmarshalError{
				Path:        jsonPath,
				Details:     "expected OID",
				Diagnostics: fmt.Sprintf("found %s", rm),
			}
		}
		// FHIR oid's must be RFC 3001 compliant.
		matched := jsonpbhelper.OIDCompiledRegex.MatchString(val)
		if !matched {
			return nil, &jsonpbhelper.UnmarshalError{
				Path:        jsonPath,
				Details:     "invalid OID",
				Diagnostics: fmt.Sprintf("found %s", rm),
			}
		}
		return createAndSetValue(val)
	case "PositiveInt":
		// Ensure that the JSON object to parse is a positive integer
		matched := jsonpbhelper.PositiveIntCompiledRegex.MatchString(string(rm))
		if !matched {
			return nil, &jsonpbhelper.UnmarshalError{
				Path:        jsonPath,
				Details:     "expected positive integer",
				Diagnostics: fmt.Sprintf("found %s", rm),
			}
		}
		var val uint32
		if err := jsp.Unmarshal(rm, &val); err != nil || val > math.MaxInt32 {
			return nil, &jsonpbhelper.UnmarshalError{
				Path:        jsonPath,
				Details:     "invalid positive integer",
				Diagnostics: fmt.Sprintf("found %s", rm),
			}
		}
		return createAndSetValue(val)
	case "String":
		var val string
		if err := jsp.Unmarshal(rm, &val); err != nil {
			return nil, &jsonpbhelper.UnmarshalError{
				Path:        jsonPath,
				Details:     "expected string",
				Diagnostics: fmt.Sprintf("found %s", rm),
			}
		}
		if err := jsonpbhelper.ValidateString(val); err != nil {
			return nil, annotateUnmarshalErrorWithPath(err, jsonPath)
		}
		return createAndSetValue(val)
	case "Time":
		m := in.New().Interface().(proto.Message)
		if err := parseTime(rm, m); err != nil {
			return nil, &jsonpbhelper.UnmarshalError{
				Path:        jsonPath,
				Details:     "invalid time",
				Diagnostics: err.Error(),
			}
		}
		return m, nil
	case "UnsignedInt":
		// Ensure that the JSON object to parse is an unsigned integer
		matched := jsonpbhelper.UnsignedIntCompiledRegex.MatchString(string(rm))
		if !matched {
			return nil, &jsonpbhelper.UnmarshalError{
				Path:        jsonPath,
				Details:     "invalid non-negative integer",
				Diagnostics: fmt.Sprintf("found %s", rm),
			}
		}
		var val uint32
		// The spec doesn't actually allow the full range of an unsigned integer, it's only the range of
		// PositiveInt plus 0.
		if err := jsp.Unmarshal(rm, &val); err != nil || val > math.MaxInt32 {
			return nil, &jsonpbhelper.UnmarshalError{
				Path:    jsonPath,
				Details: "non-negative integer out of range 0..2,147,483,647",
			}
		}
		return createAndSetValue(val)
	case "Url", "Uri", "Canonical":
		valType := strings.ToLower(string(d.Name()))
		var val string
		if err := jsp.Unmarshal(rm, &val); err != nil {
			return nil, &jsonpbhelper.UnmarshalError{
				Path:        jsonPath,
				Details:     fmt.Sprintf("expected %s", valType),
				Diagnostics: fmt.Sprintf("found %s", rm),
			}
		}
		if _, err := url.Parse(val); err != nil {
			return nil, &jsonpbhelper.UnmarshalError{
				Path:        jsonPath,
				Details:     fmt.Sprintf("invalid %s", valType),
				Diagnostics: fmt.Sprintf("found %s", rm),
			}
		}
		return createAndSetValue(val)
	case "Markdown", "Xhtml":
		valType := strings.ToLower(string(d.Name()))
		var val string
		if err := jsp.Unmarshal(rm, &val); err != nil {
			return nil, &jsonpbhelper.UnmarshalError{
				Path:        jsonPath,
				Details:     fmt.Sprintf("expected %s", valType),
				Diagnostics: fmt.Sprintf("found %s", rm),
			}
		}
		return createAndSetValue(val)
	case "Uuid":
		var val string
		if err := jsp.Unmarshal(rm, &val); err != nil {
			return nil, &jsonpbhelper.UnmarshalError{
				Path:        jsonPath,
				Details:     "expected UUID",
				Diagnostics: fmt.Sprintf("found %s", rm),
			}
		}
		// UUID must be of RFC4122 format.
		matched := jsonpbhelper.UUIDCompiledRegex.MatchString(val)
		if !matched {
			return nil, &jsonpbhelper.UnmarshalError{
				Path:        jsonPath,
				Details:     "invalid UUID",
				Diagnostics: fmt.Sprintf("found %s", rm),
			}
		}
		return createAndSetValue(val)
	}

	// Handles specialized codes.
	if proto.HasExtension(d.Options().(*descpb.MessageOptions), apb.E_FhirValuesetUrl) {
		return jsonpbhelper.UnmarshalCode(jsonPath, in, rm)
	}
	return nil, fmt.Errorf("unsupported FHIR primitive type: %v", d.Name())
}

func protoMessage(pb protoreflect.Message) proto.Message { return pb.Interface().(proto.Message) }

func protoName(pb proto.Message) protoreflect.Name {
	return proto.MessageReflect(pb).Descriptor().Name()
}
