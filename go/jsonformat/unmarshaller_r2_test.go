// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//	http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
package jsonformat

import (
	"encoding/json"
	"math"
	"strconv"
	"testing"

	"github.com/google/fhir/go/fhirversion"
	"github.com/google/fhir/go/jsonformat/errorreporter"
	"github.com/google/fhir/go/jsonformat/internal/jsonpbhelper"
	c2pb "github.com/google/fhir/go/proto/google/fhir/proto/dstu2/codes_go_proto"
	d2pb "github.com/google/fhir/go/proto/google/fhir/proto/dstu2/datatypes_go_proto"
	r2mpb "github.com/google/fhir/go/proto/google/fhir/proto/dstu2/metadatatypes_go_proto"
	r2pb "github.com/google/fhir/go/proto/google/fhir/proto/dstu2/resources_go_proto"
	"github.com/google/go-cmp/cmp"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/reflect/protoreflect"
	"google.golang.org/protobuf/testing/protocmp"
)

// LINT.IfChange
func TestUnmarshal_R2(t *testing.T) {
	tests := []struct {
		name string
		json []byte
		want *r2pb.ContainedResource
	}{
		{
			name: "R2 SearchParameter",
			json: []byte(`
    {
      "resourceType": "SearchParameter",
			"url": "http://example.com/SearchParameter",
			"name": "Search-parameter",
			"status": "active",
			"description": "custom search parameter",
			"code": "value-quantity",
			"base": "Observation",
			"type": "number"
    }`),
			want: &r2pb.ContainedResource{
				OneofResource: &r2pb.ContainedResource_SearchParameter{
					SearchParameter: &r2pb.SearchParameter{
						Url:         &d2pb.Uri{Value: "http://example.com/SearchParameter"},
						Name:        &d2pb.String{Value: "Search-parameter"},
						Status:      &c2pb.ConformanceResourceStatusCode{Value: c2pb.ConformanceResourceStatusCode_ACTIVE},
						Description: &d2pb.String{Value: "custom search parameter"},
						Code:        &d2pb.Code{Value: "value-quantity"},
						Base:        &c2pb.ResourceTypeCode{Value: c2pb.ResourceTypeCode_OBSERVATION},
						Type:        &c2pb.SearchParamTypeCode{Value: c2pb.SearchParamTypeCode_NUMBER},
					},
				},
			},
		},
		{
			name: "Observation",
			json: []byte(`
    {
      "resourceType": "Observation",
      "id": "example",
			"status": "final",
			"code": {
				"text": "test"
			},
      "text": {
        "status": "generated",
        "div": "<div xmlns=\"http://www.w3.org/1999/xhtml\">[Put rendering here]</div>"
      }
    }`),
			want: &r2pb.ContainedResource{
				OneofResource: &r2pb.ContainedResource_Observation{
					Observation: &r2pb.Observation{
						Id: &d2pb.Id{
							Value: "example",
						},
						Status: &c2pb.ObservationStatusCode{Value: c2pb.ObservationStatusCode_FINAL},
						Code:   &d2pb.CodeableConcept{Text: &d2pb.String{Value: "test"}},
						Text: &r2mpb.Narrative{
							Status: &c2pb.NarrativeStatusCode{
								Value: c2pb.NarrativeStatusCode_GENERATED,
							},
							Div: &d2pb.Xhtml{
								Value: `<div xmlns="http://www.w3.org/1999/xhtml">[Put rendering here]</div>`,
							},
						},
					},
				},
			},
		},
		{
			name: "Patient",
			json: []byte(`
    {
      "resourceType": "Patient",
      "multipleBirthBoolean": false
    }`),
			want: &r2pb.ContainedResource{
				OneofResource: &r2pb.ContainedResource_Patient{
					Patient: &r2pb.Patient{
						MultipleBirth: &r2pb.Patient_MultipleBirth{
							MultipleBirth: &r2pb.Patient_MultipleBirth_Boolean{
								Boolean: &d2pb.Boolean{Value: false}}},
					},
				},
			},
		},
		{
			name: "RepeatedResources",
			json: []byte(`
    {
		  "resourceType": "Bundle",
			"type": "collection",
      "entry": [
        {
          "resource": {
            "resourceType": "Patient"
          }
        },
        {
          "resource": {
            "resourceType": "Patient"
          }
        }
      ]
    }`),
			want: &r2pb.ContainedResource{
				OneofResource: &r2pb.ContainedResource_Bundle{
					Bundle: &r2pb.Bundle{
						Type: &c2pb.BundleTypeCode{Value: c2pb.BundleTypeCode_COLLECTION},
						Entry: []*r2pb.Bundle_Entry{{
							Resource: &r2pb.ContainedResource{
								OneofResource: &r2pb.ContainedResource_Patient{
									Patient: &r2pb.Patient{},
								},
							}},
							{Resource: &r2pb.ContainedResource{
								OneofResource: &r2pb.ContainedResource_Patient{
									Patient: &r2pb.Patient{},
								},
							}},
						},
					},
				},
			},
		},
		{
			name: "RepeatedPrimitiveExtension",
			json: []byte(`
    {
      "resourceType": "Patient",
      "name": [
        {
          "given": [
            "Toby"
          ],
          "_given": [{
            "id": "a3",
            "extension": [{
                "url": "http://hl7.org/fhir/StructureDefinition/qualifier",
                "valueCode": "MID"
            }]
          }]
        }
      ]
    }`),
			want: &r2pb.ContainedResource{
				OneofResource: &r2pb.ContainedResource_Patient{
					Patient: &r2pb.Patient{
						Name: []*d2pb.HumanName{{
							Given: []*d2pb.String{{
								Value: "Toby",
								Id: &d2pb.Id{
									Value: "a3",
								},
								Extension: []*d2pb.Extension{{
									Url: &d2pb.Uri{
										Value: "http://hl7.org/fhir/StructureDefinition/qualifier",
									},
									Value: &d2pb.Extension_ValueX{
										Choice: &d2pb.Extension_ValueX_Code{
											Code: &d2pb.Code{
												Value: "MID",
											},
										},
									},
								}},
							}},
						}},
					},
				},
			},
		},
		{
			name: "RepeatedPrimitiveExtensionNoValue",
			json: []byte(`
        {
          "resourceType": "Patient",
          "name": [
            {
              "given": [{
                "id": "a3",
								"extension": [{
									"url": "http://hl7.org/fhir/StructureDefinition/ext1",
									"valueCode": "value1"
                }]
              }],
							"_given": [{
								"extension": [{
									"url": "http://hl7.org/fhir/StructureDefinition/ext2",
									"valueCode": "value2"
								}]
							}]
            }
          ]
        }`),
			want: &r2pb.ContainedResource{
				OneofResource: &r2pb.ContainedResource_Patient{
					Patient: &r2pb.Patient{
						Name: []*d2pb.HumanName{{
							Given: []*d2pb.String{{
								Id: &d2pb.Id{Value: "a3"},
								Extension: []*d2pb.Extension{
									{
										Url: &d2pb.Uri{
											Value: "http://hl7.org/fhir/StructureDefinition/ext1",
										},
										Value: &d2pb.Extension_ValueX{
											Choice: &d2pb.Extension_ValueX_Code{
												Code: &d2pb.Code{Value: "value1"},
											},
										},
									},
									{
										Url: &d2pb.Uri{
											Value: "http://hl7.org/fhir/StructureDefinition/ext2",
										},
										Value: &d2pb.Extension_ValueX{
											Choice: &d2pb.Extension_ValueX_Code{
												Code: &d2pb.Code{Value: "value2"},
											},
										},
									},
									{
										Url: &d2pb.Uri{
											Value: "https://g.co/fhir/StructureDefinition/primitiveHasNoValue",
										},
										Value: &d2pb.Extension_ValueX{
											Choice: &d2pb.Extension_ValueX_Boolean{
												Boolean: &d2pb.Boolean{
													Value: true,
												},
											},
										},
									}},
							}},
						}},
					},
				},
			},
		},
		{
			name: "PrimitiveExtension",
			json: []byte(`
    {
      "resourceType": "Patient",
      "gender": "other",
			"_gender": {
				"extension": [{
						"url": "http://hl7.org/fhir/StructureDefinition/patient-genderIdentity",
						"valueCode": "non-binary"
				}]
			}
    }`),
			want: &r2pb.ContainedResource{
				OneofResource: &r2pb.ContainedResource_Patient{
					Patient: &r2pb.Patient{
						Gender: &c2pb.AdministrativeGenderCode{
							Value: c2pb.AdministrativeGenderCode_OTHER,
							Extension: []*d2pb.Extension{{
								Url: &d2pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition/patient-genderIdentity"},
								Value: &d2pb.Extension_ValueX{
									Choice: &d2pb.Extension_ValueX_Code{
										Code: &d2pb.Code{
											Value: "non-binary",
										},
									},
								},
							}},
						},
					},
				},
			},
		},
		{
			name: "DataElement",
			json: []byte(`
			{
			 "status":"draft",
			 "element":[
					{
						 "path":"path",
						 "maxValueUnsignedInt":92,
						 "max":"10"
					}
			 ],
			 "resourceType":"DataElement"
      }`),
			want: &r2pb.ContainedResource{
				OneofResource: &r2pb.ContainedResource_DataElement{
					DataElement: &r2pb.DataElement{
						Status: &c2pb.ConformanceResourceStatusCode{Value: c2pb.ConformanceResourceStatusCode_DRAFT},
						Element: []*r2mpb.ElementDefinition{
							{
								Path: &d2pb.String{Value: "path"},
								MaxValue: &r2mpb.ElementDefinition_MaxValue{
									MaxValue: &r2mpb.ElementDefinition_MaxValue_UnsignedInt{UnsignedInt: &d2pb.UnsignedInt{Value: 92}},
								},
								Max: &d2pb.String{Value: "10"},
							},
						},
					},
				},
			},
		},
		{
			name: "inline resource",
			json: []byte(`{
	"contained":[
		{
			"id":"nested",
			"resourceType":"Patient",
			"contained":[
				{
					"id":"double-nested",
					"resourceType":"Patient"
				}
			]
		}
	],
	"id":"example",
	"resourceType":"Patient"
}`),
			want: &r2pb.ContainedResource{
				OneofResource: &r2pb.ContainedResource_Patient{
					Patient: &r2pb.Patient{
						Id: &d2pb.Id{Value: "example"},
						Contained: []*r2pb.ContainedResource{
							{
								OneofResource: &r2pb.ContainedResource_Patient{
									Patient: &r2pb.Patient{
										Id: &d2pb.Id{Value: "nested"},
										Contained: []*r2pb.ContainedResource{
											{
												OneofResource: &r2pb.ContainedResource_Patient{
													Patient: &r2pb.Patient{
														Id: &d2pb.Id{Value: "double-nested"},
													},
												},
											},
										},
									},
								},
							},
						},
					},
				},
			},
		},
		{
			name: "identifier reference",
			json: []byte(`
    {
      "resourceType": "Patient",
      "managingOrganization": {
        "identifier": {
					"value": "myorg"
				}
			}
    }`),
			want: &r2pb.ContainedResource{
				OneofResource: &r2pb.ContainedResource_Patient{
					Patient: &r2pb.Patient{
						ManagingOrganization: &d2pb.Reference{
							Identifier: &d2pb.Identifier{
								Value: &d2pb.String{
									Value: "myorg",
								},
							},
						},
					},
				},
			},
		},
		// TODO(b/161479338): remove test once upper camel case fields are rejected.
		{
			name: "upper camel case is valid",
			json: []byte(`
			{
			  "Id":"example",
			  "resourceType": "Patient",
				"Gender": "female",
				"Extension": [
					{
						"Url": "http://nema.org/fhir/extensions#0010:1020",
						"ValueQuantity": {
							"Value": 1.83,
							"Unit": "m"
						}
					}
				],
				"Identifier": [
					{
						"System": "http://nema.org/examples/patients",
						"Value": "1234"
					}
				],
				"BirthDate": "2016-05-18",
				"_BirthDate": {
					"Id": "12345",
					"Extension": [
						{
							"Url": "http://hl7.org/fhir/StructureDefinition/patient-birthTime",
							"ValueDateTime": "2016-05-18T10:28:45Z",
							"_ValueDateTime": {
								"Extension": [{
									"Url": "http://example.com/fhir/extension",
									"ValueDateTime": "2016-05-18T10:28:45Z"
								}]
							}
						}
					]
				}
			}`),
			want: &r2pb.ContainedResource{
				OneofResource: &r2pb.ContainedResource_Patient{
					Patient: &r2pb.Patient{
						Id: &d2pb.Id{
							Value: "example",
						},
						Gender: &c2pb.AdministrativeGenderCode{
							Value: c2pb.AdministrativeGenderCode_FEMALE,
						},
						Extension: []*d2pb.Extension{{
							Url: &d2pb.Uri{Value: "http://nema.org/fhir/extensions#0010:1020"},
							Value: &d2pb.Extension_ValueX{
								Choice: &d2pb.Extension_ValueX_Quantity{
									Quantity: &d2pb.Quantity{Value: &d2pb.Decimal{Value: "1.83"}, Unit: &d2pb.String{Value: "m"}},
								},
							},
						}},
						Identifier: []*d2pb.Identifier{{
							System: &d2pb.Uri{
								Value: "http://nema.org/examples/patients",
							},
							Value: &d2pb.String{Value: "1234"},
						}},
						BirthDate: &d2pb.Date{
							Id:        &d2pb.Id{Value: "12345"},
							ValueUs:   1463554800000000,
							Timezone:  "America/Los_Angeles",
							Precision: d2pb.Date_DAY,
							Extension: []*d2pb.Extension{{
								Url: &d2pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition/patient-birthTime"},
								Value: &d2pb.Extension_ValueX{
									Choice: &d2pb.Extension_ValueX_DateTime{
										DateTime: &d2pb.DateTime{
											ValueUs:   1463567325000000,
											Timezone:  "Z",
											Precision: d2pb.DateTime_SECOND,
											Extension: []*d2pb.Extension{{
												Url: &d2pb.Uri{Value: "http://example.com/fhir/extension"},
												Value: &d2pb.Extension_ValueX{
													Choice: &d2pb.Extension_ValueX_DateTime{
														DateTime: &d2pb.DateTime{
															ValueUs:   1463567325000000,
															Timezone:  "Z",
															Precision: d2pb.DateTime_SECOND,
														},
													},
												},
											}},
										},
									},
								},
							}},
						},
					},
				},
			},
		},
		{
			name: "trailing whitespace",
			json: []byte("{\"resourceType\": \"Patient\"} \t\n"),
			want: &r2pb.ContainedResource{
				OneofResource: &r2pb.ContainedResource_Patient{
					Patient: &r2pb.Patient{},
				},
			},
		},
		{
			name: "data absent extension",
			json: []byte(`{
        "resourceType": "Observation",
        "_status": {
          "extension": [{
            "url": "http://hl7.org/fhir/ValueSet/data-absent-reason",
            "valueCode": "unsupported"
          }]
        },
        "code": {
          "coding": []
        }
      }`),
			want: &r2pb.ContainedResource{
				OneofResource: &r2pb.ContainedResource_Observation{
					Observation: &r2pb.Observation{
						Status: &c2pb.ObservationStatusCode{
							Extension: []*d2pb.Extension{{
								Url: &d2pb.Uri{Value: jsonpbhelper.PrimitiveHasNoValueURL},
								Value: &d2pb.Extension_ValueX{
									Choice: &d2pb.Extension_ValueX_Boolean{
										Boolean: &d2pb.Boolean{Value: true},
									},
								},
							}, {
								Url: &d2pb.Uri{Value: "http://hl7.org/fhir/ValueSet/data-absent-reason"},
								Value: &d2pb.Extension_ValueX{
									Choice: &d2pb.Extension_ValueX_Code{
										Code: &d2pb.Code{Value: "unsupported"},
									},
								},
							}},
						},
						Code: &d2pb.CodeableConcept{
							Coding: []*d2pb.Coding{},
						},
					},
				},
			},
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			// test Unmarshal
			u := setupUnmarshaller(t, fhirversion.DSTU2)
			got, err := u.Unmarshal(test.json)
			if err != nil {
				t.Fatalf("unmarshal %v failed: %v", test.name, err)
			}
			sortExtensions := protocmp.SortRepeated(func(e1, e2 *d2pb.Extension) bool {
				return e1.GetUrl().GetValue() < e2.GetUrl().GetValue()
			})
			if diff := cmp.Diff(test.want, got, sortExtensions, protocmp.Transform()); diff != "" {
				t.Errorf("unmarshal %v, got diff(-want +got): %s", test.name, diff)
			}

			// test UnmarshalWithOutcome
			got, mvo, err := u.UnmarshalWithOutcome(test.json)
			if err != nil {
				t.Fatalf("unmarshalWithOutcome %v failed with unexpected error: %v", test.name, err)
			}
			if diff := cmp.Diff(test.want, got, sortExtensions, protocmp.Transform()); diff != "" {
				t.Errorf("unmarshalWithOutcome %v, got diff(-want +got): %s", test.name, diff)
			}
			ic := len(mvo.R2Outcome.GetIssue())
			if ic != 0 {
				t.Errorf("unmarshalWithOutcome %v got unexpected errors, expected issue number 0, got %v", test.name, ic)
			}

			// test UnmarshalWithErrorReporter
			er := errorreporter.NewOperationErrorReporter(fhirversion.DSTU2)
			got, err = u.UnmarshalWithErrorReporter(test.json, er)
			if err != nil {
				t.Fatalf("UnmarshalWithErrorReporter %v failed with unexpected error: %v", test.name, err)
			}
			if diff := cmp.Diff(test.want, got, sortExtensions, protocmp.Transform()); diff != "" {
				t.Errorf("UnmarshalWithErrorReporter %v, got diff(-want +got): %s", test.name, diff)
			}
			ic = len(er.Outcome.R2Outcome.GetIssue())
			if ic != 0 {
				t.Errorf("UnmarshalWithErrorReporter %v got unexpected errors, expected issue number 0, got %v", test.name, ic)
			}
		})
	}
}

func TestUnmarshal_FHIRComments(t *testing.T) {
	r2u := setupUnmarshaller(t, fhirversion.DSTU2)
	tests := []struct {
		name   string
		json   []byte
		r2want *r2pb.ContainedResource
	}{
		{
			"top level fhir_comments",
			[]byte(`
    {
      "resourceType": "Patient",
			"fhir_comments": ["c1", "c2"],
      "careProvider": [
        {
          "reference": "Practitioner/1"
        }
      ]
    }`),
			&r2pb.ContainedResource{
				OneofResource: &r2pb.ContainedResource_Patient{
					Patient: &r2pb.Patient{
						CareProvider: []*d2pb.Reference{
							{Reference: &d2pb.Reference_PractitionerId{PractitionerId: &d2pb.ReferenceId{Value: "1"}}},
						},
					},
				},
			},
		},
		{
			"nested fhir_comments",
			[]byte(`
    {
      "resourceType": "Patient",
      "careProvider": [
        {
          "reference": "Practitioner/1",
					"fhir_comments": ["c1", "c2"]
        }
      ]
    }`),
			&r2pb.ContainedResource{
				OneofResource: &r2pb.ContainedResource_Patient{
					Patient: &r2pb.Patient{
						CareProvider: []*d2pb.Reference{
							{Reference: &d2pb.Reference_PractitionerId{PractitionerId: &d2pb.ReferenceId{Value: "1"}}},
						},
					},
				},
			},
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			got, err := r2u.Unmarshal(test.json)
			if err != nil {
				t.Fatalf("unmarshal %s failed: %v", test.name, err)
			}
			if !proto.Equal(got, test.r2want) {
				t.Errorf("unmarshal %s: got %v, want %v", test.name, got, test.r2want)
			}
		})
	}
}

func TestUnmarshal_NoExtendedValidation_R2(t *testing.T) {
	tests := []struct {
		name string
		json []byte
		want proto.Message
	}{
		{
			name: "resource missing required fields",
			json: []byte(`
    {
      "resourceType": "Patient",
      "link": [{}]
    }`),
			want: &r2pb.ContainedResource{
				OneofResource: &r2pb.ContainedResource_Patient{
					Patient: &r2pb.Patient{
						Link: []*r2pb.Patient_Link{{}},
					},
				},
			},
		},
		{
			name: "invalid reference type",
			json: []byte(`
    {
      "resourceType": "Patient",
      "managingOrganization": {
				"reference": "Patient/1"
			}
    }`),
			want: &r2pb.ContainedResource{
				OneofResource: &r2pb.ContainedResource_Patient{
					Patient: &r2pb.Patient{
						ManagingOrganization: &d2pb.Reference{
							Reference: &d2pb.Reference_PatientId{
								PatientId: &d2pb.ReferenceId{Value: "1"},
							},
						},
					},
				},
			},
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			u := setupUnmarshaller(t, fhirversion.DSTU2)
			_, err := u.Unmarshal(test.json)
			if err == nil {
				t.Fatalf("unmarshal %v should have failed with extended validation", test.name)
			}

			u, err = NewUnmarshallerWithoutValidation("America/Los_Angeles", fhirversion.DSTU2)
			if err != nil {
				t.Fatalf("failed to create unmarshaler; %v", err)
			}
			got, err := u.Unmarshal(test.json)
			if err != nil {
				t.Fatalf("unmarshal %v failed: %v", test.name, err)
			}
			if !cmp.Equal(got, test.want, protocmp.Transform()) {
				t.Errorf("unmarshal %v: got %v, want %v", test.name, got, test.want)
			}
		})
	}
}

func TestUnmarshalWithOutcome_R2_Errors(t *testing.T) {
	tests := []struct {
		name        string
		json        []byte
		wantOutcome *errorreporter.MultiVersionOperationOutcome
		wantMsg     proto.Message
	}{
		{
			name: "invalid reference type",
			json: []byte(`
    {
      "resourceType": "Patient",
      "managingOrganization": {
				"reference": "Patient/1"
			}
    }`),
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
			wantMsg: &r2pb.ContainedResource{
				OneofResource: &r2pb.ContainedResource_Patient{
					Patient: &r2pb.Patient{
						ManagingOrganization: &d2pb.Reference{
							Reference: &d2pb.Reference_PatientId{
								PatientId: &d2pb.ReferenceId{Value: "1"},
							},
						},
					},
				},
			},
		},
		{
			name: "missing required field",
			json: []byte(`
    {
      "resourceType": "Patient",
      "link": [{}]
    }`),
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
			wantMsg: &r2pb.ContainedResource{
				OneofResource: &r2pb.ContainedResource_Patient{
					Patient: &r2pb.Patient{
						Link: []*r2pb.Patient_Link{{}},
					},
				},
			},
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			u := setupUnmarshaller(t, fhirversion.DSTU2)
			gotRes, gotOut, err := u.UnmarshalWithOutcome(test.json)
			if err != nil {
				t.Fatalf("unmarshal with outcome %v failed: %v", test.name, err)
			}
			if diff := cmp.Diff(gotOut, test.wantOutcome, protocmp.Transform()); diff != "" {
				t.Errorf("unmarshal with outcome %v: got outcome %v, want %v", test.name, gotOut, test.wantOutcome)
			}
			if !proto.Equal(gotRes, test.wantMsg) {
				t.Errorf("unmarshal with outcome %v: got resource %v, want %v", test.name, gotRes, test.wantMsg)
			}
		})
	}
}

func TestUnmarshalWithErrorReporter_R2_Errors(t *testing.T) {
	tests := []struct {
		name        string
		json        []byte
		wantOutcome *errorreporter.MultiVersionOperationOutcome
		wantMsg     proto.Message
	}{
		{
			name: "invalid reference type",
			json: []byte(`
    {
      "resourceType": "Patient",
      "managingOrganization": {
				"reference": "Patient/1"
			}
    }`),
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
			wantMsg: &r2pb.ContainedResource{
				OneofResource: &r2pb.ContainedResource_Patient{
					Patient: &r2pb.Patient{
						ManagingOrganization: &d2pb.Reference{
							Reference: &d2pb.Reference_PatientId{
								PatientId: &d2pb.ReferenceId{Value: "1"},
							},
						},
					},
				},
			},
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			u := setupUnmarshaller(t, fhirversion.DSTU2)
			oer := errorreporter.NewOperationErrorReporter(fhirversion.DSTU2)
			gotRes, err := u.UnmarshalWithErrorReporter(test.json, oer)
			if err != nil {
				t.Fatalf("unmarshal with outcome %v failed: %v", test.name, err)
			}
			if diff := cmp.Diff(oer.Outcome, test.wantOutcome, protocmp.Transform()); diff != "" {
				t.Errorf("unmarshal with outcome %v: got outcome %v, want %v", test.name, oer.Outcome, test.wantOutcome)
			}
			if !cmp.Equal(gotRes, test.wantMsg, protocmp.Transform()) {
				t.Errorf("unmarshal with outcome %v: got resource %v, want %v", test.name, gotRes, test.wantMsg)
			}
		})
	}
}

func TestUnmarshal_R2_Errors(t *testing.T) {
	tests := []struct {
		name    string
		json    string
		wantErr string
	}{
		{
			name: "error in nested object",
			json: `
		{
      "resourceType": "Patient",
      "animal": {
				"species": {
					"coding": 1
				}
      }
    }
    `,
			wantErr: `error at "Patient.animal.species.coding": expected array`,
		},
		{
			name: "leading upper case fhir_comments",
			json: `
			{
				"resourceType": "Patient",
				"careProvider": [
					{
						"reference": "Practitioner/1",
						"Fhir_comments": ["c1", "c2"]
					}
				]
			}`,
			wantErr: `error at "Patient.careProvider[0]": unknown field`,
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			u := setupUnmarshaller(t, fhirversion.DSTU2)
			_, err := u.Unmarshal([]byte(test.json))
			if err == nil {
				t.Fatalf("unmarshal %s failed: got error < nil >, want %q", test.name, test.wantErr)
			}
			if err.Error() != test.wantErr {
				t.Errorf("unmarshal %s: got error %q, want: %q", test.name, err.Error(), test.wantErr)
			}
		})
	}
}

func TestParsePrimitiveType_R2(t *testing.T) {
	tests := []struct {
		pType string
		value json.RawMessage
		want  proto.Message
	}{
		{
			pType: "Base64Binary",
			value: json.RawMessage(`"YmFzZTY0IGJ5dGVz"`),
			want: &d2pb.Base64Binary{
				Value: []byte("base64 bytes"),
			},
		},
		{
			pType: "Boolean",
			value: json.RawMessage(`true`),
			want: &d2pb.Boolean{
				Value: true,
			},
		},
		{
			pType: "Code",
			value: json.RawMessage(`"<some code>"`),
			want: &d2pb.Code{
				Value: "<some code>",
			},
		},
		{
			pType: "Id",
			value: json.RawMessage(`"abc123"`),
			want: &d2pb.Id{
				Value: "abc123",
			},
		},
		{
			pType: "Integer",
			value: json.RawMessage(`-1234`),
			want: &d2pb.Integer{
				Value: -1234,
			},
		},
		{
			pType: "Markdown",
			value: json.RawMessage(`"# md"`),
			want: &d2pb.Markdown{
				Value: "# md",
			},
		},
		{
			pType: "Oid",
			value: json.RawMessage(`"urn:oid:1.23"`),
			want: &d2pb.Oid{
				Value: "urn:oid:1.23",
			},
		},
		{
			pType: "Uuid",
			value: json.RawMessage(`"urn:uuid:f81d4fae-7dec-11d0-a765-00a0c91e6bf6"`),
			want: &d2pb.Uuid{
				Value: "urn:uuid:f81d4fae-7dec-11d0-a765-00a0c91e6bf6",
			},
		},
		{
			pType: "PositiveInt",
			value: json.RawMessage(`5678`),
			want: &d2pb.PositiveInt{
				Value: 5678,
			},
		},
		{
			pType: "String",
			value: json.RawMessage(`"a given string"`),
			want: &d2pb.String{
				Value: "a given string",
			},
		},
		{
			pType: "UnsignedInt",
			value: json.RawMessage(`90`),
			want: &d2pb.UnsignedInt{
				Value: 90,
			},
		},
		{
			pType: "Date",
			value: json.RawMessage(`"2017"`),
			want: &d2pb.Date{
				ValueUs:   1483257600000000,
				Precision: d2pb.Date_YEAR,
				Timezone:  "America/Los_Angeles",
			},
		},
		{
			pType: "DateTime",
			value: json.RawMessage(`"2018"`),
			want: &d2pb.DateTime{
				ValueUs:   1514793600000000,
				Precision: d2pb.DateTime_YEAR,
				Timezone:  "America/Los_Angeles",
			},
		},
		{
			pType: "DateTime2",
			value: json.RawMessage(`"2018-01-01"`),
			want: &d2pb.DateTime{
				ValueUs:   1514793600000000,
				Precision: d2pb.DateTime_DAY,
				Timezone:  "America/Los_Angeles",
			},
		},
		{
			pType: "Time",
			value: json.RawMessage(`"12:00:00"`),
			want: &d2pb.Time{
				ValueUs:   43200000000,
				Precision: d2pb.Time_SECOND,
			},
		},
		{
			pType: "Instant",
			value: json.RawMessage(`"2018-01-01T12:00:00.000Z"`),
			want: &d2pb.Instant{
				ValueUs:   1514808000000000,
				Precision: d2pb.Instant_MILLISECOND,
				Timezone:  "Z",
			},
		},
		{
			pType: "Decimal",
			value: json.RawMessage(`1.23`),
			want: &d2pb.Decimal{
				Value: "1.23",
			},
		},
		{
			pType: "Gender with extension",
			value: json.RawMessage(`{
        "extension": [
          {
            "url": "http://example#gender"
          }
        ]
      }`),
			want: &c2pb.AdministrativeGenderCode{
				Extension: []*d2pb.Extension{{
					Url: &d2pb.Uri{
						Value: "http://example#gender",
					},
				}, {
					Url: &d2pb.Uri{
						Value: "https://g.co/fhir/StructureDefinition/primitiveHasNoValue",
					},
					Value: &d2pb.Extension_ValueX{
						Choice: &d2pb.Extension_ValueX_Boolean{
							Boolean: &d2pb.Boolean{
								Value: true,
							},
						},
					},
				}},
			},
		},
	}
	for _, test := range tests {
		t.Run(test.pType, func(t *testing.T) {
			u := setupUnmarshaller(t, fhirversion.DSTU2)
			value := make([]byte, len(test.value))
			copy(value, test.value)
			got, err := u.parsePrimitiveType("value", test.want.ProtoReflect(), value)
			if err != nil {
				t.Fatalf("parse primitive type: %v", jsonpbhelper.PrintUnmarshalError(err, -1))
			}
			if !cmp.Equal(got, test.want, protocmp.Transform()) {
				t.Errorf("parse primitive type %v: got %v, want %v", test.pType, got, test.want)
			}
		})
	}
}

func TestParseURIs_R2(t *testing.T) {
	tests := []string{
		"ftp://ftp.is.co.za/rfc/rfc1808.txt",
		"http://www.ietf.org/rfc/rfc2396.txt",
		"ldap://[2001:db8::7]/c=GB?objectClass?one",
		"mailto:John.Doe@example.com",
		"news:comp.infosystems.www.servers.unix",
		"tel:+1-816-555-1212",
		"telnet://192.0.2.16:80/",
		"urn:oasis:names:specification:docbook:dtd:xml:4.1.2",
		"urn:uuid:a1b-2c3",
		"/resources/Patient/1",
		"Patient/1",
		"Patient/1#2",
	}
	for _, test := range tests {
		t.Run(test, func(t *testing.T) {
			u := setupUnmarshaller(t, fhirversion.DSTU2)
			r := proto.Clone(&d2pb.Uri{})
			rpb := r.ProtoReflect()
			rpb.Set(rpb.Descriptor().Fields().ByName("value"), protoreflect.ValueOfString(test))
			got, err := u.parsePrimitiveType("value", rpb, json.RawMessage(strconv.Quote(test)))
			if err != nil {
				t.Fatalf("parse Uri, got err %v, want <nil>", err)
			}
			if !cmp.Equal(got, r, protocmp.Transform()) {
				t.Errorf("parse Uri: got %v, want %v", got, test)
			}
		})
	}
}

func TestParsePrimitiveType_R2_Errors(t *testing.T) {
	tests := []struct {
		name  string
		value json.RawMessage
		msg   proto.Message
	}{
		{
			name:  "Code",
			value: json.RawMessage(`false`),
			msg:   &d2pb.Code{},
		},
		{
			name:  "Oid - type",
			value: json.RawMessage(`0`),
			msg:   &d2pb.Oid{},
		},
		{
			name:  "PositiveInt",
			value: json.RawMessage(`-123`),
			msg:   &d2pb.PositiveInt{},
		},
		{
			name:  "PositiveInt overflow",
			value: json.RawMessage(strconv.FormatUint(math.MaxUint64, 10)),
			msg:   &d2pb.PositiveInt{},
		},
		{
			name:  "UnsignedInt 00",
			value: json.RawMessage(`00`),
			msg:   &d2pb.UnsignedInt{},
		},
		{
			name:  "UnsignedInt -123",
			value: json.RawMessage(`-123`),
			msg:   &d2pb.UnsignedInt{},
		},
		{
			name:  "UnsignedInt 0123",
			value: json.RawMessage(`0123`),
			msg:   &d2pb.UnsignedInt{},
		},
		{
			name:  "UnsignedInt overflow",
			value: json.RawMessage(strconv.FormatUint(math.MaxUint64, 10)),
			msg:   &d2pb.UnsignedInt{},
		},
		{
			name:  "BinaryData",
			value: json.RawMessage(`"invalid"`),
			msg:   &d2pb.Base64Binary{},
		},
		{
			name:  "Boolean",
			value: json.RawMessage(`"invalid"`),
			msg:   &d2pb.Boolean{},
		},
		{
			name:  "Instant no timezone",
			value: json.RawMessage(`"2018-01-01T12:00:00.000"`),
			msg:   &d2pb.Instant{},
		},
		{
			name:  "Instant no seconds",
			value: json.RawMessage(`"2018-01-01T12:00Z"`),
			msg:   &d2pb.Instant{},
		},
		{
			name:  "Decimal - leading 0",
			value: json.RawMessage(`01.23`),
			msg:   &d2pb.Decimal{},
		},
		{
			name:  "Integer",
			value: json.RawMessage(`1.0`),
			msg:   &d2pb.Integer{},
		},
		{
			name:  "Markdown",
			value: json.RawMessage(`0`),
			msg:   &d2pb.Markdown{},
		},
		{
			name:  "Uri",
			value: json.RawMessage(`0`),
			msg:   &d2pb.Uri{},
		},
		{
			name:  "Uuid",
			value: json.RawMessage(`0`),
			msg:   &d2pb.Uuid{},
		},
		{
			name:  "Xhtml",
			value: json.RawMessage(`0`),
			msg:   &d2pb.Xhtml{},
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			u := setupUnmarshaller(t, fhirversion.DSTU2)
			_, err := u.parsePrimitiveType("value", test.msg.ProtoReflect(), test.value)
			if err == nil {
				t.Errorf("parsePrimitiveType() %v succeeded, expect error", test.name)
			}
		})
	}
}

func TestUnmarshalVersioned_R2(t *testing.T) {
	patient := `{"resourceType":"Patient"}`
	u2 := setupUnmarshaller(t, fhirversion.DSTU2)
	if _, err := u2.UnmarshalR2([]byte(patient)); err != nil {
		t.Errorf("UnmarshalR2(%s) returned unexpected error; %v", patient, err)
	}
	if _, err := u2.UnmarshalR3([]byte(patient)); err == nil {
		t.Errorf("UnmarshalR3(%s) didn't return expected error", patient)
	}
	if _, err := u2.UnmarshalR4([]byte(patient)); err == nil {
		t.Errorf("UnmarshalR4(%s) didn't return expected error", patient)
	}

	u3 := setupUnmarshaller(t, fhirversion.STU3)
	if _, err := u3.UnmarshalR2([]byte(patient)); err == nil {
		t.Errorf("UnmarshalR2(%s) didn't return expected error", patient)
	}

	u4 := setupUnmarshaller(t, fhirversion.R4)
	if _, err := u4.UnmarshalR2([]byte(patient)); err == nil {
		t.Errorf("UnmarshalR2(%s) didn't return expected error", patient)
	}
}

// LINT.ThenChange(unmarshaller_test.go)
