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

	"github.com/google/fhir/go/jsonformat/internal/accessor"
	"github.com/google/fhir/go/jsonformat/internal/jsonpbhelper"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/reflect/protoreflect"

	d4pb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/datatypes_go_proto"
	d5pb "github.com/google/fhir/go/proto/google/fhir/proto/r5/core/datatypes_go_proto"
	d3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/datatypes_go_proto"
)

// normalizeFragmentReference normalizes an internal reference into its specialized field.
func normalizeFragmentReference(pb proto.Message) error {
	rpb := pb.ProtoReflect()
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
	rpb := pb.ProtoReflect()
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
		return normalizeR3Reference(ref)
	case *d4pb.Reference:
		return normalizeR4Reference(ref)
	default:
		return fmt.Errorf("invalid reference type %T", ref)
	}
}

func setReferenceID(ref protoreflect.Message, resType string, refID protoreflect.Message) error {
	fName, ok := jsonpbhelper.ReferenceFieldForType(resType)
	if !ok {
		// The assumed resource type is not a proper FHIR reference resource type. This is OK as
		// callers may make incorrect assumption from random reference URIs.
		return nil
	}
	rr := ref.Descriptor().Fields().ByName(fName)
	if rr == nil {
		// This is an error because the reference resource type cannot be found. This is likely due
		// to referencing to a FHIR resource type that only exists in other FHIR versions.
		return fmt.Errorf("unsupported reference field %s", fName)
	}
	ref.Set(rr, protoreflect.ValueOfMessage(refID))
	return nil
}

func normalizeR3Reference(ref *d3pb.Reference) error {
	uri := ref.GetUri().GetValue()
	if uri == "" {
		return nil
	}

	if strings.HasPrefix(uri, jsonpbhelper.RefFragmentPrefix) {
		fragVal := uri[len(jsonpbhelper.RefFragmentPrefix):]
		ref.Reference = &d3pb.Reference_Fragment{Fragment: &d3pb.String{Value: fragVal}}
		return nil
	}
	parts := strings.Split(uri, "/")
	if !(len(parts) == 2 || len(parts) == 4 && parts[2] == jsonpbhelper.RefHistory) {
		return nil
	}

	refID := &d3pb.ReferenceId{Value: parts[1]}
	if len(parts) > 2 {
		refID.History = &d3pb.Id{Value: parts[3]}
	}

	return setReferenceID(ref.ProtoReflect(), parts[0], refID.ProtoReflect())
}

func normalizeR4Reference(ref *d4pb.Reference) error {
	uri := ref.GetUri().GetValue()
	if uri == "" {
		return nil
	}

	if strings.HasPrefix(uri, jsonpbhelper.RefFragmentPrefix) {
		fragVal := uri[len(jsonpbhelper.RefFragmentPrefix):]
		ref.Reference = &d4pb.Reference_Fragment{Fragment: &d4pb.String{Value: fragVal}}
		return nil
	}
	parts := strings.Split(uri, "/")
	if !(len(parts) == 2 || len(parts) == 4 && parts[2] == jsonpbhelper.RefHistory) {
		return nil
	}

	refID := &d4pb.ReferenceId{Value: parts[1]}
	if len(parts) > 2 {
		refID.History = &d4pb.Id{Value: parts[3]}
	}

	return setReferenceID(ref.ProtoReflect(), parts[0], refID.ProtoReflect())
}

func normalizeR5Reference(ref *d5pb.Reference) error {
	uri := ref.GetUri().GetValue()
	if uri == "" {
		return nil
	}

	if strings.HasPrefix(uri, jsonpbhelper.RefFragmentPrefix) {
		fragVal := uri[len(jsonpbhelper.RefFragmentPrefix):]
		ref.Reference = &d5pb.Reference_Fragment{Fragment: &d5pb.String{Value: fragVal}}
		return nil
	}
	parts := strings.Split(uri, "/")
	if !(len(parts) == 2 || len(parts) == 4 && parts[2] == jsonpbhelper.RefHistory) {
		return nil
	}

	refID := &d5pb.ReferenceId{Value: parts[1]}
	if len(parts) > 2 {
		refID.History = &d5pb.Id{Value: parts[3]}
	}

	return setReferenceID(ref.ProtoReflect(), parts[0], refID.ProtoReflect())
}

// DenormalizeReference recovers the absolute reference URI from a normalized representation.
func DenormalizeReference(pb proto.Message) error {
	switch ref := pb.(type) {
	case *d3pb.Reference:
		denormalizeR3Reference(ref)
	case *d4pb.Reference:
		denormalizeR4Reference(ref)
	case *d5pb.Reference:
		denormalizeR5Reference(ref)
	default:
		return fmt.Errorf("invalid reference type %T", pb)
	}
	return nil
}

// NewDenormalizedReference creates a new reference with a URI from a normalized representation.
func NewDenormalizedReference(pb proto.Message) (proto.Message, error) {
	var newRef proto.Message
	switch ref := pb.(type) {
	case *d3pb.Reference:
		newRef = copyR3Reference(ref)
	case *d4pb.Reference:
		newRef = copyR4Reference(ref)
	case *d5pb.Reference:
		newRef = copyR5Reference(ref)
	default:
		return nil, fmt.Errorf("invalid reference type %T", pb)
	}
	if err := DenormalizeReference(newRef); err != nil {
		return nil, err
	}
	return newRef, nil
}

func copyR3Reference(ref *d3pb.Reference) proto.Message {
	return &d3pb.Reference{
		Id:         ref.GetId(),
		Extension:  ref.GetExtension(),
		Identifier: ref.GetIdentifier(),
		Display:    ref.GetDisplay(),
		Reference:  ref.GetReference(),
	}
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
	return
}

func copyR4Reference(ref *d4pb.Reference) proto.Message {
	return &d4pb.Reference{
		Id:         ref.GetId(),
		Extension:  ref.GetExtension(),
		Type:       ref.GetType(),
		Identifier: ref.GetIdentifier(),
		Display:    ref.GetDisplay(),
		Reference:  ref.GetReference(),
	}
}

func copyR5Reference(ref *d5pb.Reference) proto.Message {
	return &d5pb.Reference{
		Id:         ref.GetId(),
		Extension:  ref.GetExtension(),
		Type:       ref.GetType(),
		Identifier: ref.GetIdentifier(),
		Display:    ref.GetDisplay(),
		Reference:  ref.GetReference(),
	}
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
	return
}
