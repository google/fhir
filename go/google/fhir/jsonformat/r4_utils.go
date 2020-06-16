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

	"github.com/golang/protobuf/proto"
	"bitbucket.org/creachadair/stringset"

	dpb "google/fhir/proto/r4/core/datatypes_go_proto"
	rpb "google/fhir/proto/r4/core/resources/bundle_and_contained_resource_go_proto"
	epb "google/fhir/proto/r4/google_extensions_go_proto"
)

const (
	// r4ChoiceOneofName is the harcoded "choice".
	// In R4, all oneof fields are named as "choice". So we try to find a field by that hardcoded
	// name as a fallback approach.
	r4ChoiceOneofName = "choice"
)

type r4Config struct{}

func (c r4Config) newEmptyContainedResource() proto.Message {
	return &rpb.ContainedResource{}
}

func (c r4Config) newBase64BinarySeparatorStride(sep string, stride uint32) proto.Message {
	return &epb.Base64BinarySeparatorStride{
		Separator: &dpb.String{Value: sep},
		Stride:    &dpb.PositiveInt{Value: stride},
	}
}

func (c r4Config) newPrimitiveHasNoValue(bv bool) proto.Message {
	return &epb.PrimitiveHasNoValue{
		ValueBoolean: &dpb.Boolean{Value: bv},
	}
}

func (c r4Config) noIDFieldTypes() stringset.Set {
	return stringset.New()
}

func (c r4Config) keysToSkip() stringset.Set {
	return stringset.New()
}

// UnmarshalR4 returns the corresponding protobuf message given a serialized FHIR JSON object
func (u *Unmarshaller) UnmarshalR4(in []byte) (*rpb.ContainedResource, error) {
	if _, ok := u.cfg.(r4Config); !ok {
		return nil, fmt.Errorf("the unmarshaler is not for FHIR %s", R4)
	}
	p, err := u.Unmarshal(in)
	if err != nil {
		return nil, err
	}
	return p.(*rpb.ContainedResource), nil
}
