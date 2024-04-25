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
	"testing"

	"github.com/google/go-cmp/cmp"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/testing/protocmp"

	apb "github.com/google/fhir/go/proto/google/fhir/proto/annotations_go_proto"
	d4pb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/datatypes_go_proto"
	d3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/datatypes_go_proto"
)

func referenceProto(t *testing.T, ver apb.FhirVersion) proto.Message {
	switch ver {
	case apb.FhirVersion_STU3:
		return &d3pb.Reference{}
	case apb.FhirVersion_R4:
		return &d4pb.Reference{}
	}
	t.Fatalf("unsupported version: %s", ver)
	return nil
}

func TestAllReferenceTypes(t *testing.T) {
	versionsToSkip := []apb.FhirVersion{
		apb.FhirVersion_FHIR_VERSION_UNKNOWN,
		apb.FhirVersion_R4B, // TODO(b/265289586): Testing support for r4b
		apb.FhirVersion_R5,
	}

	for _, v := range apb.FhirVersion_value {
		ver := apb.FhirVersion(v)
		if contains(versionsToSkip, ver) {
			t.Skipf("Skipping testing version: %s", ver.String())
		}
		t.Run(ver.String(), func(t *testing.T) {
			ref := referenceProto(t, ver)
			if err := NormalizeReference(ref); err != nil {
				t.Fatalf("unsupported version: %s", ver)
			}
			if err := DenormalizeReference(ref); err != nil {
				t.Fatalf("unsupported version: %s", ver)
			}
		})
	}
}

func contains(sl []apb.FhirVersion, v apb.FhirVersion) bool {
	for _, e := range sl {
		if e == v {
			return true
		}
	}
	return false
}

func TestNormalizeDenormalizeReference(t *testing.T) {
	tests := []struct {
		name                     string
		denormalized, normalized proto.Message
	}{
		{
			"stu3 absolute url",
			&d3pb.Reference{
				Reference: &d3pb.Reference_Uri{
					Uri: &d3pb.String{
						Value: "http://a.com/patient123",
					},
				},
			},
			&d3pb.Reference{
				Reference: &d3pb.Reference_Uri{
					Uri: &d3pb.String{
						Value: "http://a.com/patient123",
					},
				},
			},
		},
		{
			"stu3 relative reference",
			&d3pb.Reference{
				Reference: &d3pb.Reference_Uri{
					Uri: &d3pb.String{
						Value: "AdverseEvent/abc123",
					},
				},
			},
			&d3pb.Reference{
				Reference: &d3pb.Reference_AdverseEventId{
					AdverseEventId: &d3pb.ReferenceId{
						Value: "abc123",
					},
				},
			},
		},
		{
			"stu3 relative reference with history",
			&d3pb.Reference{
				Reference: &d3pb.Reference_Uri{
					Uri: &d3pb.String{
						Value: "Patient/abc123/_history/3",
					},
				},
			},
			&d3pb.Reference{
				Reference: &d3pb.Reference_PatientId{
					PatientId: &d3pb.ReferenceId{
						Value: "abc123",
						History: &d3pb.Id{
							Value: "3",
						},
					},
				},
			},
		},
		{
			"stu3 internal reference",
			&d3pb.Reference{
				Reference: &d3pb.Reference_Uri{
					Uri: &d3pb.String{
						Value: "#frag_xyz",
					},
				},
			},
			&d3pb.Reference{
				Reference: &d3pb.Reference_Fragment{
					Fragment: &d3pb.String{
						Value: "frag_xyz",
					},
				},
			},
		},
		{
			"r4 absolute url",
			&d4pb.Reference{
				Reference: &d4pb.Reference_Uri{
					Uri: &d4pb.String{
						Value: "http://a.com/patient123",
					},
				},
			},
			&d4pb.Reference{
				Reference: &d4pb.Reference_Uri{
					Uri: &d4pb.String{
						Value: "http://a.com/patient123",
					},
				},
			},
		},
		{
			"r4 relative reference",
			&d4pb.Reference{
				Reference: &d4pb.Reference_Uri{
					Uri: &d4pb.String{
						Value: "SubstanceSpecification/abc123",
					},
				},
			},
			&d4pb.Reference{
				Reference: &d4pb.Reference_SubstanceSpecificationId{
					SubstanceSpecificationId: &d4pb.ReferenceId{
						Value: "abc123",
					},
				},
			},
		},
		{
			"r4 relative reference with history",
			&d4pb.Reference{
				Reference: &d4pb.Reference_Uri{
					Uri: &d4pb.String{
						Value: "Patient/abc123/_history/3",
					},
				},
			},
			&d4pb.Reference{
				Reference: &d4pb.Reference_PatientId{
					PatientId: &d4pb.ReferenceId{
						Value: "abc123",
						History: &d4pb.Id{
							Value: "3",
						},
					},
				},
			},
		},
		{
			"r4 internal reference",
			&d4pb.Reference{
				Reference: &d4pb.Reference_Uri{
					Uri: &d4pb.String{
						Value: "#frag_xyz",
					},
				},
			},
			&d4pb.Reference{
				Reference: &d4pb.Reference_Fragment{
					Fragment: &d4pb.String{
						Value: "frag_xyz",
					},
				},
			},
		},
	}
	for _, test := range tests {
		got := proto.Clone(test.denormalized)
		if err := NormalizeReference(got); err != nil {
			t.Fatalf("NormalizeReference(%q): %v", test.name, err)
		}
		if want := test.normalized; !proto.Equal(want, got) {
			t.Errorf("NormalizeReference(%q): got %v, want %v", test.name, got, want)
		}
	}
	for _, test := range tests {
		got := proto.Clone(test.normalized)
		if err := DenormalizeReference(got); err != nil {
			t.Fatalf("DenormalizeReference(%q): %v", test.name, err)
		}
		if want := test.denormalized; !proto.Equal(want, got) {
			t.Errorf("DenormalizeReference(%q): got %v, want %v", test.name, got, want)
		}
	}
}

func TestNormalizeReference_Errors(t *testing.T) {
	tests := []struct {
		name string
		in   proto.Message
	}{
		{
			name: "r3 non-existing resource type in reference",
			in: &d3pb.Reference{
				Reference: &d3pb.Reference_Uri{
					Uri: &d3pb.String{
						Value: "SubstanceSpecification/123",
					},
				},
			},
		},
		{
			name: "r4 non-existing resource type in reference",
			in: &d4pb.Reference{
				Reference: &d4pb.Reference_Uri{
					Uri: &d4pb.String{
						Value: "BodySite/123",
					},
				},
			},
		},
	}

	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			if err := NormalizeReference(tc.in); err == nil {
				t.Errorf("NormalizeReference(%v) did not return an error", tc.in)
			}
		})
	}
}

func TestNormalizeRelativeReferenceAndIgnoreHistory(t *testing.T) {
	tests := []struct {
		name                     string
		denormalized, normalized proto.Message
	}{
		{
			"stu3 no history",
			&d3pb.Reference{
				Reference: &d3pb.Reference_Uri{
					Uri: &d3pb.String{
						Value: "Patient/abc123",
					},
				},
			},
			&d3pb.Reference{
				Reference: &d3pb.Reference_PatientId{
					PatientId: &d3pb.ReferenceId{
						Value: "abc123",
					},
				},
			},
		},
		{
			"stu3 relative reference with history",
			&d3pb.Reference{
				Reference: &d3pb.Reference_Uri{
					Uri: &d3pb.String{
						Value: "Patient/abc123/_history/3",
					},
				},
			},
			&d3pb.Reference{
				Reference: &d3pb.Reference_PatientId{
					PatientId: &d3pb.ReferenceId{
						Value: "abc123",
					},
				},
			},
		},
		{
			"stu3 identifier reference",
			&d3pb.Reference{
				Identifier: &d3pb.Identifier{
					System: &d3pb.Uri{Value: "http://example.com/fhir/identifier"},
					Value:  &d3pb.String{Value: "Patient/abc123"},
				},
			},
			&d3pb.Reference{
				Identifier: &d3pb.Identifier{
					System: &d3pb.Uri{Value: "http://example.com/fhir/identifier"},
					Value:  &d3pb.String{Value: "Patient/abc123"},
				},
			},
		},
		{
			"r4 no history",
			&d4pb.Reference{
				Reference: &d4pb.Reference_Uri{
					Uri: &d4pb.String{
						Value: "Patient/abc123",
					},
				},
			},
			&d4pb.Reference{
				Reference: &d4pb.Reference_PatientId{
					PatientId: &d4pb.ReferenceId{
						Value: "abc123",
					},
				},
			},
		},
		{
			"r4 relative reference with history",
			&d4pb.Reference{
				Reference: &d4pb.Reference_Uri{
					Uri: &d4pb.String{
						Value: "Patient/abc123/_history/3",
					},
				},
			},
			&d4pb.Reference{
				Reference: &d4pb.Reference_PatientId{
					PatientId: &d4pb.ReferenceId{
						Value: "abc123",
					},
				},
			},
		},
		{
			"r4 identifier reference",
			&d4pb.Reference{
				Identifier: &d4pb.Identifier{
					System: &d4pb.Uri{Value: "http://example.com/fhir/identifier"},
					Value:  &d4pb.String{Value: "Patient/abc123"},
				},
			},
			&d4pb.Reference{
				Identifier: &d4pb.Identifier{
					System: &d4pb.Uri{Value: "http://example.com/fhir/identifier"},
					Value:  &d4pb.String{Value: "Patient/abc123"},
				},
			},
		},
	}
	for _, test := range tests {
		got := proto.Clone(test.denormalized)
		if err := normalizeRelativeReferenceAndIgnoreHistory(got); err != nil {
			t.Fatalf("NormalizeRelativeReferenceAndIgnoreHistory(%q): %v", test.name, err)
		}
		if want := test.normalized; !cmp.Equal(want, got, protocmp.Transform()) {
			t.Errorf("NormalizeRelativeReferenceAndIgnoreHistory(%q): got %v, want %v", test.name, got, want)
		}
	}
}

func TestNormalizeInternalReference(t *testing.T) {
	tests := []struct {
		name                     string
		denormalized, normalized proto.Message
	}{
		{
			"stu3 relative reference",
			&d3pb.Reference{
				Reference: &d3pb.Reference_Uri{
					Uri: &d3pb.String{
						Value: "Patient/abc123",
					},
				},
			},
			&d3pb.Reference{
				Reference: &d3pb.Reference_Uri{
					Uri: &d3pb.String{
						Value: "Patient/abc123",
					},
				},
			},
		},
		{
			"stu3 internal reference",
			&d3pb.Reference{
				Reference: &d3pb.Reference_Uri{
					Uri: &d3pb.String{
						Value: "#frag_xyz",
					},
				},
			},
			&d3pb.Reference{
				Reference: &d3pb.Reference_Fragment{
					Fragment: &d3pb.String{
						Value: "frag_xyz",
					},
				},
			},
		},
		{
			"stu3 existing internal reference",
			&d3pb.Reference{
				Reference: &d3pb.Reference_Fragment{
					Fragment: &d3pb.String{
						Value: "frag_xyz",
					},
				},
			},
			&d3pb.Reference{
				Reference: &d3pb.Reference_Fragment{
					Fragment: &d3pb.String{
						Value: "frag_xyz",
					},
				},
			},
		},
	}
	for _, test := range tests {
		got := proto.Clone(test.denormalized)
		if err := normalizeFragmentReference(got); err != nil {
			t.Fatalf("NormalizeFragmentReference(%v): %v", test.name, err)
		}
		if want := test.normalized; !cmp.Equal(want, got, protocmp.Transform()) {
			t.Errorf("NormalizeFragmentReference(%v): got %v, want %v", test.name, got, want)
		}
	}
}
