// Copyright 2021 Google LLC
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

// Package errorreporter makes all validation errors visible to callers
// so they can report or handle them as appropriate for the surrounding
// system.
package errorreporter

import (
	"testing"

	"github.com/google/fhir/go/fhirversion"
	"github.com/google/fhir/go/jsonformat/internal/jsonpbhelper"
	c4pb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/codes_go_proto"
	d4pb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/datatypes_go_proto"
	r4outcomepb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/operation_outcome_go_proto"
	c5pb "github.com/google/fhir/go/proto/google/fhir/proto/r5/core/codes_go_proto"
	d5pb "github.com/google/fhir/go/proto/google/fhir/proto/r5/core/datatypes_go_proto"
	r5outcomepb "github.com/google/fhir/go/proto/google/fhir/proto/r5/core/resources/operation_outcome_go_proto"
	c3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/codes_go_proto"
	d3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/datatypes_go_proto"
	r3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/resources_go_proto"
	"github.com/google/go-cmp/cmp"
	"google.golang.org/protobuf/testing/protocmp"
)

var (
	detail1      = `invalid reference to a Patient resource, want Organization`
	detail2      = `FHIRPath validation failed: error at "Patient.contact[0]": failed FHIRPath constraint`
	detail3      = `missing required field "other"`
	elementPath1 = `Patient.managingOrganization`
	elementPath2 = `Patient.contact[0]`
	elementPath3 = `Patient.link[0]`
)

func TestReportValidationError(t *testing.T) {
	tests := []struct {
		name string
		err  error
		ver  fhirversion.Version
		want *MultiVersionOperationOutcome
	}{
		{
			name: "r3 ErrorReporter",
			err:  &jsonpbhelper.UnmarshalError{Details: detail1},
			ver:  fhirversion.STU3,
			want: &MultiVersionOperationOutcome{
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
							Diagnostics: &d3pb.String{Value: detail1},
							Expression: []*d3pb.String{
								&d3pb.String{Value: elementPath1},
							},
						},
					},
				},
			},
		},
		{
			name: "r4 ErrorReporter",
			err:  &jsonpbhelper.UnmarshalError{Details: detail1},
			ver:  fhirversion.R4,
			want: &MultiVersionOperationOutcome{
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
							Diagnostics: &d4pb.String{Value: detail1},
							Expression: []*d4pb.String{
								&d4pb.String{Value: elementPath1},
							},
						},
					},
				},
			},
		},
		{
			name: "r5 ErrorReporter",
			err:  &jsonpbhelper.UnmarshalError{Details: detail1},
			ver:  fhirversion.R5,
			want: &MultiVersionOperationOutcome{
				Version: fhirversion.R5,
				R5Outcome: &r5outcomepb.OperationOutcome{
					Issue: []*r5outcomepb.OperationOutcome_Issue{
						&r5outcomepb.OperationOutcome_Issue{
							Code: &r5outcomepb.OperationOutcome_Issue_CodeType{
								Value: c5pb.IssueTypeCode_VALUE,
							},
							Severity: &r5outcomepb.OperationOutcome_Issue_SeverityCode{
								Value: c5pb.IssueSeverityCode_ERROR,
							},
							Diagnostics: &d5pb.String{Value: detail1},
							Expression: []*d5pb.String{
								&d5pb.String{Value: elementPath1},
							},
						},
					},
				},
			},
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			oer := NewOperationErrorReporter(test.ver)
			err := oer.ReportValidationError(elementPath1, test.err)
			if err != nil {
				t.Fatalf("Error occured during ReportValidationError: %v", err)
			}
			if diff := cmp.Diff(test.want, oer.Outcome, protocmp.Transform()); diff != "" {
				t.Errorf("ErrorReporter returned unexpected diff (-want +got):\n%s", diff)
			}
		})
	}
}

func TestReportValidationWarning(t *testing.T) {
	tests := []struct {
		name string
		err  error
		ver  fhirversion.Version
		want *MultiVersionOperationOutcome
	}{
		{
			name: "r3 ErrorReporter",
			err:  &jsonpbhelper.UnmarshalError{Details: detail1},
			ver:  fhirversion.STU3,
			want: &MultiVersionOperationOutcome{
				Version: fhirversion.STU3,
				R3Outcome: &r3pb.OperationOutcome{
					Issue: []*r3pb.OperationOutcome_Issue{
						&r3pb.OperationOutcome_Issue{
							Code: &c3pb.IssueTypeCode{
								Value: c3pb.IssueTypeCode_VALUE,
							},
							Severity: &c3pb.IssueSeverityCode{
								Value: c3pb.IssueSeverityCode_WARNING,
							},
							Diagnostics: &d3pb.String{Value: detail1},
							Expression: []*d3pb.String{
								&d3pb.String{Value: elementPath1},
							},
						},
					},
				},
			},
		},
		{
			name: "r4 ErrorReporter",
			err:  &jsonpbhelper.UnmarshalError{Details: detail1},
			ver:  fhirversion.R4,
			want: &MultiVersionOperationOutcome{
				Version: fhirversion.R4,
				R4Outcome: &r4outcomepb.OperationOutcome{
					Issue: []*r4outcomepb.OperationOutcome_Issue{
						&r4outcomepb.OperationOutcome_Issue{
							Code: &r4outcomepb.OperationOutcome_Issue_CodeType{
								Value: c4pb.IssueTypeCode_VALUE,
							},
							Severity: &r4outcomepb.OperationOutcome_Issue_SeverityCode{
								Value: c4pb.IssueSeverityCode_WARNING,
							},
							Diagnostics: &d4pb.String{Value: detail1},
							Expression: []*d4pb.String{
								&d4pb.String{Value: elementPath1},
							},
						},
					},
				},
			},
		},
		{
			name: "r5 ErrorReporter",
			err:  &jsonpbhelper.UnmarshalError{Details: detail1},
			ver:  fhirversion.R5,
			want: &MultiVersionOperationOutcome{
				Version: fhirversion.R5,
				R5Outcome: &r5outcomepb.OperationOutcome{
					Issue: []*r5outcomepb.OperationOutcome_Issue{
						&r5outcomepb.OperationOutcome_Issue{
							Code: &r5outcomepb.OperationOutcome_Issue_CodeType{
								Value: c5pb.IssueTypeCode_VALUE,
							},
							Severity: &r5outcomepb.OperationOutcome_Issue_SeverityCode{
								Value: c5pb.IssueSeverityCode_WARNING,
							},
							Diagnostics: &d5pb.String{Value: detail1},
							Expression: []*d5pb.String{
								&d5pb.String{Value: elementPath1},
							},
						},
					},
				},
			},
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			oer := NewOperationErrorReporter(test.ver)
			err := oer.ReportValidationWarning(elementPath1, test.err)
			if err != nil {
				t.Fatalf("Error occured during ReportValidationWarning: %v", err)
			}
			if diff := cmp.Diff(test.want, oer.Outcome, protocmp.Transform()); diff != "" {
				t.Errorf("ErrorReporter returned unexpected diff (-want +got):\n%s", diff)
			}
		})
	}
}

func TestReportValidationError_Accumulate(t *testing.T) {
	tests := []struct {
		name         string
		err          jsonpbhelper.UnmarshalErrorList
		elementPaths []string
		ver          fhirversion.Version
		want         *MultiVersionOperationOutcome
	}{
		{
			name:         "r3 ErrorReporter, multiple errors",
			elementPaths: []string{elementPath1, elementPath2, elementPath3},
			err: jsonpbhelper.UnmarshalErrorList{
				&jsonpbhelper.UnmarshalError{
					Details: detail1,
				},
				&jsonpbhelper.UnmarshalError{
					Details: detail2,
				},
				&jsonpbhelper.UnmarshalError{
					Details: detail3,
				},
			},
			ver: fhirversion.STU3,
			want: &MultiVersionOperationOutcome{
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
							Diagnostics: &d3pb.String{Value: detail1},
							Expression: []*d3pb.String{
								&d3pb.String{Value: elementPath1},
							},
						},
						&r3pb.OperationOutcome_Issue{
							Code: &c3pb.IssueTypeCode{
								Value: c3pb.IssueTypeCode_VALUE,
							},
							Severity: &c3pb.IssueSeverityCode{
								Value: c3pb.IssueSeverityCode_ERROR,
							},
							Diagnostics: &d3pb.String{Value: detail2},
							Expression: []*d3pb.String{
								&d3pb.String{Value: elementPath2},
							},
						},
						&r3pb.OperationOutcome_Issue{
							Code: &c3pb.IssueTypeCode{
								Value: c3pb.IssueTypeCode_VALUE,
							},
							Severity: &c3pb.IssueSeverityCode{
								Value: c3pb.IssueSeverityCode_ERROR,
							},
							Diagnostics: &d3pb.String{Value: detail3},
							Expression: []*d3pb.String{
								&d3pb.String{Value: elementPath3},
							},
						},
					},
				},
			},
		},
		{
			name:         "r4 ErrorReporter, multiple errors",
			elementPaths: []string{elementPath1, elementPath2, elementPath3},
			err: jsonpbhelper.UnmarshalErrorList{
				&jsonpbhelper.UnmarshalError{
					Details: detail1,
				},
				&jsonpbhelper.UnmarshalError{
					Details: detail2,
				},
				&jsonpbhelper.UnmarshalError{
					Details: detail3,
				},
			},
			ver: fhirversion.R4,
			want: &MultiVersionOperationOutcome{
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
							Diagnostics: &d4pb.String{Value: detail1},
							Expression: []*d4pb.String{
								&d4pb.String{Value: elementPath1},
							},
						},
						&r4outcomepb.OperationOutcome_Issue{
							Code: &r4outcomepb.OperationOutcome_Issue_CodeType{
								Value: c4pb.IssueTypeCode_VALUE,
							},
							Severity: &r4outcomepb.OperationOutcome_Issue_SeverityCode{
								Value: c4pb.IssueSeverityCode_ERROR,
							},
							Diagnostics: &d4pb.String{Value: detail2},
							Expression: []*d4pb.String{
								&d4pb.String{Value: elementPath2},
							},
						},
						&r4outcomepb.OperationOutcome_Issue{
							Code: &r4outcomepb.OperationOutcome_Issue_CodeType{
								Value: c4pb.IssueTypeCode_VALUE,
							},
							Severity: &r4outcomepb.OperationOutcome_Issue_SeverityCode{
								Value: c4pb.IssueSeverityCode_ERROR,
							},
							Diagnostics: &d4pb.String{Value: detail3},
							Expression: []*d4pb.String{
								&d4pb.String{Value: elementPath3},
							},
						},
					},
				},
			},
		},
		{
			name:         "r5 ErrorReporter, multiple errors",
			elementPaths: []string{elementPath1, elementPath2, elementPath3},
			err: jsonpbhelper.UnmarshalErrorList{
				&jsonpbhelper.UnmarshalError{
					Details: detail1,
				},
				&jsonpbhelper.UnmarshalError{
					Details: detail2,
				},
				&jsonpbhelper.UnmarshalError{
					Details: detail3,
				},
			},
			ver: fhirversion.R5,
			want: &MultiVersionOperationOutcome{
				Version: fhirversion.R5,
				R5Outcome: &r5outcomepb.OperationOutcome{
					Issue: []*r5outcomepb.OperationOutcome_Issue{
						&r5outcomepb.OperationOutcome_Issue{
							Code: &r5outcomepb.OperationOutcome_Issue_CodeType{
								Value: c5pb.IssueTypeCode_VALUE,
							},
							Severity: &r5outcomepb.OperationOutcome_Issue_SeverityCode{
								Value: c5pb.IssueSeverityCode_ERROR,
							},
							Diagnostics: &d5pb.String{Value: detail1},
							Expression: []*d5pb.String{
								&d5pb.String{Value: elementPath1},
							},
						},
						&r5outcomepb.OperationOutcome_Issue{
							Code: &r5outcomepb.OperationOutcome_Issue_CodeType{
								Value: c5pb.IssueTypeCode_VALUE,
							},
							Severity: &r5outcomepb.OperationOutcome_Issue_SeverityCode{
								Value: c5pb.IssueSeverityCode_ERROR,
							},
							Diagnostics: &d5pb.String{Value: detail2},
							Expression: []*d5pb.String{
								&d5pb.String{Value: elementPath2},
							},
						},
						&r5outcomepb.OperationOutcome_Issue{
							Code: &r5outcomepb.OperationOutcome_Issue_CodeType{
								Value: c5pb.IssueTypeCode_VALUE,
							},
							Severity: &r5outcomepb.OperationOutcome_Issue_SeverityCode{
								Value: c5pb.IssueSeverityCode_ERROR,
							},
							Diagnostics: &d5pb.String{Value: detail3},
							Expression: []*d5pb.String{
								&d5pb.String{Value: elementPath3},
							},
						},
					},
				},
			},
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			oer := NewOperationErrorReporter(test.ver)
			for i, umerr := range test.err {
				err := oer.ReportValidationError(test.elementPaths[i], umerr)
				if err != nil {
					t.Fatalf("Error occured during ReportValidationError: %v", err)
				}
			}
			if diff := cmp.Diff(test.want, oer.Outcome, protocmp.Transform()); diff != "" {
				t.Errorf("ErrorReporter returned unexpected diff (-want +got):\n%s", diff)
			}
		})
	}
}
