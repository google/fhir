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

package fhirvalidate

import (
	"math"
	"testing"

	"github.com/google/fhir/go/fhirversion"
	"github.com/google/fhir/go/jsonformat/errorreporter"
	"github.com/google/fhir/go/jsonformat/internal/jsonpbhelper"
	"github.com/google/go-cmp/cmp"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/testing/protocmp"

	c4pb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/codes_go_proto"
	d4pb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/datatypes_go_proto"
	r4pb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/bundle_and_contained_resource_go_proto"
	r4outcomepb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/operation_outcome_go_proto"
	r4patientpb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/patient_go_proto"
	c3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/codes_go_proto"
	d3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/datatypes_go_proto"
	r3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/resources_go_proto"
)

func TestRequiredFields(t *testing.T) {
	tests := []proto.Message{
		&r3pb.ContainedResource{
			OneofResource: &r3pb.ContainedResource_Patient{
				Patient: &r3pb.Patient{
					Link: []*r3pb.Patient_Link{
						{},
					},
				},
			},
		},
		&r4pb.ContainedResource{
			OneofResource: &r4pb.ContainedResource_Patient{
				Patient: &r4patientpb.Patient{
					Link: []*r4patientpb.Patient_Link{
						{},
					},
				},
			},
		},
	}
	for _, test := range tests {
		wantErr := `error at "Patient.link[0]": missing required field "other"
error at "Patient.link[0]": missing required field "type"`
		err := Validate(test)
		if err == nil {
			t.Fatalf("Validate %v failed: got error < nil >, want %q", test, wantErr)
		} else if err.Error() != wantErr {
			t.Errorf("Validate %v: got error %q, want: %q", test, err.Error(), wantErr)
		}
	}
}

func TestReferenceTypes(t *testing.T) {
	tests := []proto.Message{
		&r3pb.ContainedResource{
			OneofResource: &r3pb.ContainedResource_Patient{
				Patient: &r3pb.Patient{
					ManagingOrganization: &d3pb.Reference{
						Reference: &d3pb.Reference_PatientId{
							PatientId: &d3pb.ReferenceId{Value: "2"},
						},
					},
				},
			},
		},
		&r4pb.ContainedResource{
			OneofResource: &r4pb.ContainedResource_Patient{
				Patient: &r4patientpb.Patient{
					ManagingOrganization: &d4pb.Reference{
						Reference: &d4pb.Reference_PatientId{
							PatientId: &d4pb.ReferenceId{Value: "2"},
						},
					},
				},
			},
		},
	}
	for _, test := range tests {
		wantErr := `error at "Patient.managingOrganization": invalid reference to a Patient resource, want Organization`
		err := Validate(test)
		if err == nil {
			t.Fatalf("Validate %v failed: got error < nil >, want %q", test, wantErr)
		} else if err.Error() != wantErr {
			t.Errorf("Validate %v: got error %q, want: %q", test, err.Error(), wantErr)
		}
	}
}

func TestValidatePrimitive_Success(t *testing.T) {
	tests := []struct {
		name string
		msgs []proto.Message
	}{
		{
			name: "primitive with no value",
			msgs: []proto.Message{
				&d3pb.Code{
					Extension: []*d3pb.Extension{{
						Url: &d3pb.Uri{Value: jsonpbhelper.PrimitiveHasNoValueURL},
					}},
				},
				&d4pb.Code{
					Extension: []*d4pb.Extension{{
						Url: &d4pb.Uri{Value: jsonpbhelper.PrimitiveHasNoValueURL},
					}},
				},
			},
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			for _, msg := range test.msgs {
				err := Validate(msg)
				if err != nil {
					t.Errorf("Validate(): failed %v", err)
				}
			}
		})
	}
}

func TestValidatePrimitive_Errors(t *testing.T) {
	tests := []struct {
		name string
		msgs []proto.Message
	}{
		{
			name: "Code left spaces",
			msgs: []proto.Message{
				&d3pb.Code{Value: "    left has spaces"},
				&d4pb.Code{Value: "    left has spaces"},
			},
		},
		{
			name: "Code right tabs",
			msgs: []proto.Message{
				&d3pb.Code{Value: "right has tabs\t\t"},
				&d4pb.Code{Value: "right has tabs\t\t"},
			},
		},
		{
			name: "Code left carriage return",
			msgs: []proto.Message{
				&d3pb.Code{Value: "\rleft has carriage return"},
				&d4pb.Code{Value: "\rleft has carriage return"},
			},
		},
		{
			name: "Code right new line",
			msgs: []proto.Message{
				&d3pb.Code{Value: "right has newlines\n\n"},
				&d4pb.Code{Value: "right has newlines\n\n"},
			},
		},
		{
			name: "Id too long",
			msgs: []proto.Message{
				&d3pb.Id{Value: "this.is.a.pretty.long.id-in.fact.it.has.65.characters--1.too.many"},
				&d4pb.Id{Value: "this.is.a.pretty.long.id-in.fact.it.has.65.characters--1.too.many"},
			},
		},
		{
			name: "Id has illegal character",
			msgs: []proto.Message{
				&d3pb.Id{Value: "#Ah0!"},
				&d4pb.Id{Value: "#Ah0!"},
			},
		},
		{
			name: "Oid has wrong prefix",
			msgs: []proto.Message{
				&d3pb.Oid{Value: "wrong:prefix:0.12.34"},
				&d4pb.Oid{Value: "wrong:prefix:0.12.34"},
			},
		},
		{
			name: "Oid has illegal character",
			msgs: []proto.Message{
				&d3pb.Oid{Value: "urn:old:1.23.0x97"},
				&d4pb.Oid{Value: "urn:old:1.23.0x97"},
			},
		},
		{
			name: "PositiveInt out of range",
			msgs: []proto.Message{
				&d3pb.PositiveInt{Value: math.MaxInt32 + 1},
				&d4pb.PositiveInt{Value: math.MaxInt32 + 1},
			},
		},
		{
			name: "UnsignedInt out of range",
			msgs: []proto.Message{
				&d3pb.UnsignedInt{Value: math.MaxInt32 + 1},
				&d4pb.UnsignedInt{Value: math.MaxInt32 + 1},
			},
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			for _, msg := range test.msgs {
				err := Validate(msg)
				if err == nil {
					t.Errorf("Validate() %v succeeded, expect error", test.name)
				}
			}
		})
	}
}

func TestValidateWithErrorReporter(t *testing.T) {
	tests := []struct {
		name         string
		msgs         []proto.Message
		wantOutcomes []*errorreporter.MultiVersionOperationOutcome
	}{
		{
			name: "missing required field",
			msgs: []proto.Message{
				&r3pb.ContainedResource{
					OneofResource: &r3pb.ContainedResource_Patient{
						Patient: &r3pb.Patient{
							Link: []*r3pb.Patient_Link{
								{},
							},
						},
					},
				},
				&r4pb.ContainedResource{
					OneofResource: &r4pb.ContainedResource_Patient{
						Patient: &r4patientpb.Patient{
							Link: []*r4patientpb.Patient_Link{
								{},
							},
						},
					},
				},
			},
			wantOutcomes: []*errorreporter.MultiVersionOperationOutcome{
				&errorreporter.MultiVersionOperationOutcome{
					Version: fhirversion.STU3,
					R3Outcome: &r3pb.OperationOutcome{
						Issue: []*r3pb.OperationOutcome_Issue{
							&r3pb.OperationOutcome_Issue{
								Code: &c3pb.IssueTypeCode{
									Value: c3pb.IssueTypeCode_VALUE,
								},
								Severity: &c3pb.IssueSeverityCode{
									Value: c3pb.IssueSeverityCode_ERROR,
								},
								Diagnostics: &d3pb.String{Value: `error at "Patient.link[0]": missing required field "other"`},
								Expression: []*d3pb.String{
									&d3pb.String{Value: `Patient.link[0]`},
								},
							},
							&r3pb.OperationOutcome_Issue{
								Code: &c3pb.IssueTypeCode{
									Value: c3pb.IssueTypeCode_VALUE,
								},
								Severity: &c3pb.IssueSeverityCode{
									Value: c3pb.IssueSeverityCode_ERROR,
								},
								Diagnostics: &d3pb.String{Value: `error at "Patient.link[0]": missing required field "type"`},
								Expression: []*d3pb.String{
									&d3pb.String{Value: `Patient.link[0]`},
								},
							},
						},
					},
				},
				&errorreporter.MultiVersionOperationOutcome{
					Version: fhirversion.R4,
					R4Outcome: &r4outcomepb.OperationOutcome{
						Issue: []*r4outcomepb.OperationOutcome_Issue{
							&r4outcomepb.OperationOutcome_Issue{
								Code: &r4outcomepb.OperationOutcome_Issue_CodeType{
									Value: c4pb.IssueTypeCode_VALUE,
								},
								Severity: &r4outcomepb.OperationOutcome_Issue_SeverityCode{
									Value: c4pb.IssueSeverityCode_ERROR,
								},
								Diagnostics: &d4pb.String{Value: `error at "Patient.link[0]": missing required field "other"`},
								Expression: []*d4pb.String{
									&d4pb.String{Value: `Patient.link[0]`},
								},
							},
							&r4outcomepb.OperationOutcome_Issue{
								Code: &r4outcomepb.OperationOutcome_Issue_CodeType{
									Value: c4pb.IssueTypeCode_VALUE,
								},
								Severity: &r4outcomepb.OperationOutcome_Issue_SeverityCode{
									Value: c4pb.IssueSeverityCode_ERROR,
								},
								Diagnostics: &d4pb.String{Value: `error at "Patient.link[0]": missing required field "type"`},
								Expression: []*d4pb.String{
									&d4pb.String{Value: `Patient.link[0]`},
								},
							},
						},
					},
				},
			},
		},
		{
			name: "invalid reference type",
			msgs: []proto.Message{
				&r3pb.ContainedResource{
					OneofResource: &r3pb.ContainedResource_Patient{
						Patient: &r3pb.Patient{
							ManagingOrganization: &d3pb.Reference{
								Reference: &d3pb.Reference_PatientId{
									PatientId: &d3pb.ReferenceId{Value: "2"},
								},
							},
						},
					},
				},
				&r4pb.ContainedResource{
					OneofResource: &r4pb.ContainedResource_Patient{
						Patient: &r4patientpb.Patient{
							ManagingOrganization: &d4pb.Reference{
								Reference: &d4pb.Reference_PatientId{
									PatientId: &d4pb.ReferenceId{Value: "2"},
								},
							},
						},
					},
				},
			},
			wantOutcomes: []*errorreporter.MultiVersionOperationOutcome{
				&errorreporter.MultiVersionOperationOutcome{
					Version: fhirversion.STU3,
					R3Outcome: &r3pb.OperationOutcome{
						Issue: []*r3pb.OperationOutcome_Issue{
							&r3pb.OperationOutcome_Issue{
								Code: &c3pb.IssueTypeCode{
									Value: c3pb.IssueTypeCode_VALUE,
								},
								Severity: &c3pb.IssueSeverityCode{
									Value: c3pb.IssueSeverityCode_ERROR,
								},
								Diagnostics: &d3pb.String{Value: `error at "Patient.managingOrganization": invalid reference to a Patient resource, want Organization`},
								Expression: []*d3pb.String{
									&d3pb.String{Value: `Patient.managingOrganization`},
								},
							},
						},
					},
				},
				&errorreporter.MultiVersionOperationOutcome{
					Version: fhirversion.R4,
					R4Outcome: &r4outcomepb.OperationOutcome{
						Issue: []*r4outcomepb.OperationOutcome_Issue{
							&r4outcomepb.OperationOutcome_Issue{
								Code: &r4outcomepb.OperationOutcome_Issue_CodeType{
									Value: c4pb.IssueTypeCode_VALUE,
								},
								Severity: &r4outcomepb.OperationOutcome_Issue_SeverityCode{
									Value: c4pb.IssueSeverityCode_ERROR,
								},
								Diagnostics: &d4pb.String{Value: `error at "Patient.managingOrganization": invalid reference to a Patient resource, want Organization`},
								Expression: []*d4pb.String{
									&d4pb.String{Value: `Patient.managingOrganization`},
								},
							},
						},
					},
				},
			},
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			for i, wantOut := range test.wantOutcomes {
				t.Run(wantOut.Version.String(), func(t *testing.T) {
					oer := errorreporter.NewOperationErrorReporter(wantOut.Version)
					err := ValidateWithErrorReporter(test.msgs[i], oer)
					if err != nil {
						t.Fatalf("unmarshal with outcome %v failed: %v", test.name, err)
					}
					if diff := cmp.Diff(oer.Outcome, wantOut, protocmp.Transform()); diff != "" {
						t.Errorf("unmarshal with outcome %v: got outcome %v, want %v", test.name, oer.Outcome, wantOut)
					}
				})
			}
		})
	}
}
