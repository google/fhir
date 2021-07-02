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

	"github.com/google/fhir/go/fhirversion"
	"google.golang.org/protobuf/proto"
	"bitbucket.org/creachadair/stringset"

	dpb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/datatypes_go_proto"
	epb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/fhirproto_extensions_go_proto"
	rpb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/resources_go_proto"
)

type r3Config struct{}

func (c r3Config) newEmptyContainedResource() proto.Message {
	return &rpb.ContainedResource{}
}

func (c r3Config) newBase64BinarySeparatorStride(sep string, stride uint32) proto.Message {
	return &epb.Base64BinarySeparatorStride{
		Separator: &dpb.String{Value: sep},
		Stride:    &dpb.PositiveInt{Value: stride},
	}
}

func (c r3Config) newPrimitiveHasNoValue(bv bool) proto.Message {
	return &epb.PrimitiveHasNoValue{
		ValueBoolean: &dpb.Boolean{Value: bv},
	}
}

func (c r3Config) noIDFieldTypes() stringset.Set {
	return stringset.New()
}

func (c r3Config) keysToSkip() stringset.Set {
	return stringset.New()
}

// UnmarshalR3 returns the corresponding protobuf message given a serialized FHIR JSON object
func (u *Unmarshaller) UnmarshalR3(in []byte) (*rpb.ContainedResource, error) {
	if _, ok := u.cfg.(r3Config); !ok {
		return nil, fmt.Errorf("the unmarshaler is not for FHIR %s", fhirversion.STU3)
	}
	p, err := u.Unmarshal(in)
	if err != nil {
		return nil, err
	}
	return p.(*rpb.ContainedResource), nil
}
