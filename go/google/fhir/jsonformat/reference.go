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
	"fmt"
	"strings"

	"google/fhir/jsonformat/internal/accessor/accessor"
	"google/fhir/jsonformat/internal/jsonpbhelper/jsonpbhelper"
	"github.com/golang/protobuf/proto"
	"google.golang.org/protobuf/reflect/protoreflect"

	d4pb "google/fhir/proto/r4/core/datatypes_go_proto"
	d3pb "google/fhir/proto/stu3/datatypes_go_proto"
)

// normalizeFragmentReference normalizes an internal reference into its specialized field.
func normalizeFragmentReference(pb proto.Message) error {
	rpb := proto.MessageReflect(pb)
	uriField := rpb.Descriptor().Fields().ByName(jsonpbhelper.RefRawURI)
	if uriField == nil {
		return fmt.Errorf("invalid reference: %v", pb)
	}
	if !rpb.Has(uriField) {
		// This is not an URI reference
		return nil
	}

	uriVal, err := accessor.GetString(rpb, jsonpbhelper.RefOneofName, jsonpbhelper.RefRawURI, "value")
	if err != nil {
		return fmt.Errorf("invalid reference: %v", pb)
	}
	if strings.HasPrefix(uriVal, jsonpbhelper.RefFragmentPrefix) {
		fragVal := uriVal[len(jsonpbhelper.RefFragmentPrefix):]
		if err = accessor.SetValue(rpb, fragVal, jsonpbhelper.RefFragment, "value"); err != nil {
			return err
		}
	}

	return nil
}

// normalizeRelativeReferenceAndIgnoreHistory normalizes a relative reference into its specialized
// field. If history is included in the original reference, it would be ignored.
func normalizeRelativeReferenceAndIgnoreHistory(pb proto.Message) error {
	rpb := proto.MessageReflect(pb)
	field, err := jsonpbhelper.GetOneofField(rpb.Descriptor(), jsonpbhelper.RefOneofName, jsonpbhelper.RefRawURI)
	if err != nil {
		return err
	}

	// This is not a URI reference, so it can't be normalized.
	if !rpb.Has(field) {
		return nil
	}

	ref, err := accessor.GetString(rpb, jsonpbhelper.RefOneofName, jsonpbhelper.RefRawURI, "value")
	if err != nil {
		return err
	}

	parts := strings.Split(ref, "/")
	if len(parts) == 2 || len(parts) == 4 && parts[2] == jsonpbhelper.RefHistory {
		fn := jsonpbhelper.CamelToSnake(parts[0]) + jsonpbhelper.RefFieldSuffix
		if _, err := jsonpbhelper.GetOneofField(rpb.Descriptor(), jsonpbhelper.RefOneofName, protoreflect.Name(fn)); err != nil {
			return nil
		}
		if err := accessor.SetValue(rpb, parts[1], fn, "value"); err != nil {
			return err
		}
		return nil
	}
	return nil
}

// NormalizeReference normalizes a relative or internal reference into its specialized field.
func NormalizeReference(pb proto.Message) error {
	switch ref := pb.(type) {
	case *d3pb.Reference:
		normalizeR3Reference(ref)
	case *d4pb.Reference:
		normalizeR4Reference(ref)
	default:
		return fmt.Errorf("invalid reference type %T", ref)
	}
	return nil
}

func setReferenceID(ref protoreflect.Message, resType string, refID protoreflect.Message) {
	fName, ok := jsonpbhelper.ReferenceFieldForType(resType)
	if !ok {
		return
	}

	ref.Set(ref.Descriptor().Fields().ByName(fName), protoreflect.ValueOfMessage(refID))
}

func normalizeR3Reference(ref *d3pb.Reference) {
	uri := ref.GetUri().GetValue()
	if uri == "" {
		return
	}

	if strings.HasPrefix(uri, jsonpbhelper.RefFragmentPrefix) {
		fragVal := uri[len(jsonpbhelper.RefFragmentPrefix):]
		ref.Reference = &d3pb.Reference_Fragment{Fragment: &d3pb.String{Value: fragVal}}
		return
	}
	parts := strings.Split(uri, "/")
	if !(len(parts) == 2 || len(parts) == 4 && parts[2] == jsonpbhelper.RefHistory) {
		return
	}

	refID := &d3pb.ReferenceId{Value: parts[1]}
	if len(parts) > 2 {
		refID.History = &d3pb.Id{Value: parts[3]}
	}

	setReferenceID(ref.ProtoReflect(), parts[0], refID.ProtoReflect())
}

func normalizeR4Reference(ref *d4pb.Reference) {
	uri := ref.GetUri().GetValue()
	if uri == "" {
		return
	}

	if strings.HasPrefix(uri, jsonpbhelper.RefFragmentPrefix) {
		fragVal := uri[len(jsonpbhelper.RefFragmentPrefix):]
		ref.Reference = &d4pb.Reference_Fragment{Fragment: &d4pb.String{Value: fragVal}}
		return
	}
	parts := strings.Split(uri, "/")
	if !(len(parts) == 2 || len(parts) == 4 && parts[2] == jsonpbhelper.RefHistory) {
		return
	}

	refID := &d4pb.ReferenceId{Value: parts[1]}
	if len(parts) > 2 {
		refID.History = &d4pb.Id{Value: parts[3]}
	}

	setReferenceID(ref.ProtoReflect(), parts[0], refID.ProtoReflect())
}

// DenormalizeReference recovers the absolute reference URI from a normalized representation.
func DenormalizeReference(pb proto.Message) error {
	switch ref := pb.(type) {
	case *d3pb.Reference:
		denormalizeR3Reference(ref)
	case *d4pb.Reference:
		denormalizeR4Reference(ref)
	default:
		return fmt.Errorf("invalid reference type %T", pb)
	}
	return nil
}

func denormalizeR3Reference(ref *d3pb.Reference) {
	if uri := ref.GetUri(); uri != nil {
		return
	}
	if frag := ref.GetFragment(); frag != nil {
		ref.Reference = &d3pb.Reference_Uri{Uri: &d3pb.String{Value: jsonpbhelper.RefFragmentPrefix + frag.GetValue()}}
		return
	}

	rpb := ref.ProtoReflect()
	f, err := jsonpbhelper.ResourceIDField(rpb)
	if err != nil || f == nil {
		return
	}

	refType, ok := jsonpbhelper.ResourceTypeForReference(f.Name())
	if !ok {
		return
	}

	refID, _ := rpb.Get(f).Message().Interface().(*d3pb.ReferenceId)
	parts := []string{refType, refID.GetValue()}
	if refID.GetHistory().GetValue() != "" {
		parts = append(parts, jsonpbhelper.RefHistory, refID.GetHistory().GetValue())
	}
	ref.Reference = &d3pb.Reference_Uri{Uri: &d3pb.String{Value: strings.Join(parts, "/")}}
}

func denormalizeR4Reference(ref *d4pb.Reference) {
	if uri := ref.GetUri(); uri != nil {
		return
	}
	if frag := ref.GetFragment(); frag != nil {
		ref.Reference = &d4pb.Reference_Uri{Uri: &d4pb.String{Value: jsonpbhelper.RefFragmentPrefix + frag.GetValue()}}
		return
	}

	rpb := ref.ProtoReflect()
	f, err := jsonpbhelper.ResourceIDField(rpb)
	if err != nil || f == nil {
		return
	}

	refType, ok := jsonpbhelper.ResourceTypeForReference(f.Name())
	if !ok {
		return
	}

	refID, _ := rpb.Get(f).Message().Interface().(*d4pb.ReferenceId)
	parts := []string{refType, refID.GetValue()}
	if refID.GetHistory().GetValue() != "" {
		parts = append(parts, jsonpbhelper.RefHistory, refID.GetHistory().GetValue())
	}
	ref.Reference = &d4pb.Reference_Uri{Uri: &d4pb.String{Value: strings.Join(parts, "/")}}
}
