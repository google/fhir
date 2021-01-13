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

	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/reflect/protoreflect"
	"bitbucket.org/creachadair/stringset"
)

// A Version is a version of the FHIR standard.
type Version string

// FHIR converter versions.
const (
	DSTU2 = Version("DSTU2")
	STU3  = Version("STU3")
	R4    = Version("R4")
)

// String returns the Version as a string.
func (v Version) String() string {
	return string(v)
}

// config encapsulates the configuration for the converter of a particular FHIR version.
type config interface {
	newEmptyContainedResource() proto.Message
	newBase64BinarySeparatorStride(sep string, stride uint32) proto.Message
	newPrimitiveHasNoValue(b bool) proto.Message
	noIDFieldTypes() stringset.Set
	keysToSkip() stringset.Set
}

// getConfig returns the converter config for the given FHIR version.
func getConfig(ver Version) (config, error) {
	switch ver {
	case DSTU2:
		return dstu2Config{}, nil
	case STU3:
		return r3Config{}, nil
	case R4:
		return r4Config{}, nil
	default:
		return nil, fmt.Errorf("unsupported FHIR version %s", ver)
	}
}

// containedResourceProtoName returns the short proto name of the ContainedResource
// proto provided by the given config
func containedResourceProtoName(cfg config) protoreflect.Name {
	cr := cfg.newEmptyContainedResource()
	return cr.ProtoReflect().Descriptor().Name()
}
