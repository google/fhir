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

// Package fhirvalidate performs additional validations on FHIR protos according
// to rules in the FHIR spec.
//
// This includes regexes for string-based types, bounds checking for integers,
// required fields and enforcing reference typings.
package fhirvalidate

import (
	"fmt"
	"math"
	"net/url"
	"strings"

	"github.com/google/fhir/go/jsonformat/errorreporter"
	"github.com/google/fhir/go/jsonformat/internal/jsonpbhelper"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/reflect/protoreflect"
	"bitbucket.org/creachadair/stringset"

	apb "github.com/google/fhir/go/proto/google/fhir/proto/annotations_go_proto"
	d4pb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/datatypes_go_proto"
	d3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/datatypes_go_proto"
)

var (
	stringMessageNames = collectDescriptorNames(
		&d3pb.String{}, &d4pb.String{})
	unsignedIntMessageNames = collectDescriptorNames(
		&d3pb.UnsignedInt{}, &d4pb.UnsignedInt{})
	positiveIntMessageNames = collectDescriptorNames(
		&d3pb.PositiveInt{}, &d4pb.PositiveInt{})
	intMessageNames = unsignedIntMessageNames.Union(positiveIntMessageNames)
	oidMessageNames = collectDescriptorNames(
		&d3pb.Oid{}, &d4pb.Oid{})
	idMessageNames = collectDescriptorNames(
		&d3pb.Id{}, &d4pb.Id{})
	uuidMessageNames = collectDescriptorNames(
		&d3pb.Uuid{}, &d4pb.Uuid{})
	codeMessageNames = collectDescriptorNames(
		&d3pb.Code{}, &d4pb.Code{})
	stringRegexMessageNames = oidMessageNames.Union(idMessageNames).Union(uuidMessageNames).Union(codeMessageNames)
	urlMessageNames         = collectDescriptorNames(
		&d3pb.Uri{}, &d4pb.Uri{}, &d4pb.Url{}, &d4pb.Canonical{})
	validatedTypes stringset.Set
)

func init() {
	for _, names := range []stringset.Set{
		stringMessageNames,
		intMessageNames,
		stringRegexMessageNames,
		urlMessageNames,
	} {
		validatedTypes = validatedTypes.Union(names)
	}
}

func collectDescriptorNames(msgs ...proto.Message) stringset.Set {
	names := stringset.New()
	for _, msg := range msgs {
		names.Add(string(msg.ProtoReflect().Descriptor().FullName()))
	}
	return names
}

type validationStep func(fd protoreflect.FieldDescriptor, msg protoreflect.Message) error

type validationStepWithErrorReporter func(fd protoreflect.FieldDescriptor, msg protoreflect.Message, jsonPath string, er errorreporter.ErrorReporter) error

func validateRequiredFields(_ protoreflect.FieldDescriptor, msg protoreflect.Message) error {
	return jsonpbhelper.ValidateRequiredFields(msg)
}

func validateRequiredFieldsWithErrorReporter(fd protoreflect.FieldDescriptor, msg protoreflect.Message, jsonPath string, errorReporter errorreporter.ErrorReporter) error {
	var errors jsonpbhelper.UnmarshalErrorList
	if err := validateRequiredFields(fd, msg); err != nil {
		if !jsonpbhelper.IsUnmarshalError(err) {
			return err
		}
		if err := jsonpbhelper.AppendUnmarshalError(&errors, jsonpbhelper.AnnotateUnmarshalErrorWithPath(err, jsonPath)); err != nil {
			return err
		}
		for _, error := range errors {
			errorReporter.ReportValidationError(error.Path, error)
		}
	}
	return nil
}

func validateReferenceTypes(fd protoreflect.FieldDescriptor, msg protoreflect.Message) error {
	if !proto.HasExtension(msg.Descriptor().Options(), apb.E_FhirReferenceType) {
		return nil
	}
	return jsonpbhelper.ValidateReferenceType(fd, msg)
}

func validateReferenceTypesWithErrorReporter(fd protoreflect.FieldDescriptor, msg protoreflect.Message, jsonPath string, errorReporter errorreporter.ErrorReporter) error {
	var errors jsonpbhelper.UnmarshalErrorList
	if err := validateReferenceTypes(fd, msg); err != nil {
		if !jsonpbhelper.IsUnmarshalError(err) {
			return err
		}
		if err := jsonpbhelper.AppendUnmarshalError(&errors, jsonpbhelper.AnnotateUnmarshalErrorWithPath(err, jsonPath)); err != nil {
			return err
		}
		for _, error := range errors {
			errorReporter.ReportValidationError(error.Path, error)
		}
	}
	return nil
}

func validatePrimitives(_ protoreflect.FieldDescriptor, msg protoreflect.Message) error {
	if !validatedTypes.Contains(string(msg.Descriptor().FullName())) {
		return nil
	}
	name := string(msg.Descriptor().FullName())
	switch {
	case stringMessageNames.Contains(name):
		return jsonpbhelper.ValidateString(msg.Get(msg.Descriptor().Fields().ByName("value")).String())
	case intMessageNames.Contains(name):
		val := msg.Get(msg.Descriptor().Fields().ByName("value")).Uint()
		if val > math.MaxInt32 {
			// The spec doesn't actually allow the full range of an unsigned integer,
			// it's only the range of PositiveInt plus 0.
			start := 0
			if positiveIntMessageNames.Contains(name) {
				start = 1
			}
			return &jsonpbhelper.UnmarshalError{
				Details: fmt.Sprintf("non-negative integer out of range %d..2,147,483,647", start),
			}
		}
	case stringRegexMessageNames.Contains(name):
		if matched := validateStringPrimitiveRegex(msg); !matched {
			return &jsonpbhelper.UnmarshalError{
				Details: fmt.Sprintf("invalid %s format", msg.Descriptor().Name()),
			}
		}
	case urlMessageNames.Contains(string(msg.Descriptor().FullName())):
		val := msg.Get(msg.Descriptor().Fields().ByName("value")).String()
		if _, err := url.Parse(val); err != nil {
			return &jsonpbhelper.UnmarshalError{
				Details: fmt.Sprintf("invalid %s", strings.ToLower(string(msg.Descriptor().Name()))),
			}
		}
	}
	return nil
}

func validatePrimitivesWithErrorReporter(fd protoreflect.FieldDescriptor, msg protoreflect.Message, jsonPath string, errorReporter errorreporter.ErrorReporter) error {
	var errors jsonpbhelper.UnmarshalErrorList
	if err := validatePrimitives(fd, msg); err != nil {
		if !jsonpbhelper.IsUnmarshalError(err) {
			return err
		}
		if err := jsonpbhelper.AppendUnmarshalError(&errors, jsonpbhelper.AnnotateUnmarshalErrorWithPath(err, jsonPath)); err != nil {
			return err
		}
		for _, error := range errors {
			errorReporter.ReportValidationError(error.Path, err)
		}
	}
	return nil
}

func validateStringPrimitiveRegex(msg protoreflect.Message) bool {
	val := msg.Get(msg.Descriptor().Fields().ByName("value")).String()
	return jsonpbhelper.RegexValues[msg.Descriptor().FullName()].MatchString(val)
}

// Validate a FHIR msg against the rules defined in the FHIR spec. See package
// description for what is included.
func Validate(msg proto.Message) error {
	validationSteps := []validationStep{
		validatePrimitives,
		validateRequiredFields,
		validateReferenceTypes,
	}
	return walkMessage(msg.ProtoReflect(), nil, "", validationSteps)
}

// ValidateWithErrorReporter validates a FHIR msg against the rules defined in the FHIR
// spec, validation errors will be reported according to provided error reporter.
// See package description for what is included.
func ValidateWithErrorReporter(msg proto.Message, er errorreporter.ErrorReporter) error {
	validationSteps := []validationStepWithErrorReporter{
		validatePrimitivesWithErrorReporter,
		validateRequiredFieldsWithErrorReporter,
		validateReferenceTypesWithErrorReporter,
	}
	return walkMessageWithErrorReporter(msg.ProtoReflect(), nil, "", validationSteps, er)
}

// ValidatePrimitives on the msg according to the FHIR spec. This includes
// regexes for string-based types and bounds checking for integers.
func ValidatePrimitives(msg proto.Message) error {
	return walkMessage(msg.ProtoReflect(), nil, "", []validationStep{validatePrimitives})
}

// ValidatePrimitivesWithErrorReporter on the msg according to the FHIR spec. This includes
// regexes for string-based types and bounds checking for integers.
// Validation errors will be reported according to provided error reporter.
func ValidatePrimitivesWithErrorReporter(msg proto.Message, er errorreporter.ErrorReporter) error {
	return walkMessageWithErrorReporter(msg.ProtoReflect(), nil, "", []validationStepWithErrorReporter{validatePrimitivesWithErrorReporter}, er)
}

func addFieldToPath(jsonPath, field string) string {
	if len(jsonPath) == 0 {
		field = strings.Title(field)
	}
	return jsonpbhelper.AddFieldToPath(jsonPath, field)
}

func walkMessage(msg protoreflect.Message, fd protoreflect.FieldDescriptor, jsonPath string, validators []validationStep) error {
	var errors jsonpbhelper.UnmarshalErrorList
	for _, validator := range validators {
		if err := validator(fd, msg); err != nil {
			if err := jsonpbhelper.AppendUnmarshalError(&errors, jsonpbhelper.AnnotateUnmarshalErrorWithPath(err, jsonPath)); err != nil {
				return err
			}
		}
	}
	var fatalErr error
	msg.Range(func(fd protoreflect.FieldDescriptor, value protoreflect.Value) bool {
		if fd.Message() == nil {
			return true
		}
		jsonPath := addFieldToPath(jsonPath, fd.JSONName())
		if fd.IsList() {
			l := value.List()
			for i := 0; i < l.Len(); i++ {
				if err := walkMessage(l.Get(i).Message(), fd, jsonpbhelper.AddIndexToPath(jsonPath, i), validators); err != nil {
					if err := jsonpbhelper.AppendUnmarshalError(&errors, err); err != nil {
						fatalErr = err
						return false
					}
				}
			}
		} else {
			err := walkMessage(value.Message(), fd, jsonPath, validators)
			if err := jsonpbhelper.AppendUnmarshalError(&errors, err); err != nil {
				fatalErr = err
				return false
			}
		}
		return true
	})
	if fatalErr != nil {
		return fatalErr
	}
	if len(errors) > 0 {
		return errors
	}
	return nil
}

func walkMessageWithErrorReporter(msg protoreflect.Message, fd protoreflect.FieldDescriptor, jsonPath string, validators []validationStepWithErrorReporter, er errorreporter.ErrorReporter) error {
	for _, validator := range validators {
		if err := validator(fd, msg, jsonPath, er); err != nil {
			return err
		}
	}
	var fatalErr error
	msg.Range(func(fd protoreflect.FieldDescriptor, value protoreflect.Value) bool {
		if fd.Message() == nil {
			return true
		}
		jsonPath := addFieldToPath(jsonPath, fd.JSONName())
		if fd.IsList() {
			l := value.List()
			for i := 0; i < l.Len(); i++ {
				if err := walkMessageWithErrorReporter(l.Get(i).Message(), fd, jsonpbhelper.AddIndexToPath(jsonPath, i), validators, er); err != nil {
					fatalErr = err
					return false
				}
			}
		} else {
			if err := walkMessageWithErrorReporter(value.Message(), fd, jsonPath, validators, er); err != nil {
				fatalErr = err
				return false
			}
		}
		return true
	})
	if fatalErr != nil {
		return fatalErr
	}
	return nil
}
