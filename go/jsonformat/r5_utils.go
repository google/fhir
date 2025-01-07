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
	"bufio"
	"fmt"
	"io"

	"github.com/google/fhir/go/fhirversion"
	"google.golang.org/protobuf/proto"
	"bitbucket.org/creachadair/stringset"

	dpb "github.com/google/fhir/go/proto/google/fhir/proto/r5/core/datatypes_go_proto"
	rpb "github.com/google/fhir/go/proto/google/fhir/proto/r5/core/resources/bundle_and_contained_resource_go_proto"
	epb "github.com/google/fhir/go/proto/google/fhir/proto/r5/fhirproto_extensions_go_proto"
)

const (
	// r5ChoiceOneofName is the hardcoded "choice".
	// In R5, all oneof fields are named as "choice". So we try to find a field by that hardcoded
	// name as a fallback approach.
	r5ChoiceOneofName = "choice"
)

type r5Config struct{}

func (c r5Config) newEmptyContainedResource() proto.Message {
	return &rpb.ContainedResource{}
}

func (c r5Config) newBase64BinarySeparatorStride(sep string, stride uint32) proto.Message {
	return &epb.Base64BinarySeparatorStride{
		Separator: &dpb.String{Value: sep},
		Stride:    &dpb.PositiveInt{Value: stride},
	}
}

func (c r5Config) newPrimitiveHasNoValue(bv bool) proto.Message {
	return &epb.PrimitiveHasNoValue{
		ValueBoolean: &dpb.Boolean{Value: bv},
	}
}

func (c r5Config) noIDFieldTypes() stringset.Set {
	return stringset.New()
}

func (c r5Config) keysToSkip() stringset.Set {
	return stringset.New()
}

// UnmarshalR5 returns the corresponding protobuf message given a serialized FHIR JSON object
func (u *Unmarshaller) UnmarshalR5(in []byte) (*rpb.ContainedResource, error) {
	if _, ok := u.cfg.(r5Config); !ok {
		return nil, fmt.Errorf("the unmarshaler is not for FHIR %s", fhirversion.R5)
	}
	p, err := u.Unmarshal(in)
	if err != nil {
		return nil, err
	}
	return p.(*rpb.ContainedResource), nil
}

// ContainedR5ResourceOrError holds a ContainedResource or an error as a single entity. If Error is
// set, that indicates there was an error unmarshaling this contained resource (and the error is
// propagated through from the underlying UnmarshalR5 call).
type ContainedR5ResourceOrError struct {
	ContainedResource *rpb.ContainedResource
	Error             error
}

// UnmarshalR5Streaming reads FHIR NDJSON from the provided io.Reader and writes parsed
// ContainedResources (or an error) to the returned channel. When the input io.Reader is exhausted
// and there are no more messages to be parsed, the output channel will be closed.
func (u *Unmarshaller) UnmarshalR5Streaming(in io.Reader) <-chan *ContainedR5ResourceOrError {
	s := bufio.NewScanner(in)
	out := make(chan *ContainedR5ResourceOrError)

	go func() {
		for s.Scan() {
			resource, err := u.UnmarshalR5(s.Bytes())
			out <- &ContainedR5ResourceOrError{resource, err}
		}
		if err := s.Err(); err != nil {
			out <- &ContainedR5ResourceOrError{nil, err}
		}
		close(out)
	}()

	return out

}
