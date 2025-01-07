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
	"io"
	"strconv"
	"strings"
	"time"
	"unicode/utf8"

	"github.com/google/fhir/go/fhirversion"
	"github.com/google/fhir/go/jsonformat/errorreporter"
	"github.com/google/fhir/go/jsonformat/fhirvalidate"
	"github.com/google/fhir/go/jsonformat/internal/accessor"
	"github.com/google/fhir/go/jsonformat/internal/jsonpbhelper"
	"github.com/json-iterator/go"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/reflect/protoreflect"

	anypb "google.golang.org/protobuf/types/known/anypb"
	apb "github.com/google/fhir/go/proto/google/fhir/proto/annotations_go_proto"
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
	MaxNestingDepth int
	// Stores whether extended validation checks like required fields and
	// reference checking should be run.
	enableExtendedValidation bool
	cfg                      config
	ver                      fhirversion.Version
}

// NewUnmarshaller returns an Unmarshaller that performs resource validation.
func NewUnmarshaller(tz string, ver fhirversion.Version) (*Unmarshaller, error) {
	return newUnmarshaller(tz, ver, true /*enableExtendedValidation*/)
}

// NewUnmarshallerWithoutValidation returns an Unmarshaller that doesn't perform resource validation.
func NewUnmarshallerWithoutValidation(tz string, ver fhirversion.Version) (*Unmarshaller, error) {
	return newUnmarshaller(tz, ver, false /*enableExtendedValidation*/)
}

func newUnmarshaller(tz string, ver fhirversion.Version, enableExtendedValidation bool) (*Unmarshaller, error) {
	cfg, err := getConfig(ver)
	if err != nil {
		return nil, err
	}
	l, err := time.LoadLocation(tz)
	if err != nil {
		return nil, err
	}

	return &Unmarshaller{
		TimeZone:                 l,
		cfg:                      cfg,
		enableExtendedValidation: enableExtendedValidation,
		ver:                      ver,
	}, nil
}

// Unmarshal a FHIR resource from JSON into a ContainedResource proto. The FHIR
// version of the proto is determined by the version the Unmarshaller was
// created with.
func (u *Unmarshaller) Unmarshal(in []byte, opts ...fhirvalidate.ValidationOption) (proto.Message, error) {
	var umErrList jsonpbhelper.UnmarshalErrorList
	er := errorreporter.NewBasicErrorReporter()
	res, err := u.UnmarshalWithErrorReporter(in, er, opts...)
	if err != nil {
		return res, err
	}
	for _, error := range er.Errors {
		if err := jsonpbhelper.AppendUnmarshalError(&umErrList, *error); err != nil {
			return res, err
		}
	}
	if len(umErrList) > 0 {
		return res, umErrList
	}
	return res, nil
}

// UnmarshalWithErrorReporter unmarshals a FHIR resource from JSON []byte data into a
// ContainedResource proto. During the process, the validation errors are
// reported according to user defined error reporter.
// The FHIR version of the proto is determined by the version the Unmarshaller was
// created with.
func (u *Unmarshaller) UnmarshalWithErrorReporter(in []byte, er errorreporter.ErrorReporter, opts ...fhirvalidate.ValidationOption) (proto.Message, error) {
	var decoded map[string]json.RawMessage
	if err := jsp.Unmarshal(in, &decoded); err != nil {
		return nil, &jsonpbhelper.UnmarshalError{
			Details:     "invalid JSON",
			Diagnostics: err.Error(),
			Cause:       err,
		}
	}
	return u.unmarshalJSONObject(decoded, er, opts...)
}

func readFullResource(in io.Reader) (map[string]json.RawMessage, error) {
	var decoded map[string]json.RawMessage
	d := jsp.NewDecoder(in)
	if err := d.Decode(&decoded); err != nil {
		return nil, err
	}
	// Emulate the same behaviour as json.Unmarshal where extra bytes cause an error.
	if d.More() {
		b := make([]byte, 1)
		_, err := io.ReadFull(d.Buffered(), b)
		// Should be impossible since the underlying reader is a bytes.Reader, but that could change.
		if err != nil {
			return nil, err
		}
		return nil, fmt.Errorf("invalid character '%v' after top-level value", b[0])
	}
	return decoded, nil
}

// UnmarshalFromReaderWithErrorReporter read FHIR data as JSON from an io.Reader and unmarshals it into a
// ContainedResource proto. During the process, the validation errors are
// reported according to user defined error reporter.
// The FHIR version of the proto is determined by the version the Unmarshaller was
// created with.
func (u *Unmarshaller) UnmarshalFromReaderWithErrorReporter(in io.Reader, er errorreporter.ErrorReporter) (proto.Message, error) {
	// TODO(b/244184211): report parseContainedResource error with error reporter
	// Decode the JSON object into a map.
	decoded, err := readFullResource(in)
	if err != nil {
		return nil, &jsonpbhelper.UnmarshalError{
			Details:     "invalid JSON",
			Diagnostics: err.Error(),
			Cause:       err,
		}
	}
	return u.unmarshalJSONObject(decoded, er)
}

func (u *Unmarshaller) unmarshalJSONObject(decoded map[string]json.RawMessage, er errorreporter.ErrorReporter, opts ...fhirvalidate.ValidationOption) (proto.Message, error) {
	res, err := u.parseContainedResource("", decoded)
	if err != nil {
		return res, err
	}
	if u.enableExtendedValidation {
		if err := fhirvalidate.ValidateWithErrorReporter(res, er); err != nil {
			return res, err
		}
	} else if err := fhirvalidate.ValidatePrimitivesWithErrorReporter(res, er); err != nil {
		return res, err
	}
	return res, nil
}

// UnmarshalWithOutcome unmarshalls a FHIR resource from JSON into a ContainedResource
// proto. During the process, the validation errors are reported according to the predefined
// OperationErrorReporter.
// The FHIR version of the proto is determined by the version the Unmarshaller was
// created with.
func (u *Unmarshaller) UnmarshalWithOutcome(in []byte) (proto.Message, *errorreporter.MultiVersionOperationOutcome, error) {
	er := errorreporter.NewOperationErrorReporter(u.ver)
	res, err := u.UnmarshalWithErrorReporter(in, er)
	if err != nil {
		return res, nil, err
	}
	return res, er.Outcome, nil
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

func (u *Unmarshaller) parseContainedResource(jsonPath string, decmap map[string]json.RawMessage) (proto.Message, error) {
	var errors jsonpbhelper.UnmarshalErrorList
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
	if jsonPath != "" {
		jsonPath = jsonpbhelper.AddFieldToPath(jsonPath, fmt.Sprintf("ofType(%s)", rtstr))
	} else {
		jsonPath = jsonpbhelper.AddFieldToPath(jsonPath, rtstr)
	}

	// Populate the fields in the protobuf message to return.
	// Encapsulate in a ContainedResource.
	cr := u.cfg.newEmptyContainedResource()
	rcr := cr.ProtoReflect()
	pbdesc := rcr.Descriptor()
	oneofDesc := pbdesc.Oneofs().ByName(jsonpbhelper.OneofName)
	if oneofDesc == nil {
		return nil, fmt.Errorf("oneof field not found: %v", jsonpbhelper.OneofName)
	}
	for i := 0; i < oneofDesc.Fields().Len(); i++ {
		f := oneofDesc.Fields().Get(i)
		if f.Message() != nil && string(f.Message().Name()) == rtstr {
			if err := u.mergeMessage(jsonPath, decmap, rcr.Mutable(f).Message()); err != nil {
				if err := jsonpbhelper.AppendUnmarshalError(&errors, err); err != nil {
					return nil, err
				}
			}
			if len(errors) > 0 {
				return nil, errors
			}
			return cr, nil
		}
	}
	return nil, append(errors, &jsonpbhelper.UnmarshalError{
		Path:        jsonPath,
		Details:     fmt.Sprintf("unknown resource type"),
		Diagnostics: strconv.Quote(rtstr),
	})
}

func (u *Unmarshaller) mergeRawMessage(jsonPath string, rm json.RawMessage, pb protoreflect.Message) error {
	var decmap map[string]json.RawMessage
	if err := jsp.Unmarshal(rm, &decmap); err != nil {
		return &jsonpbhelper.UnmarshalError{
			Path:        jsonPath,
			Details:     fmt.Sprintf("invalid value (expected a %s object)", pb.Descriptor().Name()),
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
		proto.Merge(pb.Interface(), cr)
		return nil
	}
	if pbdesc.Name() == protoName(&anypb.Any{}) && lastFieldInPath(jsonPath) == jsonpbhelper.ContainedField {
		// Special handling of inlined resources, with 'contained' JSON field name and Any proto type.
		cr, err := u.parseContainedResource(jsonPath, decmap)
		if err != nil {
			return err
		}
		any := &anypb.Any{}
		if err := any.MarshalFrom(cr); err != nil {
			return err
		}
		proto.Merge(pb.Interface(), any)
		return nil
	}
	var errors jsonpbhelper.UnmarshalErrorList
	fieldMap := jsonpbhelper.FieldMap(pbdesc)
	// Iterate through all fields, and merge to the proto.
	for k, v := range decmap {
		if u.cfg.keysToSkip().Contains(k) {
			continue
		}

		// TODO(b/161479338): reject upper camel case fields names after suitable deprecation warning.
		var normalizedFieldName string
		if strings.HasPrefix(k, "_") {
			normalizedFieldName = "_" + lowerFirst(k[1:])
		} else {
			normalizedFieldName = lowerFirst(k)
		}

		f, ok := fieldMap[normalizedFieldName]
		if !ok {
			errors = append(errors, &jsonpbhelper.UnmarshalError{
				Path:        jsonPath,
				Details:     "unknown field",
				Diagnostics: strconv.Quote(k),
			})
			continue
		}
		if jsonpbhelper.IsChoice(f.Message()) {
			if err := u.mergeChoiceField(jsonPath, f, k, v, pb); err != nil {
				if err := jsonpbhelper.AppendUnmarshalError(&errors, err); err != nil {
					return err
				}
				continue
			}
		} else if err := u.mergeField(jsonpbhelper.AddFieldToPath(jsonPath, k), f, v, pb); err != nil {
			if err := jsonpbhelper.AppendUnmarshalError(&errors, err); err != nil {
				return err
			}
			continue
		}
	}
	if len(errors) > 0 {
		return errors
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

	// TODO(b/161479338): reject upper camel case fields names after suitable deprecation warning.
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

	choice := pb.Get(f).Message().WhichOneof(choiceField.ContainingOneof())
	if choice != nil && choiceField.Name() != choice.Name() {
		return &jsonpbhelper.UnmarshalError{
			Path:        jsonPath,
			Details:     fmt.Sprintf("cannot accept multiple values for %s field", string(f.Name())),
			Diagnostics: strconv.Quote(k),
		}
	}

	return u.mergeField(jsonpbhelper.AddFieldToPath(jsonPath, k), choiceField, v, pb.Mutable(f).Message())
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
			return u.mergePrimitiveType(pb.Mutable(f).Message().Interface(), p)
		}
		// f is expected to be a singular message (i.e. mutable), otherwise it is trying to merge
		// an invalid field such as primitive type's "value" field here.
		if f.Message() == nil {
			return &jsonpbhelper.UnmarshalError{
				Path:    jsonPath,
				Details: "invalid field",
			}
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

func (u *Unmarshaller) mergeRepeatedField(jsonPath string, fd protoreflect.FieldDescriptor, sourceElems []json.RawMessage, targetMsg protoreflect.Message) error {
	targetList := targetMsg.Mutable(fd).List()
	if !(targetList.Len() == 0 || targetList.Len() == len(sourceElems)) {
		return &jsonpbhelper.UnmarshalError{
			Path:    jsonPath,
			Details: fmt.Sprintf("array length mismatch, expected %d, found %d", targetList.Len(), len(sourceElems)),
		}
	}

	var errors jsonpbhelper.UnmarshalErrorList
	fill := targetList.Len() == 0
	for i, sourceElem := range sourceElems {
		var targetElem protoreflect.Message
		if fill {
			targetElem = targetList.AppendMutable().Message()
		} else {
			targetElem = targetList.Get(i).Message()
		}
		if err := u.mergeSingleField(jsonpbhelper.AddIndexToPath(jsonPath, i), fd, sourceElem, targetElem); err != nil {
			if err := jsonpbhelper.AppendUnmarshalError(&errors, err); err != nil {
				return err
			}
			continue
		}
	}
	if len(errors) > 0 {
		return errors
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
		return u.mergePrimitiveType(pb.Interface(), p)
	}
	if !proto.HasExtension(d.Options(), apb.E_FhirReferenceType) {
		return u.mergeRawMessage(jsonPath, rm, pb)
	}

	return u.mergeReference(jsonPath, rm, pb)
}

func (u *Unmarshaller) mergeReference(jsonPath string, rm json.RawMessage, pb protoreflect.Message) error {
	if err := u.mergeRawMessage(jsonPath, rm, pb); err != nil {
		return err
	}
	if err := NormalizeReference(pb.Interface()); err != nil {
		return &jsonpbhelper.UnmarshalError{
			Path:        jsonPath,
			Details:     "invalid reference",
			Diagnostics: err.Error(),
		}
	}
	return nil
}

func (u *Unmarshaller) mergePrimitiveType(dst, src proto.Message) error {
	if proto.Size(src) == 0 {
		// No merging necessary.
		return nil
	}
	if proto.Size(dst) == 0 {
		return mergePrimitive(dst, src)
	}

	dnv := jsonpbhelper.HasExtension(dst, jsonpbhelper.PrimitiveHasNoValueURL)
	if dnv {
		if err := jsonpbhelper.RemoveExtension(dst, jsonpbhelper.PrimitiveHasNoValueURL); err != nil {
			return err
		}
	}

	snv := jsonpbhelper.HasExtension(src, jsonpbhelper.PrimitiveHasNoValueURL)
	if err := mergePrimitive(dst, src); err != nil {
		return err
	}
	if !dnv && snv {
		// Remove the HasNoValue extension if dst actually has value.
		if err := jsonpbhelper.RemoveExtension(dst, jsonpbhelper.PrimitiveHasNoValueURL); err != nil {
			return err
		}
	}

	return nil
}

func mergePrimitive(dst, src proto.Message) error {
	dm := dst.ProtoReflect()
	sm := src.ProtoReflect()
	if dm.Type() != sm.Type() {
		return fmt.Errorf("cannot merge proto of type %s into %s", sm.Type().Descriptor().Name(), dm.Type().Descriptor().Name())
	}
	// In the case where we don't have any extensions we can simply copy the fields into dst to save allocations.
	sm.Range(func(fd protoreflect.FieldDescriptor, v protoreflect.Value) bool {
		if !dm.Has(fd) {
			dm.Set(fd, v)
			// Clear the field so the merge doesn't undo the savings here.
			sm.Clear(fd)
		}
		return true
	})
	// Fallback to merge functionality for the more complicated cases.
	proto.Merge(dst, src)
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
		ext := extListInPb.NewElement().Message().Interface()
		if err := jsonpbhelper.ProtoToExtension(u.cfg.newPrimitiveHasNoValue(true), ext); err != nil {
			return nil, err
		}
		if err := jsonpbhelper.AddInternalExtension(pb.Interface(), ext); err != nil {
			return nil, err
		}
		return pb.Interface(), nil
	}
	d := in.Descriptor()
	createAndSetValue := func(val interface{}) (proto.Message, error) {
		rpb := in.New()
		if err := accessor.SetValue(rpb, val, "value"); err != nil {
			return nil, err
		}
		return rpb.Interface(), nil
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
		m := in.New().Interface()
		if err := jsonpbhelper.ParseBinary(rm, m, u.cfg.newBase64BinarySeparatorStride); err != nil {
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
		return createAndSetValue(val)
	case "Date":
		var date string
		if err := jsonpbhelper.JSP.Unmarshal(rm, &date); err != nil {
			return nil, &jsonpbhelper.UnmarshalError{
				Path:        jsonPath,
				Details:     "expected date",
				Diagnostics: fmt.Sprintf("found %s", rm),
			}
		}
		m := in.New().Interface()
		if err := jsonpbhelper.ParseDateFromString(date, u.TimeZone, m); err != nil {
			return nil, &jsonpbhelper.UnmarshalError{
				Path:        jsonPath,
				Details:     "expected date",
				Diagnostics: fmt.Sprintf("found %s", rm),
			}
		}
		return m, nil
	case "DateTime":
		var date string
		if err := jsonpbhelper.JSP.Unmarshal(rm, &date); err != nil {
			return nil, &jsonpbhelper.UnmarshalError{
				Path:        jsonPath,
				Details:     "expected datetime",
				Diagnostics: fmt.Sprintf("found %s", rm),
			}
		}
		m := in.New().Interface()
		if err := jsonpbhelper.ParseDateTimeFromString(date, u.TimeZone, m); err != nil {
			return nil, &jsonpbhelper.UnmarshalError{
				Path:        jsonPath,
				Details:     "expected datetime",
				Diagnostics: fmt.Sprintf("found %s", rm),
			}
		}
		return m, nil
	case "Decimal":
		m := in.New().Interface()
		if err := jsonpbhelper.ParseDecimal(rm, m); err != nil {
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
		return createAndSetValue(val)
	case "Instant":
		m := in.New().Interface()
		if err := jsonpbhelper.ParseInstant(rm, m); err != nil {
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
	case "Integer64":
		var val int64
		if err := jsp.Unmarshal(rm, &val); err != nil {
			return nil, &jsonpbhelper.UnmarshalError{
				Path:        jsonPath,
				Details:     "expected integer64",
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
		if err := jsp.Unmarshal(rm, &val); err != nil {
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
		return createAndSetValue(val)
	case "Time":
		m := in.New().Interface()
		if err := jsonpbhelper.ParseTime(rm, m); err != nil {
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
		if err := jsp.Unmarshal(rm, &val); err != nil {
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
		return createAndSetValue(val)
	case "ReferenceId":
		return nil, &jsonpbhelper.UnmarshalError{
			Path:    jsonPath,
			Details: fmt.Sprintf("invalid type: %v", d.Name()),
		}
	}

	// Handles specialized codes.
	if proto.HasExtension(d.Options(), apb.E_FhirValuesetUrl) {
		return jsonpbhelper.UnmarshalCode(jsonPath, in, rm)
	}
	return nil, fmt.Errorf("unsupported FHIR primitive type: %v", d.Name())
}

func protoName(pb proto.Message) protoreflect.Name {
	return pb.ProtoReflect().Descriptor().Name()
}
