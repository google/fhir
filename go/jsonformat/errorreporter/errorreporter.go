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
	"fmt"

	"github.com/google/fhir/go/fhirversion"

	c4pb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/codes_go_proto"
	d4pb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/datatypes_go_proto"
	r4outcomepb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/operation_outcome_go_proto"
	c3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/codes_go_proto"
	d3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/datatypes_go_proto"
	r3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/resources_go_proto"
)

// IssueSeverityCode describes the severity of an operation output issue.
type IssueSeverityCode string

// IssueTypeCode describes the type of an operation output issue.
type IssueTypeCode string

// FHIR versions and Issue codes for operation outcome.
const (
	IssueSeverityWarning = IssueSeverityCode("warning")
	IssueSeverityError   = IssueSeverityCode("error")
	ValueIssueTypeCode   = IssueTypeCode("VALUE")
)

var (
	// R3IssueSeverityCodeMap maps IssueSeverityCode to R3IssueSeverityCode
	R3IssueSeverityCodeMap = map[IssueSeverityCode]c3pb.IssueSeverityCode_Value{
		IssueSeverityError:   c3pb.IssueSeverityCode_ERROR,
		IssueSeverityWarning: c3pb.IssueSeverityCode_WARNING,
	}
	// R3OutcomeCodeMap maps IssueTypeCode to R3IssueTypeCode_Value
	R3OutcomeCodeMap = map[IssueTypeCode]c3pb.IssueTypeCode_Value{
		ValueIssueTypeCode: c3pb.IssueTypeCode_VALUE,
	}
	// R4IssueSeverityCodeMap maps IssueSeverityCode to R4IssueSeverityCode
	R4IssueSeverityCodeMap = map[IssueSeverityCode]c4pb.IssueSeverityCode_Value{
		IssueSeverityError:   c4pb.IssueSeverityCode_ERROR,
		IssueSeverityWarning: c4pb.IssueSeverityCode_WARNING,
	}
	// R4OutcomeCodeMap maps IssueTypeCode to R4IssueTypeCode_Value
	R4OutcomeCodeMap = map[IssueTypeCode]c4pb.IssueTypeCode_Value{
		ValueIssueTypeCode: c4pb.IssueTypeCode_VALUE,
	}
)

// An ErrorReporter can be used to handle validation errors in the manner
// of the caller's choosing.
type ErrorReporter interface {
	// ReportValidationError reports validation errors occurred during validation.
	//
	// If the error can be satisfactorily reported it should return nil, instructing
	// the FHIR validation logic to proceed.
	//
	// Conversely, if this returns an error, it means an error was encountered while
	// attempting to handle the original error, indicating a failure mode of the
	// ErrorReporter itself.
	ReportValidationError(elementPath string, err error) error
	// ReportValidationError reports validation warning occurred during validation.
	//
	// If the error can be satisfactorily reported, it should return nil, instructing
	// the FHIR validation logic to proceed.
	//
	// Conversely, if this returns an error, it means an error was encountered while
	// attempting to handle the original error, indicating a failure mode of the
	// ErrorReporter itself.
	ReportValidationWarning(elementPath string, err error) error
}

// MultiVersionOperationOutcome encompasses Operations of multiple FHIR versions.
type MultiVersionOperationOutcome struct {
	Version   fhirversion.Version
	R3Outcome *r3pb.OperationOutcome
	R4Outcome *r4outcomepb.OperationOutcome
}

// OperationErrorReporter is an implementation of ErrorReporter. It makes
// validation errors visible to callers by preserving errors in a
// MultiVersionOperationOutcome.
// TODO: Add examples
type OperationErrorReporter struct {
	Outcome *MultiVersionOperationOutcome
}

// NewOperationErrorReporter returns an OperationErrorRerporter with specified
// fhir version.
func NewOperationErrorReporter(ver fhirversion.Version) *OperationErrorReporter {
	outcome := &MultiVersionOperationOutcome{Version: ver}
	switch ver {
	case fhirversion.STU3:
		outcome.R3Outcome = &r3pb.OperationOutcome{}
	case fhirversion.R4:
		outcome.R4Outcome = &r4outcomepb.OperationOutcome{}
	}
	return &OperationErrorReporter{
		Outcome: outcome,
	}
}

// ReportValidationError reports an issue at "Error" severity, indicating that the operation should
// be considered a failure.
func (oe *OperationErrorReporter) ReportValidationError(elementPath string, err error) error {
	return oe.report(elementPath, err, ValueIssueTypeCode, IssueSeverityError)
}

// ReportValidationWarning reports an issue at "Warning" severity, indicating that the operation
// encountered an issue, but can still be considered successful.
func (oe *OperationErrorReporter) ReportValidationWarning(elementPath string, err error) error {
	return oe.report(elementPath, err, ValueIssueTypeCode, IssueSeverityWarning)
}

func (oe *OperationErrorReporter) report(elementPath string, err error, typeCode IssueTypeCode, severity IssueSeverityCode) error {
	switch oe.Outcome.Version {
	case fhirversion.STU3:
		issues := oe.Outcome.R3Outcome.GetIssue()
		issue := &r3pb.OperationOutcome_Issue{
			Code: &c3pb.IssueTypeCode{
				Value: R3OutcomeCodeMap[typeCode],
			},
			Severity: &c3pb.IssueSeverityCode{
				Value: R3IssueSeverityCodeMap[severity],
			},
			Diagnostics: &d3pb.String{Value: err.Error()},
		}
		if elementPath != "" {
			issue.Expression = append(issue.Expression, &d3pb.String{Value: elementPath})
		}
		oe.Outcome.R3Outcome.Issue = append(issues, issue)
	case fhirversion.R4:
		issues := oe.Outcome.R4Outcome.GetIssue()
		issue := &r4outcomepb.OperationOutcome_Issue{
			Code: &r4outcomepb.OperationOutcome_Issue_CodeType{
				Value: R4OutcomeCodeMap[typeCode],
			},
			Severity: &r4outcomepb.OperationOutcome_Issue_SeverityCode{
				Value: R4IssueSeverityCodeMap[severity],
			},
			Diagnostics: &d4pb.String{Value: err.Error()},
		}
		if elementPath != "" {
			issue.Expression = append(issue.Expression, &d4pb.String{Value: elementPath})
		}
		oe.Outcome.R4Outcome.Issue = append(issues, issue)
	default:
		return fmt.Errorf("unsupported FHIR version %s", oe.Outcome.Version)
	}
	return nil
}

// BasicErrorReporter simply stores all errors during valudation.
//
// This is primarily for legacy use; most users should use an OperationOutcomeErrorReporter
// or their own implementation.
type BasicErrorReporter struct {
	Errors []*error
}

// NewBasicErrorReporter creates a basic error reporter
func NewBasicErrorReporter() *BasicErrorReporter {
	return &BasicErrorReporter{}
}

// ReportValidationError stores the error by appending it to the errors field.
func (be *BasicErrorReporter) ReportValidationError(_ string, err error) error {
	be.Errors = append(be.Errors, &err)
	return nil
}

// ReportValidationWarning does nothing.
func (be *BasicErrorReporter) ReportValidationWarning(elementPath string, err error) error {
	return nil
}
