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
	"strings"

	"github.com/google/fhir/go/fhirversion"
	"github.com/google/fhir/go/jsonformat/internal/accessor"
	"github.com/google/fhir/go/jsonformat/internal/jsonpbhelper"
	"golang.org/x/exp/maps"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/reflect/protoreflect"

	anypb "google.golang.org/protobuf/types/known/anypb"
	apb "github.com/google/fhir/go/proto/google/fhir/proto/annotations_go_proto"
)

// jsonFormat is the format in which the marshaller will represent the FHIR
// proto in JSON form.
type jsonFormat int

// ExtensionError is an error that occurs when the extension is invalid for
// the marshaller, such as when an extension has empty URL value.
// TODO(b/197531657): Use ErrorReporter for more robust special case handling.
type ExtensionError struct {
	err string
}

func (e *ExtensionError) Error() string {
	return e.err
}

const (
	// formatPure indicates a lossless JSON representation of FHIR proto.
	formatPure jsonFormat = iota

	// formatAnalytic indicates a lossy JSON representation with specified maximum
	// recursive depth and limited support for Extensions.
	formatAnalytic

	// formatAnalyticWithInferredSchema indicates a lossy JSON representation with
	// specified maximum recursive depth and support for extensions as first class
	// fields.
	formatAnalyticWithInferredSchema

	// formatAnalyticV2WithInferredSchema indicates a lossy JSON representation with
	// specified maximum recursive depth and support for extensions as first class
	// fields, allowing for repetitive extensions and including contained resources.
	formatAnalyticV2WithInferredSchema
)

// Marshaller is an object for serializing FHIR protocol buffer messages into a JSON object.
type Marshaller struct {
	enableIndent   bool
	prefix, indent string
	jsonFormat     jsonFormat
	maxDepth       int
	depths         map[string]int
	cfg            config
	// If true, the resourceType field will be populated in the output JSON.
	// This is enabled for the pure format and contained resources in AnalyticsV2.
	includeResourceType bool
}

// NewMarshaller returns a Marshaller.
func NewMarshaller(enableIndent bool, prefix, indent string, ver fhirversion.Version) (*Marshaller, error) {
	cfg, err := getConfig(ver)
	if err != nil {
		return nil, err
	}
	return &Marshaller{
		enableIndent:        enableIndent,
		prefix:              prefix,
		jsonFormat:          formatPure,
		indent:              indent,
		cfg:                 cfg,
		includeResourceType: true,
	}, nil
}

// NewPrettyMarshaller returns a pretty Marshaller.
func NewPrettyMarshaller(ver fhirversion.Version) (*Marshaller, error) {
	return NewMarshaller(true, "", "  ", ver)
}

// NewAnalyticsMarshaller returns an Analytics Marshaller with limited support
// for extensions. A default maxDepth of 2 will be used if the input is 0.
func NewAnalyticsMarshaller(maxDepth int, ver fhirversion.Version) (*Marshaller, error) {
	return newAnalyticsMarshaller(maxDepth, ver, formatAnalytic)
}

// NewAnalyticsMarshallerWithInferredSchema returns an Analytics Marshaller with
// support for extensions as first class fields. A default maxDepth of 2 will be
// used if the input is 0.
func NewAnalyticsMarshallerWithInferredSchema(maxDepth int, ver fhirversion.Version) (*Marshaller, error) {
	return newAnalyticsMarshaller(maxDepth, ver, formatAnalyticWithInferredSchema)
}

// NewAnalyticsV2MarshallerWithInferredSchema returns an Analytics Marshaller with
// support for extensions as first class fields. A default maxDepth of 2 will be
// used if the input is 0.
func NewAnalyticsV2MarshallerWithInferredSchema(maxDepth int, ver fhirversion.Version) (*Marshaller, error) {
	return newAnalyticsMarshaller(maxDepth, ver, formatAnalyticV2WithInferredSchema)
}

func newAnalyticsMarshaller(maxDepth int, ver fhirversion.Version, format jsonFormat) (*Marshaller, error) {
	if maxDepth == 0 {
		maxDepth = jsonpbhelper.DefaultAnalyticsRecurExpansionDepth
	}
	cfg, err := getConfig(ver)
	if err != nil {
		return nil, err
	}
	return &Marshaller{
		enableIndent: false,
		jsonFormat:   format,
		maxDepth:     maxDepth,
		depths:       map[string]int{},
		cfg:          cfg,
	}, nil
}

func (m *Marshaller) clone() *Marshaller {
	return &Marshaller{
		enableIndent:        m.enableIndent,
		prefix:              m.prefix,
		indent:              m.indent,
		jsonFormat:          m.jsonFormat,
		maxDepth:            m.maxDepth,
		depths:              maps.Clone(m.depths),
		cfg:                 m.cfg,
		includeResourceType: m.includeResourceType,
	}
}

// MarshalToString returns serialized JSON object of a ContainedResource protobuf message as string.
func (m *Marshaller) MarshalToString(pb proto.Message) (string, error) {
	pbTypeName := pb.ProtoReflect().Descriptor().FullName()
	emptyCR := m.cfg.newEmptyContainedResource()
	expTypeName := emptyCR.ProtoReflect().Descriptor().FullName()
	if pbTypeName != expTypeName {
		return "", fmt.Errorf("type mismatch, given proto is a message of type: %v, marshaller expects message of type: %v", pbTypeName, expTypeName)
	}
	res, err := m.Marshal(pb)
	return string(res), err
}

// MarshalResourceToString functions identically to MarshalToString, but accepts
// a fhir.Resource interface instead of a ContainedResource. See
// MarshalResource() for rationale.
func (m *Marshaller) MarshalResourceToString(r proto.Message) (string, error) {
	res, err := m.MarshalResource(r)
	return string(res), err
}

// Marshal returns serialized JSON object of a ContainedResource protobuf message.
func (m *Marshaller) Marshal(pb proto.Message) ([]byte, error) {
	pbTypeName := pb.ProtoReflect().Descriptor().FullName()
	emptyCR := m.cfg.newEmptyContainedResource()
	expTypeName := emptyCR.ProtoReflect().Descriptor().FullName()
	if pbTypeName != expTypeName {
		return nil, fmt.Errorf("type mismatch, given proto is a message of type: %v, marshaller expects message of type: %v", pbTypeName, expTypeName)
	}
	data, err := m.marshal(pb.ProtoReflect())
	if err != nil {
		return nil, err
	}
	return m.render(data)
}

func (m *Marshaller) render(data jsonpbhelper.IsJSON) ([]byte, error) {
	// We continue to use json instead of jsoniter for serialization because jsoniter has a bug in
	// how it creates streams from its shared pool. The consequence of this is that indentation gets
	// reset at every level.
	buf := bytes.Buffer{}
	enc := json.NewEncoder(&buf)
	enc.SetEscapeHTML(false)
	if m.enableIndent {
		enc.SetIndent(m.prefix, m.indent)
	}
	if err := enc.Encode(data); err != nil {
		return nil, err
	}
	// Encode seems to always have a trailing newline.
	return bytes.TrimSuffix(buf.Bytes(), []byte("\n")), nil
}

// MarshalResource functions identically to Marshal, but accepts a fhir.Resource
// interface instead of a ContainedResource. This allows for reduced nesting in
// declaring messages, and does not require knowledge of the specific Resource
// type.
func (m *Marshaller) MarshalResource(r proto.Message) ([]byte, error) {
	data, err := m.marshalResource(r.ProtoReflect())
	if err != nil {
		return nil, err
	}
	return m.render(data)
}

// Marshal returns JSON serialization of a ContainedResource protobuf message.
func (m *Marshaller) marshal(pb protoreflect.Message) (jsonpbhelper.JSONObject, error) {
	pbdesc := pb.Descriptor()
	if pbdesc.Name() != containedResourceProtoName(m.cfg) {
		return nil, fmt.Errorf("unexpected resource type: %v", pbdesc.Name())
	}
	od := pb.Descriptor().Oneofs().ByName(jsonpbhelper.OneofName)
	if od == nil {
		return nil, fmt.Errorf("no field is set in the oneof")
	}
	resourceField := pb.WhichOneof(od)
	if resourceField == nil {
		return nil, fmt.Errorf("no field is set in the oneof")
	}
	if resourceField.Message() == nil {
		return nil, fmt.Errorf("unexpected oneof field kind: %v", resourceField.Kind())
	}
	return m.marshalResource(pb.Get(resourceField).Message())
}

func (m *Marshaller) marshalResource(pb protoreflect.Message) (jsonpbhelper.JSONObject, error) {
	decmap, err := m.marshalMessageToMap(pb)
	if err != nil {
		return nil, err
	}
	if m.includeResourceType {
		decmap[jsonpbhelper.ResourceTypeField] = jsonpbhelper.JSONString(string(pb.Descriptor().Name()))
	}
	return decmap, nil
}

// MarshalToJSONObject returns the resource message as a JSON object, instead of marshalling the JSON data to a []byte.
// This can be useful if you need to modify the marshalled JSON data without needing to re-decode it.
func (m *Marshaller) MarshalToJSONObject(pb proto.Message) (jsonpbhelper.JSONObject, error) {
	return m.marshal(pb.ProtoReflect())
}

// MarshalElement marshals any FHIR complex value to JSON.
func (m *Marshaller) MarshalElement(pb proto.Message) ([]byte, error) {
	obj, err := m.marshalMessageToMap(pb.ProtoReflect())
	if err != nil {
		return nil, err
	}
	return m.render(obj)
}

func (m *Marshaller) marshalRepeatedFieldValue(decmap jsonpbhelper.JSONObject, f protoreflect.FieldDescriptor, pbs []protoreflect.Message) error {
	fieldName := f.JSONName()
	if fieldName == jsonpbhelper.Extension {
		switch m.jsonFormat {
		case formatAnalyticWithInferredSchema:
			return m.marshalExtensionsAsFirstClassFields(decmap, pbs)
		case formatAnalyticV2WithInferredSchema:
			return m.marshalExtensionsAsFirstClassFieldsV2(decmap, jsonpbhelper.JSONObject{}, pbs)
		case formatAnalytic:
			return m.marshalExtensionsAsURLs(decmap, pbs)
		}
	}

	rms := make(jsonpbhelper.JSONArray, 0, len(pbs))
	exts := make(jsonpbhelper.JSONArray, 0, len(pbs))

	hasValue := false
	hasExtension := false
	isPrimitive := jsonpbhelper.IsPrimitiveType(f.Message())

	if !isPrimitive && m.depths != nil {
		m.depths[fieldName]++
		defer func() { m.depths[fieldName]-- }()
		if m.depths[fieldName] > m.maxDepth {
			return nil
		}
	}

	for i, pb := range pbs {
		if isPrimitive {
			rm, err := jsonpbhelper.MarshalPrimitiveType(pb)
			if err != nil {
				return err
			}
			rms = append(rms, rm)
			if rm != nil {
				hasValue = true
			}
			if m.jsonFormat != formatAnalytic {
				ext, err := m.marshalPrimitiveExtensions(pb)
				if err != nil {
					return err
				}
				exts = append(exts, ext)
				if ext != nil {
					hasExtension = true
				}
			}
		} else {
			rm, err := m.marshalNonPrimitiveFieldValue(f, pb)
			if err != nil {
				return fmt.Errorf("marshalRepeatedFieldValue %v[%v]: %w", fieldName, i, err)
			}
			rms = append(rms, rm)
			if rm != nil {
				hasValue = true
			}
		}
	}
	if hasValue {
		decmap[fieldName] = rms
	}
	if hasExtension {
		decmap["_"+fieldName] = exts
	}
	return nil
}

func (m *Marshaller) marshalExtensionsAsFirstClassFields(decmap jsonpbhelper.JSONObject, pbs []protoreflect.Message) error {
	// Loop through the extenions first to get all the field name occurrence, lowercase field name
	// is used for counting since duplicate field names are not allowed in BigQuery even if the
	// case differs.
	fieldNameOccurrence := map[string]int{}
	for _, pb := range pbs {
		urlVal, err := jsonpbhelper.ExtensionURL(pb)
		if err != nil {
			return &ExtensionError{err: err.Error()}
		}
		fieldName := jsonpbhelper.ExtensionFieldName(urlVal)
		if fieldName == "" {
			return &ExtensionError{err: fmt.Sprintf("extension field name is empty for url %q", urlVal)}
		}
		fieldNameOccurrence[strings.ToLower(fieldName)]++
	}
	for _, pb := range pbs {
		urlVal, err := jsonpbhelper.ExtensionURL(pb)
		if err != nil {
			return &ExtensionError{err: err.Error()}
		}
		fieldName := jsonpbhelper.ExtensionFieldName(urlVal)
		if _, has := decmap[fieldName]; has || fieldNameOccurrence[strings.ToLower(fieldName)] > 1 {
			// Collision with proto fields or other extensions. Switch to use full extension url.
			fieldName = jsonpbhelper.FullExtensionFieldName(urlVal)
			fieldNameOccurrence[strings.ToLower(fieldName)]++
			if _, has := decmap[fieldName]; has || fieldNameOccurrence[strings.ToLower(fieldName)] > 1 {
				// Throw an error when it still collides.
				return &ExtensionError{err: fmt.Sprintf("extension field %s ran into collision", fieldName)}
			}
		}

		rm, err := m.marshalSingleExtensionHelper(pb)
		if err != nil {
			return err
		}
		decmap[fieldName] = rm
	}
	return nil
}

func (m *Marshaller) marshalExtensionsAsFirstClassFieldsV2(decmap, valObj jsonpbhelper.JSONObject, pbs []protoreflect.Message) error {
	var ok bool
	var val jsonpbhelper.IsJSON
	if val, ok = valObj["value"]; ok {
		decmap["value"] = val
	}
	// Loop through the extenions first to get all the field name occurrence, lowercase field name
	// is used for counting since duplicate field names are not allowed in BigQuery even if the
	// case differs.
	fieldNameOccurrence := map[string]int{}
	fullFieldNameOccurrence := map[string]int{}
	for _, pb := range pbs {
		urlVal, err := jsonpbhelper.ExtensionURL(pb)
		if err != nil {
			return &ExtensionError{err: err.Error()}
		}
		fn := jsonpbhelper.ExtensionFieldName(urlVal)
		if fn == "" {
			return &ExtensionError{err: fmt.Sprintf("extension field name is empty for url %q", urlVal)}
		}
		ffn := jsonpbhelper.FullExtensionFieldName(urlVal)
		if fn == "value" && ffn == "value" && ok {
			// Throw an error when fieldname collides with existing value.
			return &ExtensionError{err: "extension field has sub-extension that collides with value field"}
		}
		fieldNameOccurrence[strings.ToLower(fn)]++
		fullFieldNameOccurrence[strings.ToLower(ffn)]++
	}

	useFullExtension := map[string]bool{}
	for _, pb := range pbs {
		urlVal, err := jsonpbhelper.ExtensionURL(pb)
		if err != nil {
			return &ExtensionError{err: err.Error()}
		}
		fn := jsonpbhelper.ExtensionFieldName(urlVal)
		ffn := jsonpbhelper.FullExtensionFieldName(urlVal)

		if fieldNameOccurrence[strings.ToLower(fn)] > fullFieldNameOccurrence[strings.ToLower(ffn)] {
			useFullExtension[fn] = true
		} else if _, has := decmap[fn]; has {
			useFullExtension[fn] = true
		} else if fn == "value" {
			useFullExtension[fn] = true
		} else {
			useFullExtension[fn] = false
		}
	}

	repExtMap := map[string]jsonpbhelper.JSONArray{}

	for _, pb := range pbs {
		urlVal, err := jsonpbhelper.ExtensionURL(pb)
		if err != nil {
			return &ExtensionError{err: err.Error()}
		}
		fn := jsonpbhelper.ExtensionFieldName(urlVal)
		occurrence := fieldNameOccurrence[strings.ToLower(fn)]
		if useFullExtension[fn] {
			fn = jsonpbhelper.FullExtensionFieldName(urlVal)
			occurrence = fullFieldNameOccurrence[strings.ToLower(fn)]
		}

		lfn := strings.ToLower(fn)

		if _, has := repExtMap[lfn]; !has {
			repExtMap[lfn] = make(jsonpbhelper.JSONArray, 0, len(pbs))
		}
		rms := repExtMap[lfn]

		rm, err := m.marshalSingleExtensionHelper(pb)
		if err != nil {
			return err
		}
		rms = append(rms, rm)
		repExtMap[lfn] = rms

		// Add to decmap if this is the last repeated extension occurrence
		if len(rms) == occurrence {
			decmap[fn] = rms
		}
	}

	return nil
}

func (m *Marshaller) marshalSingleExtensionHelper(pb protoreflect.Message) (jsonpbhelper.IsJSON, error) {
	value, err := jsonpbhelper.ExtensionValue(pb)
	if err != nil {
		return nil, &ExtensionError{err: err.Error()}
	}
	var valObj jsonpbhelper.JSONObject
	if value != nil {
		msg, err := m.marshalMessageToMap(value)
		if err != nil {
			return nil, nil
		}
		valObj = jsonpbhelper.JSONObject{"value": msg}
		if m.jsonFormat == formatAnalyticWithInferredSchema {
			return valObj, nil
		}
	}
	// Each extension element must have either a value element or a nested child extension, not both
	// marshal sub-extensions only when it does not have a value
	crf := pb.Get(pb.Descriptor().Fields().ByName(jsonpbhelper.Extension)).List()
	cpbs := make([]protoreflect.Message, 0, crf.Len())
	for i := 0; i < crf.Len(); i++ {
		cpbs = append(cpbs, crf.Get(i).Message())
	}
	cm := jsonpbhelper.JSONObject{}
	if m.jsonFormat == formatAnalyticV2WithInferredSchema {
		if err := m.marshalExtensionsAsFirstClassFieldsV2(cm, valObj, cpbs); err != nil {
			return nil, err
		}
	} else {
		if err := m.marshalExtensionsAsFirstClassFields(cm, cpbs); err != nil {
			return nil, err
		}
	}
	return cm, err
}

func (m *Marshaller) marshalExtensionsAsURLs(decmap jsonpbhelper.JSONObject, pbs []protoreflect.Message) error {
	exts := make(jsonpbhelper.JSONArray, 0, len(pbs))
	for _, pb := range pbs {
		urlVal, err := jsonpbhelper.ExtensionURL(pb)
		if err != nil {
			return err
		}
		exts = append(exts, jsonpbhelper.JSONString(urlVal))
	}
	decmap[jsonpbhelper.Extension] = exts
	return nil
}

func (m *Marshaller) marshalPrimitiveExtensions(pb protoreflect.Message) (jsonpbhelper.IsJSON, error) {
	desc := pb.Descriptor()
	decmap := jsonpbhelper.JSONObject{}
	// Omit ID fields for analytics json.
	// See https://github.com/rbrush/sql-on-fhir/blob/master/sql-on-fhir.md#id-fields-omitted.
	if m.jsonFormat == formatPure {
		// Populate ID if set.
		id := desc.Fields().ByName("id")
		if id != nil && pb.Has(id) {
			idStr, err := accessor.GetString(pb, "id", "value")
			if err != nil {
				return nil, err
			}
			decmap["id"] = jsonpbhelper.JSONString(idStr)
		} else if id == nil {
			if !m.cfg.noIDFieldTypes().Contains(string(pb.Descriptor().Name())) {
				return nil, fmt.Errorf("primitive type has no id field: %v", pb.Interface())
			}
		}
	}
	// Populate extensions if set.
	e := pb.Descriptor().Fields().ByName(jsonpbhelper.Extension)
	if e != nil {
		if err := m.marshalExtensions(pb, e, decmap); err != nil {
			return nil, err
		}
	}
	if len(decmap) > 0 {
		return decmap, nil
	}
	return nil, nil
}

func (m *Marshaller) marshalExtensions(pb protoreflect.Message, extField protoreflect.FieldDescriptor, decmap jsonpbhelper.JSONObject) error {
	rf := pb.Get(extField).List()
	if rf.Len() == 0 {
		return nil
	}
	pbs := make([]protoreflect.Message, 0, rf.Len())
	for i := 0; i < rf.Len(); i++ {
		pb := rf.Get(i).Message()
		urlVal, err := jsonpbhelper.ExtensionURL(pb)
		if err == nil && (urlVal == jsonpbhelper.PrimitiveHasNoValueURL || urlVal == jsonpbhelper.Base64BinarySeparatorStrideURL) {
			continue
		}
		pbs = append(pbs, pb)
	}
	if len(pbs) == 0 {
		return nil
	}
	sm := jsonpbhelper.JSONObject{}
	err := m.marshalRepeatedFieldValue(sm, extField, pbs)
	if err != nil {
		return err
	}
	if m.jsonFormat == formatPure {
		// Unmarshal primitive extensions to the "extension" field.
		decmap[jsonpbhelper.Extension] = sm[jsonpbhelper.Extension]
	} else if m.jsonFormat == formatAnalyticWithInferredSchema || m.jsonFormat == formatAnalyticV2WithInferredSchema {
		// Promote primitive extensions to first class fields.
		for k, v := range sm {
			decmap[k] = v
		}
	}
	return nil
}

func (m *Marshaller) marshalFieldValue(decmap jsonpbhelper.JSONObject, f protoreflect.FieldDescriptor, pb protoreflect.Message) error {
	// Historically the FHIR reference URI was mapped to the JSON name "reference", but this
	// conflict between a JSON name and protobuf name is now disallowed in some languages, so
	// we cannot rely on it in the protobuf definitions.
	var jsonName string
	if f.JSONName() == "uri" && jsonpbhelper.IsReferenceType(f.Parent().(protoreflect.MessageDescriptor)) {
		jsonName = "reference"
	} else {
		jsonName = f.JSONName()
	}
	if m.jsonFormat == formatPure {
		// for choice type fields in non-analytics output, we need to zoom into the field within oneof.
		// e.g. value.quantity changed to valueQuantity
		ict := proto.GetExtension(pb.Descriptor().Options(), apb.E_IsChoiceType).(bool)
		if ict {
			fn := f.Name()
			if pb.Descriptor().Oneofs().Len() != 1 {
				return fmt.Errorf("Choice type must have exactly one oneof: %v", f.FullName())
			}
			od := pb.Descriptor().Oneofs().Get(0)
			fd := pb.WhichOneof(od)
			if fd == nil {
				return fmt.Errorf("no oneof set in choice type %v", fn)
			}
			// Zoom into the field within oneof.
			f = fd
			jsonName = jsonpbhelper.SnakeToLowerCamel(string(fn) + "_" + jsonpbhelper.CamelToSnake(fd.JSONName()))
			pb = pb.Get(fd).Message()
		}
	}
	if jsonpbhelper.IsPrimitiveType(f.Message()) {
		base, err := jsonpbhelper.MarshalPrimitiveType(pb)
		if err != nil {
			return err
		}
		if base != nil {
			decmap[jsonName] = base
		}
		if m.jsonFormat != formatAnalytic {
			ext, err := m.marshalPrimitiveExtensions(pb)
			if err != nil {
				return err
			}
			if ext != nil {
				decmap["_"+jsonName] = ext
			}
		}
		return nil
	}
	if m.depths != nil {
		m.depths[jsonName]++
		defer func() { m.depths[jsonName]-- }()
		if m.depths[jsonName] > m.maxDepth {
			return nil
		}
	}
	rm, err := m.marshalNonPrimitiveFieldValue(f, pb)
	if err != nil {
		return err
	}
	if rm != nil {
		decmap[jsonName] = rm
	}
	return nil
}

func (m *Marshaller) marshalNonPrimitiveFieldValue(f protoreflect.FieldDescriptor, pb protoreflect.Message) (jsonpbhelper.IsJSON, error) {
	d := f.Message()
	if jsonpbhelper.IsPrimitiveType(d) {
		return nil, fmt.Errorf("unexpected primitive type field: %v", f.Name())
	}
	if d.Name() == containedResourceProtoName(m.cfg) {
		if m.jsonFormat == formatAnalyticV2WithInferredSchema {
			containedMarshaller := m.clone()
			containedMarshaller.includeResourceType = true
			str, err := containedMarshaller.MarshalToString(pb.Interface())
			if err != nil {
				return nil, err
			}
			return jsonpbhelper.JSONString(str), nil
		} else if m.jsonFormat != formatPure {
			// Contained resources are dropped for analytics output
			return nil, nil
		}
		return m.marshal(pb)
	}
	// Handle inlined resources which are wrapped in Any proto. The JSON field name must be 'contained'.
	if _, ok := pb.Interface().(*anypb.Any); ok && f.JSONName() == jsonpbhelper.ContainedField {
		if m.jsonFormat == formatAnalyticV2WithInferredSchema {
			crpb := m.cfg.newEmptyContainedResource()
			pbAny := pb.Interface().(*anypb.Any)
			if err := pbAny.UnmarshalTo(crpb); err != nil {
				return nil, fmt.Errorf("unmarshalling Any, err: %w", err)
			}
			containedMarshaller := m.clone()
			containedMarshaller.includeResourceType = true
			str, err := containedMarshaller.MarshalToString(crpb.ProtoReflect().Interface())
			if err != nil {
				return nil, err
			}
			return jsonpbhelper.JSONString(str), nil
		} else if m.jsonFormat != formatPure {
			// Contained resources are dropped for analytics output
			return nil, nil
		}
		crpb := m.cfg.newEmptyContainedResource()
		pbAny := pb.Interface().(*anypb.Any)
		if err := pbAny.UnmarshalTo(crpb); err != nil {
			return nil, fmt.Errorf("unmarshalling Any, err: %w", err)
		}
		return m.marshal(crpb.ProtoReflect())
	}

	if proto.HasExtension(d.Options(), apb.E_FhirReferenceType) {
		return m.marshalReference(pb)
	}
	return m.marshalMessageToMap(pb)
}

func (m *Marshaller) marshalReference(rpb protoreflect.Message) (jsonpbhelper.IsJSON, error) {
	// NewDenormalizedReference creates a new Reference message for marshalling but
	// doesn't transfer specific fields from the original fragment such as id and extension.
	newRefMsg, err := NewDenormalizedReference(rpb.Interface())
	if err != nil {
		return nil, err
	}
	newRef := newRefMsg.ProtoReflect()

	refOneofDesc := rpb.Descriptor().Oneofs().ByName("reference")
	if refOneofDesc != nil && m.jsonFormat == formatPure {
		if f := rpb.WhichOneof(refOneofDesc); f != nil && f.Name() == "fragment" {
			fragmentMsg := rpb.Get(f).Message()

			idField := fragmentMsg.Descriptor().Fields().ByName("id")
			if idField != nil && fragmentMsg.Has(idField) {
				newRef.Set(newRef.Descriptor().Fields().ByName("id"), fragmentMsg.Get(idField))
			}

			extField := fragmentMsg.Descriptor().Fields().ByName("extension")
			if extField != nil && fragmentMsg.Has(extField) {
				newRef.Set(newRef.Descriptor().Fields().ByName("extension"), fragmentMsg.Get(extField))
			}
		}
	}
	if m.jsonFormat != formatPure {
		if err := normalizeRelativeReferenceAndIgnoreHistory(newRefMsg); err != nil {
			return nil, err
		}
	}
	return m.marshalMessageToMap(newRef)
}

func (m *Marshaller) marshalMessageToMap(pb protoreflect.Message) (jsonpbhelper.JSONObject, error) {
	decmap := jsonpbhelper.JSONObject{}
	var err error
	pb.Range(func(f protoreflect.FieldDescriptor, val protoreflect.Value) bool {
		if f.Message() == nil {
			err = fmt.Errorf("field %v has unexpected kind %v", f.Name(), f.Kind())
			return false
		}
		if f.IsMap() {
			err = fmt.Errorf("field %v is map, which is not supported", f.Name())
		}
		switch f.Cardinality() {
		case protoreflect.Optional:
			if err = m.marshalFieldValue(decmap, f, val.Message()); err != nil {
				err = fmt.Errorf("marshalMessageToMap optional field %v: %w", f.Name(), err)
				return false
			}
		case protoreflect.Repeated:
			rf := val.List()
			pbs := make([]protoreflect.Message, 0, rf.Len())
			for i := 0; i < rf.Len(); i++ {
				pbs = append(pbs, rf.Get(i).Message())
			}
			if err = m.marshalRepeatedFieldValue(decmap, f, pbs); err != nil {
				err = fmt.Errorf("marshalMessageToMap repeated field %v: %w", f.Name(), err)
				return false
			}
		default:
			err = fmt.Errorf("field %v is neither optional nor repeated", f.Name())
			return false
		}
		return true
	})
	if err != nil {
		return nil, err
	}
	if m.jsonFormat != formatPure && !jsonpbhelper.IsResourceType(pb.Descriptor()) && !jsonpbhelper.IsChoice(pb.Descriptor()) {
		// Omit FHIR element ID fields for analytics json.
		// See https://github.com/rbrush/sql-on-fhir/blob/master/sql-on-fhir.md#id-fields-omitted.
		delete(decmap, "id")
	}
	return decmap, nil
}

// SerializeInstant takes an Instant proto message and serializes it to a datetime string.
func SerializeInstant(instant proto.Message) (string, error) {
	return jsonpbhelper.SerializeInstant(instant)
}
