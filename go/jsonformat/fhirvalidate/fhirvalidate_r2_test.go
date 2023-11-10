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

	c2pb "github.com/google/fhir/go/proto/google/fhir/proto/dstu2/codes_go_proto"
	d2pb "github.com/google/fhir/go/proto/google/fhir/proto/dstu2/datatypes_go_proto"
	r2pb "github.com/google/fhir/go/proto/google/fhir/proto/dstu2/resources_go_proto"
)

func TestRequiredFields_R2(t *testing.T) {
	test := &r2pb.ContainedResource{
		OneofResource: &r2pb.ContainedResource_Patient{
			Patient: &r2pb.Patient{
				Link: []*r2pb.Patient_Link{
					{},
				},
			},
		},
	}
	wantErr := `error at "Patient.link[0]": missing required field "other"
error at "Patient.link[0]": missing required field "type"`
	err := Validate(test)
	if err == nil {
		t.Fatalf("Validate %v failed: got error < nil >, want %q", test, wantErr)
	} else if err.Error() != wantErr {
		t.Errorf("Validate %v: got error %q, want: %q", test, err.Error(), wantErr)
	}
}

func TestReferenceTypes_R2(t *testing.T) {
	test := &r2pb.ContainedResource{
		OneofResource: &r2pb.ContainedResource_Patient{
			Patient: &r2pb.Patient{
				ManagingOrganization: &d2pb.Reference{
					Reference: &d2pb.Reference_PatientId{
						PatientId: &d2pb.ReferenceId{Value: "2"},
					},
				},
			},
		},
	}
	wantErr := `error at "Patient.managingOrganization": invalid reference to a Patient resource, want Organization`
	err := Validate(test)
	if err == nil {
		t.Fatalf("Validate %v failed: got error < nil >, want %q", test, wantErr)
	} else if err.Error() != wantErr {
		t.Errorf("Validate %v: got error %q, want: %q", test, err.Error(), wantErr)
	}
}

func TestValidatePrimitive_R2_Success(t *testing.T) {
	msg := &d2pb.Code{
		Extension: []*d2pb.Extension{{
			Url: &d2pb.Uri{Value: jsonpbhelper.PrimitiveHasNoValueURL},
		}},
	}
	err := Validate(msg)
	if err != nil {
		t.Errorf("Validate(): failed %v", err)
	}
}

func TestValidatePrimitive_R2_Errors(t *testing.T) {
	tests := []struct {
		name string
		msg  proto.Message
	}{
		{
			name: "Code left spaces",
			msg:  &d2pb.Code{Value: "    left has spaces"},
		},
		{
			name: "Code right tabs",
			msg:  &d2pb.Code{Value: "right has tabs\t\t"},
		},
		{
			name: "Code left carriage return",
			msg:  &d2pb.Code{Value: "\rleft has carriage return"},
		},
		{
			name: "Code right new line",
			msg:  &d2pb.Code{Value: "right has newlines\n\n"},
		},
		{
			name: "Id too long",
			msg:  &d2pb.Id{Value: "this.is.a.pretty.long.id-in.fact.it.has.65.characters--1.too.many"},
		},
		{
			name: "Id has illegal character",
			msg:  &d2pb.Id{Value: "#Ah0!"},
		},
		{
			name: "Oid has wrong prefix",
			msg:  &d2pb.Oid{Value: "wrong:prefix:0.12.34"},
		},
		{
			name: "Oid has illegal character",
			msg:  &d2pb.Oid{Value: "urn:old:1.23.0x97"},
		},
		{
			name: "PositiveInt out of range",
			msg:  &d2pb.PositiveInt{Value: math.MaxInt32 + 1},
		},
		{
			name: "UnsignedInt out of range",
			msg:  &d2pb.UnsignedInt{Value: math.MaxInt32 + 1},
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			err := Validate(test.msg)
			if err == nil {
				t.Errorf("Validate() %v succeeded, expect error", test.name)
			}
		})
	}
}

func TestValidateWithErrorReporter_R2(t *testing.T) {
	tests := []struct {
		name        string
		msg         proto.Message
		wantOutcome *errorreporter.MultiVersionOperationOutcome
	}{
		{
			name: "missing required field",
			msg: &r2pb.ContainedResource{
				OneofResource: &r2pb.ContainedResource_Patient{
					Patient: &r2pb.Patient{
						Link: []*r2pb.Patient_Link{
							{},
						},
					},
				},
			},
			wantOutcome: &errorreporter.MultiVersionOperationOutcome{
				Version: fhirversion.DSTU2,
				R2Outcome: &r2pb.OperationOutcome{
					Issue: []*r2pb.OperationOutcome_Issue{
						&r2pb.OperationOutcome_Issue{
							Code: &c2pb.IssueTypeCode{
								Value: c2pb.IssueTypeCode_VALUE,
							},
							Severity: &c2pb.IssueSeverityCode{
								Value: c2pb.IssueSeverityCode_ERROR,
							},
							Diagnostics: &d2pb.String{Value: `error at "Patient.link[0]": missing required field "other"`},
							Location: []*d2pb.String{
								&d2pb.String{Value: `Patient.link[0]`},
							},
						},
						&r2pb.OperationOutcome_Issue{
							Code: &c2pb.IssueTypeCode{
								Value: c2pb.IssueTypeCode_VALUE,
							},
							Severity: &c2pb.IssueSeverityCode{
								Value: c2pb.IssueSeverityCode_ERROR,
							},
							Diagnostics: &d2pb.String{Value: `error at "Patient.link[0]": missing required field "type"`},
							Location: []*d2pb.String{
								&d2pb.String{Value: `Patient.link[0]`},
							},
						},
					},
				},
			},
		},
		{
			name: "invalid reference type",
			msg: &r2pb.ContainedResource{
				OneofResource: &r2pb.ContainedResource_Patient{
					Patient: &r2pb.Patient{
						ManagingOrganization: &d2pb.Reference{
							Reference: &d2pb.Reference_PatientId{
								PatientId: &d2pb.ReferenceId{Value: "2"},
							},
						},
					},
				},
			},
			wantOutcome: &errorreporter.MultiVersionOperationOutcome{
				Version: fhirversion.DSTU2,
				R2Outcome: &r2pb.OperationOutcome{
					Issue: []*r2pb.OperationOutcome_Issue{
						&r2pb.OperationOutcome_Issue{
							Code: &c2pb.IssueTypeCode{
								Value: c2pb.IssueTypeCode_VALUE,
							},
							Severity: &c2pb.IssueSeverityCode{
								Value: c2pb.IssueSeverityCode_ERROR,
							},
							Diagnostics: &d2pb.String{Value: `error at "Patient.managingOrganization": invalid reference to a Patient resource, want Organization`},
							Location: []*d2pb.String{
								&d2pb.String{Value: `Patient.managingOrganization`},
							},
						},
					},
				},
			},
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			oer := errorreporter.NewOperationErrorReporter(fhirversion.DSTU2)
			err := ValidateWithErrorReporter(test.msg, oer)
			if err != nil {
				t.Fatalf("unmarshal with outcome %v failed: %v", test.name, err)
			}
			if diff := cmp.Diff(oer.Outcome, test.wantOutcome, protocmp.Transform()); diff != "" {
				t.Errorf("unmarshal with outcome %v: got outcome %v, want %v", test.name, oer.Outcome, test.wantOutcome)
			}
		})
	}
}
