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

	"bitbucket.org/creachadair/stringset"
	"google.golang.org/protobuf/proto"

	dpb "github.com/google/fhir/go/proto/google/fhir/proto/dstu2/datatypes_go_proto"
	epb "github.com/google/fhir/go/proto/google/fhir/proto/dstu2/fhirproto_extensions_go_proto"
	rpb "github.com/google/fhir/go/proto/google/fhir/proto/dstu2/resources_go_proto"
)

type dstu2Config struct{}

func (c dstu2Config) newEmptyContainedResource() proto.Message {
	return &rpb.ContainedResource{}
}

func (c dstu2Config) newBase64BinarySeparatorStride(sep string, stride uint32) proto.Message {
	return &epb.Base64BinarySeparatorStride{
		Separator: &dpb.String{Value: sep},
		Stride:    &dpb.PositiveInt{Value: stride},
	}
}

func (c dstu2Config) newPrimitiveHasNoValue(bv bool) proto.Message {
	return &epb.PrimitiveHasNoValue{
		ValueBoolean: &dpb.Boolean{Value: bv},
	}
}

func (c dstu2Config) noIDFieldTypes() stringset.Set {
	return stringset.New()
}

func (c dstu2Config) keysToSkip() stringset.Set {
	return stringset.New()
}

// UnmarshalDSTU2 returns the corresponding protobuf message given a serialized FHIR JSON object
func (u *Unmarshaller) UnmarshalDSTU2(in []byte) (*rpb.ContainedResource, error) {
	if _, ok := u.cfg.(dstu2Config); !ok {
		return nil, fmt.Errorf("the unmarshaler is not for FHIR %s", DSTU2)
	}
	p, err := u.Unmarshal(in)
	if err != nil {
		return nil, err
	}
	return p.(*rpb.ContainedResource), nil
}
