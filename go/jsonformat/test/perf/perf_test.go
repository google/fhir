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

// Package perf_test evaluates the performance of the marshaller and unmarshaller.
package perf_test

import (
	"io/ioutil"
	"bytes"
	"encoding/base64"
	"io"
	"math/rand"
	"path"
	"strings"
	"testing"

	"github.com/bazelbuild/rules_go/go/tools/bazel"
	"github.com/google/fhir/go/fhirversion"
	"github.com/google/fhir/go/jsonformat"
	d4pb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/datatypes_go_proto"
	r4binarypb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/binary_go_proto"
	r4pb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/bundle_and_contained_resource_go_proto"
)

const (
	testDataDir          = "testdata"
	benchmarkParallelism = 8
)

func prepare(b *testing.B, f string, enableValidation bool) ([]byte, *jsonformat.Marshaller, *jsonformat.Unmarshaller) {
	p := path.Join(testDataDir, f)
	path, err := bazel.Runfile(p)
	if err != nil {
	  b.Fatalf("Failed to read file %s due to error: %v", p, err)
	}
	d, err := ioutil.ReadFile(path)
	if err != nil {
	  b.Fatalf("Failed to read file %s due to error: %v", p, err)
	}
	if err != nil {
		b.Fatalf("Failed to read file %s due to error: %v", p, err)
	}
	m, err := jsonformat.NewMarshaller(false, "", "  ", fhirversion.STU3)
	if err != nil {
		b.Fatalf("Failed to create the marshaller due to error: %v", err)
	}
	var um *jsonformat.Unmarshaller
	if enableValidation {
		um, err = jsonformat.NewUnmarshaller("UTC", fhirversion.STU3)
	} else {
		um, err = jsonformat.NewUnmarshallerWithoutValidation("UTC", fhirversion.STU3)
	}
	if err != nil {
		b.Fatalf("Failed to create the unmarshaller due to error: %v", err)
	}
	return d, m, um
}

func roundTrip(b *testing.B, d []byte, m *jsonformat.Marshaller, um *jsonformat.Unmarshaller) {
	r, err := um.Unmarshal(d)
	if err != nil {
		b.Fatalf("Failed to unmarshal data due to error: %v", err)
	}
	if _, err := m.Marshal(r); err != nil {
		b.Fatalf("Failed to marshal data due to error: %v", err)
	}
}

func BenchmarkRoundTrip(b *testing.B) {
	d, m, um := prepare(b, "synthea.bundle.json", true)
	for i := 0; i < b.N; i++ {
		roundTrip(b, d, m, um)
	}
}

func BenchmarkRoundTrip_NoValidation(b *testing.B) {
	d, m, um := prepare(b, "synthea.bundle.json", false)
	for i := 0; i < b.N; i++ {
		roundTrip(b, d, m, um)
	}
}

func parallelRoundTrip(b *testing.B, d []byte, m *jsonformat.Marshaller, um *jsonformat.Unmarshaller) {
	count := b.N / benchmarkParallelism
	c := make(chan struct{}, benchmarkParallelism)
	for i := 0; i < benchmarkParallelism; i++ {
		go func() {
			for i := 0; i < count; i++ {
				roundTrip(b, d, m, um)
			}
			c <- struct{}{}
		}()
	}
	for i := 0; i < benchmarkParallelism; i++ {
		<-c
	}
}

func BenchmarkRoundTrip_InParallel(b *testing.B) {
	d, m, um := prepare(b, "synthea.bundle.json", true)
	parallelRoundTrip(b, d, m, um)
}

func BenchmarkRoundTrip_InParallel_NoValidation(b *testing.B) {
	d, m, um := prepare(b, "synthea.bundle.json", false)
	parallelRoundTrip(b, d, m, um)
}

func BenchmarkLargeArray(b *testing.B) {
	_, m, um := prepare(b, "synthea.bundle.json", true)
	sb := strings.Builder{}
	sb.WriteString(`{"resourceType":"Group","actual":true,"type":"person","member":[`)
	for i := 0; i < 100000; i++ {
		if i > 0 {
			sb.WriteRune(',')
		}
		sb.WriteString(`{"entity":{"reference":"Patient/1"}}`)
	}
	sb.WriteString("]}")
	d := []byte(sb.String())
	b.ResetTimer()

	for i := 0; i < b.N; i++ {
		roundTrip(b, d, m, um)
	}
}

func BenchmarkLargeBinary_Marshal(b *testing.B) {
	r := rand.New(rand.NewSource(0))
	buf := bytes.Buffer{}
	if _, err := io.CopyN(&buf, r, 1<<30); err != nil {
		b.Fatal(err.Error())
	}
	res := &r4pb.ContainedResource{
		OneofResource: &r4pb.ContainedResource_Binary{
			Binary: &r4binarypb.Binary{
				Data: &d4pb.Base64Binary{Value: buf.Bytes()},
			},
		},
	}
	m, err := jsonformat.NewMarshaller(false, "", "  ", fhirversion.R4)
	if err != nil {
		b.Fatalf("Failed to create the marshaller due to error: %v", err)
	}
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		_, err := m.Marshal(res)
		if err != nil {
			b.Error(err.Error())
		}
	}
}

func BenchmarkLargeBinary_Unmarshal(b *testing.B) {
	r := rand.New(rand.NewSource(0))
	sb := strings.Builder{}
	if _, err := sb.WriteString(`{"resourceType":"Binary","contentType":"application/octet-stream","data":"`); err != nil {
		b.Fatal(err.Error())
	}

	enc := base64.NewEncoder(base64.StdEncoding, &sb)

	if _, err := io.CopyN(enc, r, 1<<30); err != nil {
		b.Fatal(err.Error())
	}

	if err := enc.Close(); err != nil {
		b.Fatal(err.Error())
	}
	if _, err := sb.WriteString(`"}`); err != nil {
		b.Fatal(err.Error())
	}

	um, err := jsonformat.NewUnmarshaller("UTC", fhirversion.R4)
	if err != nil {
		b.Fatalf("Failed to create the unmarshaller due to error: %v", err)
	}

	b.ResetTimer()

	for i := 0; i < b.N; i++ {
		_, err := um.Unmarshal([]byte(sb.String()))
		if err != nil {
			b.Error(err.Error())
		}
	}
}
