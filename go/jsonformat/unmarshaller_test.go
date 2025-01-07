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
	"bytes"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"math"
	"path"
	"regexp"

	"path/filepath"
	"strconv"
	"strings"
	"testing"

	"runtime"

	"github.com/bazelbuild/rules_go/go/tools/bazel"
	"github.com/google/fhir/go/fhirversion"
	"github.com/google/fhir/go/jsonformat/errorreporter"
	"github.com/google/fhir/go/jsonformat/internal/jsonpbhelper"
	"github.com/google/go-cmp/cmp"
	"github.com/google/go-cmp/cmp/cmpopts"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/reflect/protoreflect"
	"google.golang.org/protobuf/testing/protocmp"

	anypb "google.golang.org/protobuf/types/known/anypb"
	c4pb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/codes_go_proto"
	d4pb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/datatypes_go_proto"
	r4pb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/bundle_and_contained_resource_go_proto"
	r4devicepb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/device_go_proto"
	r4observationpb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/observation_go_proto"
	r4outcomepb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/operation_outcome_go_proto"
	r4patientpb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/patient_go_proto"
	r4searchparampb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/search_parameter_go_proto"
	c5pb "github.com/google/fhir/go/proto/google/fhir/proto/r5/core/codes_go_proto"
	d5pb "github.com/google/fhir/go/proto/google/fhir/proto/r5/core/datatypes_go_proto"
	r5pb "github.com/google/fhir/go/proto/google/fhir/proto/r5/core/resources/bundle_and_contained_resource_go_proto"
	r5devicepb "github.com/google/fhir/go/proto/google/fhir/proto/r5/core/resources/device_go_proto"
	r5observationpb "github.com/google/fhir/go/proto/google/fhir/proto/r5/core/resources/observation_go_proto"
	r5outcomepb "github.com/google/fhir/go/proto/google/fhir/proto/r5/core/resources/operation_outcome_go_proto"
	r5patientpb "github.com/google/fhir/go/proto/google/fhir/proto/r5/core/resources/patient_go_proto"
	r5searchparampb "github.com/google/fhir/go/proto/google/fhir/proto/r5/core/resources/search_parameter_go_proto"
	r5valuesetpb "github.com/google/fhir/go/proto/google/fhir/proto/r5/core/valuesets_go_proto"
	c3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/codes_go_proto"
	d3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/datatypes_go_proto"
	m3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/metadatatypes_go_proto"
	r3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/resources_go_proto"
)

var (
	unnestErrorListOpt = cmpopts.AcyclicTransformer("UnnestedErrorList", func(v interface{}) interface{} {
		if el, ok := v.(jsonpbhelper.UnmarshalErrorList); ok && len(el) == 1 {
			return el[0]
		}
		return v
	})
	basePath = "testdata/jsonformat"
	allVers  = []fhirversion.Version{fhirversion.STU3, fhirversion.R4, fhirversion.R5}
)

// TODO(b/135148603): Find a better way to maintain the versioned unit tests.
// TODO(b/185413163): Legacy tests in this file use hardcoded data -
// these should be moved to testdata files to make it easy to share testcases between languages.
// New test cases should create new testdata files in the basePath above.

func setupUnmarshaller(t *testing.T, ver fhirversion.Version) *Unmarshaller {
	t.Helper()
	u, err := NewUnmarshaller("America/Los_Angeles", ver)
	if err != nil {
		t.Fatalf("failed to create unmarshaler; %v", err)
	}
	return u
}

var (
	_, b, _, _ = runtime.Caller(0)
	callerRoot = filepath.Dir(b)
)
// getRootPath returns the root bazel runfiles path if running in a bazel
// environment, otherwise it will return the root path of the FHIR proto
// repository. Typically this is used to access files such as testdata.
// As usual, the caller should check to see if the expected file(s)
// exist, and if not, report an error.
func getRootPath() string {
	var root string
 	root, err := bazel.RunfilesPath()
 	if err != nil {
		// Fall back to the non-bazel way to get to the root directory.
		root = callerRoot + "/../../"
	}
	return root
}

func readFile(basePath, fileName string) ([]byte, error) {
	filePath := path.Join(getRootPath(), basePath, fileName)
	return ioutil.ReadFile(filePath)
}

func TestUnmarshal(t *testing.T) {
	tests := []struct {
		name  string
		json  []byte
		wants []mvr
	}{
		{
			name: "SearchParameter",
			json: []byte(`
    {
      "resourceType": "SearchParameter",
			"url": "http://example.com/SearchParameter",
			"name": "Search-parameter",
			"status": "active",
			"description": "custom search parameter",
      "code": "value-quantity",
			"base": ["Observation"],
			"type": "number"
    }`),
			wants: []mvr{
				{
					ver: fhirversion.STU3,
					r: &r3pb.ContainedResource{
						OneofResource: &r3pb.ContainedResource_SearchParameter{
							SearchParameter: &r3pb.SearchParameter{
								Url:         &d3pb.Uri{Value: "http://example.com/SearchParameter"},
								Name:        &d3pb.String{Value: "Search-parameter"},
								Status:      &c3pb.PublicationStatusCode{Value: c3pb.PublicationStatusCode_ACTIVE},
								Description: &d3pb.Markdown{Value: "custom search parameter"},
								Code:        &d3pb.Code{Value: "value-quantity"},
								Base:        []*c3pb.ResourceTypeCode{{Value: c3pb.ResourceTypeCode_OBSERVATION}},
								Type:        &c3pb.SearchParamTypeCode{Value: c3pb.SearchParamTypeCode_NUMBER},
							},
						},
					},
				},
				{
					ver: fhirversion.R4,
					r: &r4pb.ContainedResource{
						OneofResource: &r4pb.ContainedResource_SearchParameter{
							SearchParameter: &r4searchparampb.SearchParameter{
								Url:         &d4pb.Uri{Value: "http://example.com/SearchParameter"},
								Name:        &d4pb.String{Value: "Search-parameter"},
								Status:      &r4searchparampb.SearchParameter_StatusCode{Value: c4pb.PublicationStatusCode_ACTIVE},
								Description: &d4pb.Markdown{Value: "custom search parameter"},
								Code:        &d4pb.Code{Value: "value-quantity"},
								Base:        []*r4searchparampb.SearchParameter_BaseCode{{Value: c4pb.ResourceTypeCode_OBSERVATION}},
								Type:        &r4searchparampb.SearchParameter_TypeCode{Value: c4pb.SearchParamTypeCode_NUMBER},
							},
						},
					},
				},
				{
					ver: fhirversion.R5,
					r: &r5pb.ContainedResource{
						OneofResource: &r5pb.ContainedResource_SearchParameter{
							SearchParameter: &r5searchparampb.SearchParameter{
								Url:         &d5pb.Uri{Value: "http://example.com/SearchParameter"},
								Name:        &d5pb.String{Value: "Search-parameter"},
								Status:      &r5searchparampb.SearchParameter_StatusCode{Value: c5pb.PublicationStatusCode_ACTIVE},
								Description: &d5pb.Markdown{Value: "custom search parameter"},
								Code:        &d5pb.Code{Value: "value-quantity"},
								Base:        []*r5searchparampb.SearchParameter_BaseCode{{Value: r5valuesetpb.VersionIndependentResourceTypesAllValueSet_OBSERVATION}},
								Type:        &r5searchparampb.SearchParameter_TypeCode{Value: c5pb.SearchParamTypeCode_NUMBER},
							},
						},
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
			wants: []mvr{
				{
					ver: fhirversion.STU3,
					r: &r3pb.ContainedResource{
						OneofResource: &r3pb.ContainedResource_Observation{
							Observation: &r3pb.Observation{
								Id: &d3pb.Id{
									Value: "example",
								},
								Status: &c3pb.ObservationStatusCode{Value: c3pb.ObservationStatusCode_FINAL},
								Code:   &d3pb.CodeableConcept{Text: &d3pb.String{Value: "test"}},
								Text: &m3pb.Narrative{
									Status: &c3pb.NarrativeStatusCode{
										Value: c3pb.NarrativeStatusCode_GENERATED,
									},
									Div: &d3pb.Xhtml{
										Value: `<div xmlns="http://www.w3.org/1999/xhtml">[Put rendering here]</div>`,
									},
								},
							},
						},
					},
				},
				{
					ver: fhirversion.R4,
					r: &r4pb.ContainedResource{
						OneofResource: &r4pb.ContainedResource_Observation{
							Observation: &r4observationpb.Observation{
								Id: &d4pb.Id{
									Value: "example",
								},
								Status: &r4observationpb.Observation_StatusCode{Value: c4pb.ObservationStatusCode_FINAL},
								Code:   &d4pb.CodeableConcept{Text: &d4pb.String{Value: "test"}},
								Text: &d4pb.Narrative{
									Status: &d4pb.Narrative_StatusCode{
										Value: c4pb.NarrativeStatusCode_GENERATED,
									},
									Div: &d4pb.Xhtml{
										Value: `<div xmlns="http://www.w3.org/1999/xhtml">[Put rendering here]</div>`,
									},
								},
							},
						},
					},
				},
				{
					ver: fhirversion.R5,
					r: &r5pb.ContainedResource{
						OneofResource: &r5pb.ContainedResource_Observation{
							Observation: &r5observationpb.Observation{
								Id: &d5pb.Id{
									Value: "example",
								},
								Status: &r5observationpb.Observation_StatusCode{Value: c5pb.ObservationStatusCode_FINAL},
								Code:   &d5pb.CodeableConcept{Text: &d5pb.String{Value: "test"}},
								Text: &d5pb.Narrative{
									Status: &d5pb.Narrative_StatusCode{
										Value: c5pb.NarrativeStatusCode_GENERATED,
									},
									Div: &d5pb.Xhtml{
										Value: `<div xmlns="http://www.w3.org/1999/xhtml">[Put rendering here]</div>`,
									},
								},
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
			wants: []mvr{
				{
					ver: fhirversion.STU3,
					r: &r3pb.ContainedResource{
						OneofResource: &r3pb.ContainedResource_Patient{
							Patient: &r3pb.Patient{
								MultipleBirth: &r3pb.Patient_MultipleBirth{
									MultipleBirth: &r3pb.Patient_MultipleBirth_Boolean{
										Boolean: &d3pb.Boolean{Value: false}}},
							},
						},
					},
				},
				{
					ver: fhirversion.R4,
					r: &r4pb.ContainedResource{
						OneofResource: &r4pb.ContainedResource_Patient{
							Patient: &r4patientpb.Patient{
								MultipleBirth: &r4patientpb.Patient_MultipleBirthX{
									Choice: &r4patientpb.Patient_MultipleBirthX_Boolean{
										Boolean: &d4pb.Boolean{Value: false}}},
							},
						},
					},
				},
				{
					ver: fhirversion.R5,
					r: &r5pb.ContainedResource{
						OneofResource: &r5pb.ContainedResource_Patient{
							Patient: &r5patientpb.Patient{
								MultipleBirth: &r5patientpb.Patient_MultipleBirthX{
									Choice: &r5patientpb.Patient_MultipleBirthX_Boolean{
										Boolean: &d5pb.Boolean{Value: false}}},
							},
						},
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
					"fullUrl": "Patient/1",
          "resource": {
            "resourceType": "Patient"
          }
        },
        {
					"fullUrl": "Patient/2",
          "resource": {
            "resourceType": "Patient"
          }
        }
      ]
    }`),
			wants: []mvr{
				{
					ver: fhirversion.STU3,
					r: &r3pb.ContainedResource{
						OneofResource: &r3pb.ContainedResource_Bundle{
							Bundle: &r3pb.Bundle{
								Type: &c3pb.BundleTypeCode{Value: c3pb.BundleTypeCode_COLLECTION},
								Entry: []*r3pb.Bundle_Entry{{
									FullUrl: &d3pb.Uri{Value: "Patient/1"},
									Resource: &r3pb.ContainedResource{
										OneofResource: &r3pb.ContainedResource_Patient{
											Patient: &r3pb.Patient{},
										},
									}},
									{
										FullUrl: &d3pb.Uri{Value: "Patient/2"},
										Resource: &r3pb.ContainedResource{
											OneofResource: &r3pb.ContainedResource_Patient{
												Patient: &r3pb.Patient{},
											},
										}},
								},
							},
						},
					},
				},
				{
					ver: fhirversion.R4,
					r: &r4pb.ContainedResource{
						OneofResource: &r4pb.ContainedResource_Bundle{
							Bundle: &r4pb.Bundle{
								Type: &r4pb.Bundle_TypeCode{Value: c4pb.BundleTypeCode_COLLECTION},
								Entry: []*r4pb.Bundle_Entry{{
									FullUrl: &d4pb.Uri{Value: "Patient/1"},
									Resource: &r4pb.ContainedResource{
										OneofResource: &r4pb.ContainedResource_Patient{
											Patient: &r4patientpb.Patient{},
										},
									}},
									{
										FullUrl: &d4pb.Uri{Value: "Patient/2"},
										Resource: &r4pb.ContainedResource{
											OneofResource: &r4pb.ContainedResource_Patient{
												Patient: &r4patientpb.Patient{},
											},
										}},
								},
							},
						},
					},
				},
				{
					ver: fhirversion.R5,
					r: &r5pb.ContainedResource{
						OneofResource: &r5pb.ContainedResource_Bundle{
							Bundle: &r5pb.Bundle{
								Type: &r5pb.Bundle_TypeCode{Value: c5pb.BundleTypeCode_COLLECTION},
								Entry: []*r5pb.Bundle_Entry{{
									FullUrl: &d5pb.Uri{Value: "Patient/1"},
									Resource: &r5pb.ContainedResource{
										OneofResource: &r5pb.ContainedResource_Patient{
											Patient: &r5patientpb.Patient{},
										},
									}},
									{
										FullUrl: &d5pb.Uri{Value: "Patient/2"},
										Resource: &r5pb.ContainedResource{
											OneofResource: &r5pb.ContainedResource_Patient{
												Patient: &r5patientpb.Patient{},
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
			wants: []mvr{
				{
					ver: fhirversion.STU3,
					r: &r3pb.ContainedResource{
						OneofResource: &r3pb.ContainedResource_Patient{
							Patient: &r3pb.Patient{
								Name: []*d3pb.HumanName{{
									Given: []*d3pb.String{{
										Value: "Toby",
										Id: &d3pb.String{
											Value: "a3",
										},
										Extension: []*d3pb.Extension{{
											Url: &d3pb.Uri{
												Value: "http://hl7.org/fhir/StructureDefinition/qualifier",
											},
											Value: &d3pb.Extension_ValueX{
												Choice: &d3pb.Extension_ValueX_Code{
													Code: &d3pb.Code{
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
					ver: fhirversion.R4,
					r: &r4pb.ContainedResource{
						OneofResource: &r4pb.ContainedResource_Patient{
							Patient: &r4patientpb.Patient{
								Name: []*d4pb.HumanName{{
									Given: []*d4pb.String{{
										Value: "Toby",
										Id: &d4pb.String{
											Value: "a3",
										},
										Extension: []*d4pb.Extension{{
											Url: &d4pb.Uri{
												Value: "http://hl7.org/fhir/StructureDefinition/qualifier",
											},
											Value: &d4pb.Extension_ValueX{
												Choice: &d4pb.Extension_ValueX_Code{
													Code: &d4pb.Code{
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
					ver: fhirversion.R5,
					r: &r5pb.ContainedResource{
						OneofResource: &r5pb.ContainedResource_Patient{
							Patient: &r5patientpb.Patient{
								Name: []*d5pb.HumanName{{
									Given: []*d5pb.String{{
										Value: "Toby",
										Id: &d5pb.String{
											Value: "a3",
										},
										Extension: []*d5pb.Extension{{
											Url: &d5pb.Uri{
												Value: "http://hl7.org/fhir/StructureDefinition/qualifier",
											},
											Value: &d5pb.Extension_ValueX{
												Choice: &d5pb.Extension_ValueX_Code{
													Code: &d5pb.Code{
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
			wants: []mvr{
				{
					ver: fhirversion.STU3,
					r: &r3pb.ContainedResource{
						OneofResource: &r3pb.ContainedResource_Patient{
							Patient: &r3pb.Patient{
								Name: []*d3pb.HumanName{{
									Given: []*d3pb.String{{
										Id: &d3pb.String{Value: "a3"},
										Extension: []*d3pb.Extension{
											{
												Url: &d3pb.Uri{
													Value: "http://hl7.org/fhir/StructureDefinition/ext1",
												},
												Value: &d3pb.Extension_ValueX{
													Choice: &d3pb.Extension_ValueX_Code{
														Code: &d3pb.Code{Value: "value1"},
													},
												},
											},
											{
												Url: &d3pb.Uri{
													Value: "http://hl7.org/fhir/StructureDefinition/ext2",
												},
												Value: &d3pb.Extension_ValueX{
													Choice: &d3pb.Extension_ValueX_Code{
														Code: &d3pb.Code{Value: "value2"},
													},
												},
											},
											{
												Url: &d3pb.Uri{
													Value: "https://g.co/fhir/StructureDefinition/primitiveHasNoValue",
												},
												Value: &d3pb.Extension_ValueX{
													Choice: &d3pb.Extension_ValueX_Boolean{
														Boolean: &d3pb.Boolean{
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
					ver: fhirversion.R4,
					r: &r4pb.ContainedResource{
						OneofResource: &r4pb.ContainedResource_Patient{
							Patient: &r4patientpb.Patient{
								Name: []*d4pb.HumanName{{
									Given: []*d4pb.String{{
										Id: &d4pb.String{Value: "a3"},
										Extension: []*d4pb.Extension{
											{
												Url: &d4pb.Uri{
													Value: "http://hl7.org/fhir/StructureDefinition/ext1",
												},
												Value: &d4pb.Extension_ValueX{
													Choice: &d4pb.Extension_ValueX_Code{
														Code: &d4pb.Code{Value: "value1"},
													},
												},
											},
											{
												Url: &d4pb.Uri{
													Value: "http://hl7.org/fhir/StructureDefinition/ext2",
												},
												Value: &d4pb.Extension_ValueX{
													Choice: &d4pb.Extension_ValueX_Code{
														Code: &d4pb.Code{Value: "value2"},
													},
												},
											},
											{
												Url: &d4pb.Uri{
													Value: "https://g.co/fhir/StructureDefinition/primitiveHasNoValue",
												},
												Value: &d4pb.Extension_ValueX{
													Choice: &d4pb.Extension_ValueX_Boolean{
														Boolean: &d4pb.Boolean{
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
					ver: fhirversion.R5,
					r: &r5pb.ContainedResource{
						OneofResource: &r5pb.ContainedResource_Patient{
							Patient: &r5patientpb.Patient{
								Name: []*d5pb.HumanName{{
									Given: []*d5pb.String{{
										Id: &d5pb.String{Value: "a3"},
										Extension: []*d5pb.Extension{
											{
												Url: &d5pb.Uri{
													Value: "http://hl7.org/fhir/StructureDefinition/ext1",
												},
												Value: &d5pb.Extension_ValueX{
													Choice: &d5pb.Extension_ValueX_Code{
														Code: &d5pb.Code{Value: "value1"},
													},
												},
											},
											{
												Url: &d5pb.Uri{
													Value: "http://hl7.org/fhir/StructureDefinition/ext2",
												},
												Value: &d5pb.Extension_ValueX{
													Choice: &d5pb.Extension_ValueX_Code{
														Code: &d5pb.Code{Value: "value2"},
													},
												},
											},
											{
												Url: &d5pb.Uri{
													Value: "https://g.co/fhir/StructureDefinition/primitiveHasNoValue",
												},
												Value: &d5pb.Extension_ValueX{
													Choice: &d5pb.Extension_ValueX_Boolean{
														Boolean: &d5pb.Boolean{
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
			wants: []mvr{
				{
					ver: fhirversion.STU3,
					r: &r3pb.ContainedResource{
						OneofResource: &r3pb.ContainedResource_Patient{
							Patient: &r3pb.Patient{
								Gender: &c3pb.AdministrativeGenderCode{
									Value: c3pb.AdministrativeGenderCode_OTHER,
									Extension: []*d3pb.Extension{{
										Url: &d3pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition/patient-genderIdentity"},
										Value: &d3pb.Extension_ValueX{
											Choice: &d3pb.Extension_ValueX_Code{
												Code: &d3pb.Code{
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
					ver: fhirversion.R4,
					r: &r4pb.ContainedResource{
						OneofResource: &r4pb.ContainedResource_Patient{
							Patient: &r4patientpb.Patient{
								Gender: &r4patientpb.Patient_GenderCode{
									Value: c4pb.AdministrativeGenderCode_OTHER,
									Extension: []*d4pb.Extension{{
										Url: &d4pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition/patient-genderIdentity"},
										Value: &d4pb.Extension_ValueX{
											Choice: &d4pb.Extension_ValueX_Code{
												Code: &d4pb.Code{
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
					ver: fhirversion.R5,
					r: &r5pb.ContainedResource{
						OneofResource: &r5pb.ContainedResource_Patient{
							Patient: &r5patientpb.Patient{
								Gender: &r5patientpb.Patient_GenderCode{
									Value: c5pb.AdministrativeGenderCode_OTHER,
									Extension: []*d5pb.Extension{{
										Url: &d5pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition/patient-genderIdentity"},
										Value: &d5pb.Extension_ValueX{
											Choice: &d5pb.Extension_ValueX_Code{
												Code: &d5pb.Code{
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
			},
		},
		{
			name: "STU3 DataElement",
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
			wants: []mvr{
				{
					ver: fhirversion.STU3,
					r: &r3pb.ContainedResource{
						OneofResource: &r3pb.ContainedResource_DataElement{
							DataElement: &r3pb.DataElement{
								Status: &c3pb.PublicationStatusCode{Value: c3pb.PublicationStatusCode_DRAFT},
								Element: []*m3pb.ElementDefinition{
									{
										Path: &d3pb.String{Value: "path"},
										MaxValue: &m3pb.ElementDefinition_MaxValue{
											MaxValue: &m3pb.ElementDefinition_MaxValue_UnsignedInt{UnsignedInt: &d3pb.UnsignedInt{Value: 92}},
										},
										Max: &d3pb.String{Value: "10"},
									},
								},
							},
						},
					},
				},
			},
		},
		{
			name: "STU3 acronym field",
			json: []byte(`{"id":"example","resourceType":"Device","udi":{"carrierHRF":"test"}}`),
			wants: []mvr{
				{
					ver: fhirversion.STU3,
					r: &r3pb.ContainedResource{
						OneofResource: &r3pb.ContainedResource_Device{
							Device: &r3pb.Device{
								Id: &d3pb.Id{
									Value: "example",
								},
								Udi: &r3pb.Device_Udi{
									CarrierHrf: &d3pb.String{Value: "test"},
								},
							},
						},
					},
				},
			},
		},
		{
			name: "R4 acronym field",
			json: []byte(`{"id":"example","resourceType":"Device","udiCarrier":[{"carrierHRF":"test"}]}`),
			wants: []mvr{
				{
					ver: fhirversion.R4,
					r: &r4pb.ContainedResource{
						OneofResource: &r4pb.ContainedResource_Device{
							Device: &r4devicepb.Device{
								Id: &d4pb.Id{
									Value: "example",
								},
								UdiCarrier: []*r4devicepb.Device_UdiCarrier{
									{CarrierHrf: &d4pb.String{Value: "test"}},
								},
							},
						},
					},
				},
			},
		},
		{
			name: "R5 acronym field",
			json: []byte(`{"id":"example","resourceType":"Device","udiCarrier":[{"deviceIdentifier":"1","issuer":"1","carrierHRF":"test"}]}`),
			wants: []mvr{
				{
					ver: fhirversion.R5,
					r: &r5pb.ContainedResource{
						OneofResource: &r5pb.ContainedResource_Device{
							Device: &r5devicepb.Device{
								Id: &d5pb.Id{
									Value: "example",
								},
								UdiCarrier: []*r5devicepb.Device_UdiCarrier{
									{
										DeviceIdentifier: &d5pb.String{Value: "1"},
										Issuer:           &d5pb.Uri{Value: "1"},
										CarrierHrf:       &d5pb.String{Value: "test"},
									},
								},
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
			wants: []mvr{
				{
					ver: fhirversion.R4,
					r: &r4pb.ContainedResource{
						OneofResource: &r4pb.ContainedResource_Patient{
							Patient: &r4patientpb.Patient{
								Id: &d4pb.Id{Value: "example"},
								Contained: []*anypb.Any{
									marshalToAny(t, &r4pb.ContainedResource{
										OneofResource: &r4pb.ContainedResource_Patient{
											Patient: &r4patientpb.Patient{
												Id: &d4pb.Id{Value: "nested"},
												Contained: []*anypb.Any{
													marshalToAny(t, &r4pb.ContainedResource{
														OneofResource: &r4pb.ContainedResource_Patient{
															Patient: &r4patientpb.Patient{
																Id: &d4pb.Id{Value: "double-nested"},
															},
														},
													}),
												},
											},
										},
									}),
								},
							},
						},
					},
				},
				{
					ver: fhirversion.STU3,
					r: &r3pb.ContainedResource{
						OneofResource: &r3pb.ContainedResource_Patient{
							Patient: &r3pb.Patient{
								Id: &d3pb.Id{Value: "example"},
								Contained: []*r3pb.ContainedResource{
									{
										OneofResource: &r3pb.ContainedResource_Patient{
											Patient: &r3pb.Patient{
												Id: &d3pb.Id{Value: "nested"},
												Contained: []*r3pb.ContainedResource{
													{
														OneofResource: &r3pb.ContainedResource_Patient{
															Patient: &r3pb.Patient{
																Id: &d3pb.Id{Value: "double-nested"},
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
					ver: fhirversion.R5,
					r: &r5pb.ContainedResource{
						OneofResource: &r5pb.ContainedResource_Patient{
							Patient: &r5patientpb.Patient{
								Id: &d5pb.Id{Value: "example"},
								Contained: []*anypb.Any{
									marshalToAny(t, &r5pb.ContainedResource{
										OneofResource: &r5pb.ContainedResource_Patient{
											Patient: &r5patientpb.Patient{
												Id: &d5pb.Id{Value: "nested"},
												Contained: []*anypb.Any{
													marshalToAny(t, &r5pb.ContainedResource{
														OneofResource: &r5pb.ContainedResource_Patient{
															Patient: &r5patientpb.Patient{
																Id: &d5pb.Id{Value: "double-nested"},
															},
														},
													}),
												},
											},
										},
									}),
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
			wants: []mvr{
				{
					ver: fhirversion.STU3,
					r: &r3pb.ContainedResource{
						OneofResource: &r3pb.ContainedResource_Patient{
							Patient: &r3pb.Patient{
								ManagingOrganization: &d3pb.Reference{
									Identifier: &d3pb.Identifier{
										Value: &d3pb.String{
											Value: "myorg",
										},
									},
								},
							},
						},
					},
				},
				{
					ver: fhirversion.R4,
					r: &r4pb.ContainedResource{
						OneofResource: &r4pb.ContainedResource_Patient{
							Patient: &r4patientpb.Patient{
								ManagingOrganization: &d4pb.Reference{
									Identifier: &d4pb.Identifier{
										Value: &d4pb.String{
											Value: "myorg",
										},
									},
								},
							},
						},
					},
				},
				{
					ver: fhirversion.R5,
					r: &r5pb.ContainedResource{
						OneofResource: &r5pb.ContainedResource_Patient{
							Patient: &r5patientpb.Patient{
								ManagingOrganization: &d5pb.Reference{
									Identifier: &d5pb.Identifier{
										Value: &d5pb.String{
											Value: "myorg",
										},
									},
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
			wants: []mvr{
				{
					ver: fhirversion.STU3,
					r: &r3pb.ContainedResource{
						OneofResource: &r3pb.ContainedResource_Patient{
							Patient: &r3pb.Patient{
								Id: &d3pb.Id{
									Value: "example",
								},
								Gender: &c3pb.AdministrativeGenderCode{
									Value: c3pb.AdministrativeGenderCode_FEMALE,
								},
								Extension: []*d3pb.Extension{{
									Url: &d3pb.Uri{Value: "http://nema.org/fhir/extensions#0010:1020"},
									Value: &d3pb.Extension_ValueX{
										Choice: &d3pb.Extension_ValueX_Quantity{
											Quantity: &d3pb.Quantity{Value: &d3pb.Decimal{Value: "1.83"}, Unit: &d3pb.String{Value: "m"}},
										},
									},
								}},
								Identifier: []*d3pb.Identifier{{
									System: &d3pb.Uri{
										Value: "http://nema.org/examples/patients",
									},
									Value: &d3pb.String{Value: "1234"},
								}},
								BirthDate: &d3pb.Date{
									Id:        &d3pb.String{Value: "12345"},
									ValueUs:   1463554800000000,
									Timezone:  "America/Los_Angeles",
									Precision: d3pb.Date_DAY,
									Extension: []*d3pb.Extension{{
										Url: &d3pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition/patient-birthTime"},
										Value: &d3pb.Extension_ValueX{
											Choice: &d3pb.Extension_ValueX_DateTime{
												DateTime: &d3pb.DateTime{
													ValueUs:   1463567325000000,
													Timezone:  "Z",
													Precision: d3pb.DateTime_SECOND,
													Extension: []*d3pb.Extension{{
														Url: &d3pb.Uri{Value: "http://example.com/fhir/extension"},
														Value: &d3pb.Extension_ValueX{
															Choice: &d3pb.Extension_ValueX_DateTime{
																DateTime: &d3pb.DateTime{
																	ValueUs:   1463567325000000,
																	Timezone:  "Z",
																	Precision: d3pb.DateTime_SECOND,
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
					ver: fhirversion.R4,
					r: &r4pb.ContainedResource{
						OneofResource: &r4pb.ContainedResource_Patient{
							Patient: &r4patientpb.Patient{
								Id: &d4pb.Id{
									Value: "example",
								},
								Gender: &r4patientpb.Patient_GenderCode{
									Value: c4pb.AdministrativeGenderCode_FEMALE,
								},
								Extension: []*d4pb.Extension{{
									Url: &d4pb.Uri{Value: "http://nema.org/fhir/extensions#0010:1020"},
									Value: &d4pb.Extension_ValueX{
										Choice: &d4pb.Extension_ValueX_Quantity{
											Quantity: &d4pb.Quantity{Value: &d4pb.Decimal{Value: "1.83"}, Unit: &d4pb.String{Value: "m"}},
										},
									},
								}},
								Identifier: []*d4pb.Identifier{{
									System: &d4pb.Uri{
										Value: "http://nema.org/examples/patients",
									},
									Value: &d4pb.String{Value: "1234"},
								}},
								BirthDate: &d4pb.Date{
									Id:        &d4pb.String{Value: "12345"},
									ValueUs:   1463554800000000,
									Timezone:  "America/Los_Angeles",
									Precision: d4pb.Date_DAY,
									Extension: []*d4pb.Extension{{
										Url: &d4pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition/patient-birthTime"},
										Value: &d4pb.Extension_ValueX{
											Choice: &d4pb.Extension_ValueX_DateTime{
												DateTime: &d4pb.DateTime{
													ValueUs:   1463567325000000,
													Timezone:  "Z",
													Precision: d4pb.DateTime_SECOND,
													Extension: []*d4pb.Extension{{
														Url: &d4pb.Uri{Value: "http://example.com/fhir/extension"},
														Value: &d4pb.Extension_ValueX{
															Choice: &d4pb.Extension_ValueX_DateTime{
																DateTime: &d4pb.DateTime{
																	ValueUs:   1463567325000000,
																	Timezone:  "Z",
																	Precision: d4pb.DateTime_SECOND,
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
					ver: fhirversion.R5,
					r: &r5pb.ContainedResource{
						OneofResource: &r5pb.ContainedResource_Patient{
							Patient: &r5patientpb.Patient{
								Id: &d5pb.Id{
									Value: "example",
								},
								Gender: &r5patientpb.Patient_GenderCode{
									Value: c5pb.AdministrativeGenderCode_FEMALE,
								},
								Extension: []*d5pb.Extension{{
									Url: &d5pb.Uri{Value: "http://nema.org/fhir/extensions#0010:1020"},
									Value: &d5pb.Extension_ValueX{
										Choice: &d5pb.Extension_ValueX_Quantity{
											Quantity: &d5pb.Quantity{Value: &d5pb.Decimal{Value: "1.83"}, Unit: &d5pb.String{Value: "m"}},
										},
									},
								}},
								Identifier: []*d5pb.Identifier{{
									System: &d5pb.Uri{
										Value: "http://nema.org/examples/patients",
									},
									Value: &d5pb.String{Value: "1234"},
								}},
								BirthDate: &d5pb.Date{
									Id:        &d5pb.String{Value: "12345"},
									ValueUs:   1463554800000000,
									Timezone:  "America/Los_Angeles",
									Precision: d5pb.Date_DAY,
									Extension: []*d5pb.Extension{{
										Url: &d5pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition/patient-birthTime"},
										Value: &d5pb.Extension_ValueX{
											Choice: &d5pb.Extension_ValueX_DateTime{
												DateTime: &d5pb.DateTime{
													ValueUs:   1463567325000000,
													Timezone:  "Z",
													Precision: d5pb.DateTime_SECOND,
													Extension: []*d5pb.Extension{{
														Url: &d5pb.Uri{Value: "http://example.com/fhir/extension"},
														Value: &d5pb.Extension_ValueX{
															Choice: &d5pb.Extension_ValueX_DateTime{
																DateTime: &d5pb.DateTime{
																	ValueUs:   1463567325000000,
																	Timezone:  "Z",
																	Precision: d5pb.DateTime_SECOND,
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
			},
		},
		{
			name: "trailing whitespace",
			json: []byte("{\"resourceType\": \"Patient\"} \t\n"),
			wants: []mvr{
				{
					ver: fhirversion.STU3,
					r: &r3pb.ContainedResource{
						OneofResource: &r3pb.ContainedResource_Patient{
							Patient: &r3pb.Patient{},
						},
					},
				},
				{
					ver: fhirversion.R4,
					r: &r4pb.ContainedResource{
						OneofResource: &r4pb.ContainedResource_Patient{
							Patient: &r4patientpb.Patient{},
						},
					},
				},
				{
					ver: fhirversion.R5,
					r: &r5pb.ContainedResource{
						OneofResource: &r5pb.ContainedResource_Patient{
							Patient: &r5patientpb.Patient{},
						},
					},
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
			wants: []mvr{
				{
					ver: fhirversion.STU3,
					r: &r3pb.ContainedResource{
						OneofResource: &r3pb.ContainedResource_Observation{
							Observation: &r3pb.Observation{
								Status: &c3pb.ObservationStatusCode{
									Extension: []*d3pb.Extension{{
										Url: &d3pb.Uri{Value: jsonpbhelper.PrimitiveHasNoValueURL},
										Value: &d3pb.Extension_ValueX{
											Choice: &d3pb.Extension_ValueX_Boolean{
												Boolean: &d3pb.Boolean{Value: true},
											},
										},
									}, {
										Url: &d3pb.Uri{Value: "http://hl7.org/fhir/ValueSet/data-absent-reason"},
										Value: &d3pb.Extension_ValueX{
											Choice: &d3pb.Extension_ValueX_Code{
												Code: &d3pb.Code{Value: "unsupported"},
											},
										},
									}},
								},
								Code: &d3pb.CodeableConcept{
									Coding: []*d3pb.Coding{},
								},
							},
						},
					},
				},
				{
					ver: fhirversion.R4,
					r: &r4pb.ContainedResource{
						OneofResource: &r4pb.ContainedResource_Observation{
							Observation: &r4observationpb.Observation{
								Status: &r4observationpb.Observation_StatusCode{
									Extension: []*d4pb.Extension{{
										Url: &d4pb.Uri{Value: jsonpbhelper.PrimitiveHasNoValueURL},
										Value: &d4pb.Extension_ValueX{
											Choice: &d4pb.Extension_ValueX_Boolean{
												Boolean: &d4pb.Boolean{Value: true},
											},
										},
									}, {
										Url: &d4pb.Uri{Value: "http://hl7.org/fhir/ValueSet/data-absent-reason"},
										Value: &d4pb.Extension_ValueX{
											Choice: &d4pb.Extension_ValueX_Code{
												Code: &d4pb.Code{Value: "unsupported"},
											},
										},
									}},
								},
								Code: &d4pb.CodeableConcept{
									Coding: []*d4pb.Coding{},
								},
							},
						},
					},
				},
				{
					ver: fhirversion.R5,
					r: &r5pb.ContainedResource{
						OneofResource: &r5pb.ContainedResource_Observation{
							Observation: &r5observationpb.Observation{
								Status: &r5observationpb.Observation_StatusCode{
									Extension: []*d5pb.Extension{{
										Url: &d5pb.Uri{Value: jsonpbhelper.PrimitiveHasNoValueURL},
										Value: &d5pb.Extension_ValueX{
											Choice: &d5pb.Extension_ValueX_Boolean{
												Boolean: &d5pb.Boolean{Value: true},
											},
										},
									}, {
										Url: &d5pb.Uri{Value: "http://hl7.org/fhir/ValueSet/data-absent-reason"},
										Value: &d5pb.Extension_ValueX{
											Choice: &d5pb.Extension_ValueX_Code{
												Code: &d5pb.Code{Value: "unsupported"},
											},
										},
									}},
								},
								Code: &d5pb.CodeableConcept{
									Coding: []*d5pb.Coding{},
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
			for _, w := range test.wants {
				t.Run(w.ver.String(), func(t *testing.T) {
					// test Unmarshal
					u := setupUnmarshaller(t, w.ver)
					got, err := u.Unmarshal(test.json)
					if err != nil {
						t.Fatalf("unmarshal %v failed: %v", test.name, err)
					}
					sortExtensions := cmp.Options{
						protocmp.SortRepeated(func(e1, e2 *d3pb.Extension) bool {
							return e1.GetUrl().GetValue() < e2.GetUrl().GetValue()
						}),
						protocmp.SortRepeated(func(e1, e2 *d4pb.Extension) bool {
							return e1.GetUrl().GetValue() < e2.GetUrl().GetValue()
						}),
						protocmp.SortRepeated(func(e1, e2 *d5pb.Extension) bool {
							return e1.GetUrl().GetValue() < e2.GetUrl().GetValue()
						}),
					}
					if diff := cmp.Diff(w.r, got, sortExtensions, protocmp.Transform()); diff != "" {
						t.Errorf("unmarshal %v, got diff(-want +got): %s", test.name, diff)
					}

					// test UnmarshalWithOutcome
					got, mvo, err := u.UnmarshalWithOutcome(test.json)
					if err != nil {
						t.Fatalf("unmarshalWithOutcome %v failed with unexpected error: %v", test.name, err)
					}
					if diff := cmp.Diff(w.r, got, sortExtensions, protocmp.Transform()); diff != "" {
						t.Errorf("unmarshalWithOutcome %v, got diff(-want +got): %s", test.name, diff)
					}
					var ic int
					switch w.ver {
					case fhirversion.STU3:
						ic = len(mvo.R3Outcome.GetIssue())
					case fhirversion.R4:
						ic = len(mvo.R4Outcome.GetIssue())
					case fhirversion.R5:
						ic = len(mvo.R5Outcome.GetIssue())
					default:
						t.Fatalf("unmarshalWithOutcome %v had unexpected version %v", test.name, w.ver)
					}
					if ic != 0 {
						t.Errorf("unmarshalWithOutcome %v got unexpected errors, expected issue number 0, got %v", test.name, ic)
					}

					// test UnmarshalWithErrorReporter
					er := errorreporter.NewOperationErrorReporter(w.ver)
					got, err = u.UnmarshalWithErrorReporter(test.json, er)
					if err != nil {
						t.Fatalf("UnmarshalWithErrorReporter %v failed with unexpected error: %v", test.name, err)
					}
					if diff := cmp.Diff(w.r, got, sortExtensions, protocmp.Transform()); diff != "" {
						t.Errorf("UnmarshalWithErrorReporter %v, got diff(-want +got): %s", test.name, diff)
					}
					switch w.ver {
					case fhirversion.STU3:
						ic = len(er.Outcome.R3Outcome.GetIssue())
					case fhirversion.R4:
						ic = len(er.Outcome.R4Outcome.GetIssue())
					case fhirversion.R5:
						ic = len(er.Outcome.R5Outcome.GetIssue())
					default:
						t.Fatalf("UnmarshalWithErrorReporter %v had unexpected version %v", test.name, w.ver)
					}
					if ic != 0 {
						t.Errorf("UnmarshalWithErrorReporter %v got unexpected errors, expected issue number 0, got %v", test.name, ic)
					}
				})
			}
		})
	}
}

func TestUnmarshal_NoExtendedValidation(t *testing.T) {
	tests := []struct {
		name  string
		json  []byte
		wants []mvr
	}{
		{
			name: "resource missing required fields",
			json: []byte(`
    {
      "resourceType": "Patient",
      "link": [{}]
    }`),
			wants: []mvr{
				{
					ver: fhirversion.STU3,
					r: &r3pb.ContainedResource{
						OneofResource: &r3pb.ContainedResource_Patient{
							Patient: &r3pb.Patient{
								Link: []*r3pb.Patient_Link{{}},
							},
						},
					},
				},
				{
					ver: fhirversion.R4,
					r: &r4pb.ContainedResource{
						OneofResource: &r4pb.ContainedResource_Patient{
							Patient: &r4patientpb.Patient{
								Link: []*r4patientpb.Patient_Link{{}},
							},
						},
					},
				},
				{
					ver: fhirversion.R5,
					r: &r5pb.ContainedResource{
						OneofResource: &r5pb.ContainedResource_Patient{
							Patient: &r5patientpb.Patient{
								Link: []*r5patientpb.Patient_Link{{}},
							},
						},
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
			wants: []mvr{
				{
					ver: fhirversion.STU3,
					r: &r3pb.ContainedResource{
						OneofResource: &r3pb.ContainedResource_Patient{
							Patient: &r3pb.Patient{
								ManagingOrganization: &d3pb.Reference{
									Reference: &d3pb.Reference_PatientId{
										PatientId: &d3pb.ReferenceId{Value: "1"},
									},
								},
							},
						},
					},
				},
				{
					ver: fhirversion.R4,
					r: &r4pb.ContainedResource{
						OneofResource: &r4pb.ContainedResource_Patient{
							Patient: &r4patientpb.Patient{
								ManagingOrganization: &d4pb.Reference{
									Reference: &d4pb.Reference_PatientId{
										PatientId: &d4pb.ReferenceId{Value: "1"},
									},
								},
							},
						},
					},
				},
				{
					ver: fhirversion.R5,
					r: &r5pb.ContainedResource{
						OneofResource: &r5pb.ContainedResource_Patient{
							Patient: &r5patientpb.Patient{
								ManagingOrganization: &d5pb.Reference{
									Reference: &d5pb.Reference_PatientId{
										PatientId: &d5pb.ReferenceId{Value: "1"},
									},
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
			for _, w := range test.wants {
				t.Run(w.ver.String(), func(t *testing.T) {
					u := setupUnmarshaller(t, w.ver)
					_, err := u.Unmarshal(test.json)
					if err == nil {
						t.Fatalf("unmarshal %v should have failed with extended validation", test.name)
					}

					u, err = NewUnmarshallerWithoutValidation("America/Los_Angeles", w.ver)
					if err != nil {
						t.Fatalf("failed to create unmarshaler; %v", err)
					}
					got, err := u.Unmarshal(test.json)
					if err != nil {
						t.Fatalf("unmarshal %v failed: %v", test.name, err)
					}
					if !proto.Equal(got, w.r) {
						t.Errorf("unmarshal %v: got %v, want %v", test.name, got, w.r)
					}
				})
			}
		})
	}
}

func TestUnmarshalWithOutcome_Errors(t *testing.T) {
	tests := []struct {
		name         string
		json         []byte
		wantOutcomes []*errorreporter.MultiVersionOperationOutcome
		wantMsgs     []proto.Message
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
				&errorreporter.MultiVersionOperationOutcome{
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
								Diagnostics: &d5pb.String{Value: `error at "Patient.managingOrganization": invalid reference to a Patient resource, want Organization`},
								Expression: []*d5pb.String{
									&d5pb.String{Value: `Patient.managingOrganization`},
								},
							},
						},
					},
				},
			},
			wantMsgs: []proto.Message{
				&r3pb.ContainedResource{
					OneofResource: &r3pb.ContainedResource_Patient{
						Patient: &r3pb.Patient{
							ManagingOrganization: &d3pb.Reference{
								Reference: &d3pb.Reference_PatientId{
									PatientId: &d3pb.ReferenceId{Value: "1"},
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
									PatientId: &d4pb.ReferenceId{Value: "1"},
								},
							},
						},
					},
				},
				&r5pb.ContainedResource{
					OneofResource: &r5pb.ContainedResource_Patient{
						Patient: &r5patientpb.Patient{
							ManagingOrganization: &d5pb.Reference{
								Reference: &d5pb.Reference_PatientId{
									PatientId: &d5pb.ReferenceId{Value: "1"},
								},
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
				&errorreporter.MultiVersionOperationOutcome{
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
								Diagnostics: &d5pb.String{Value: `error at "Patient.link[0]": missing required field "other"`},
								Expression: []*d5pb.String{
									&d5pb.String{Value: `Patient.link[0]`},
								},
							},
							&r5outcomepb.OperationOutcome_Issue{
								Code: &r5outcomepb.OperationOutcome_Issue_CodeType{
									Value: c5pb.IssueTypeCode_VALUE,
								},
								Severity: &r5outcomepb.OperationOutcome_Issue_SeverityCode{
									Value: c5pb.IssueSeverityCode_ERROR,
								},
								Diagnostics: &d5pb.String{Value: `error at "Patient.link[0]": missing required field "type"`},
								Expression: []*d5pb.String{
									&d5pb.String{Value: `Patient.link[0]`},
								},
							},
						},
					},
				},
			},
			wantMsgs: []proto.Message{
				&r3pb.ContainedResource{
					OneofResource: &r3pb.ContainedResource_Patient{
						Patient: &r3pb.Patient{
							Link: []*r3pb.Patient_Link{{}},
						},
					},
				},
				&r4pb.ContainedResource{
					OneofResource: &r4pb.ContainedResource_Patient{
						Patient: &r4patientpb.Patient{
							Link: []*r4patientpb.Patient_Link{{}},
						},
					},
				},
				&r5pb.ContainedResource{
					OneofResource: &r5pb.ContainedResource_Patient{
						Patient: &r5patientpb.Patient{
							Link: []*r5patientpb.Patient_Link{{}},
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
					wantMsg := test.wantMsgs[i]
					u := setupUnmarshaller(t, wantOut.Version)
					gotRes, gotOut, err := u.UnmarshalWithOutcome(test.json)
					if err != nil {
						t.Fatalf("unmarshal with outcome %v failed: %v", test.name, err)
					}
					if diff := cmp.Diff(gotOut, wantOut, protocmp.Transform()); diff != "" {
						t.Errorf("unmarshal with outcome %v: got outcome %v, want %v", test.name, gotOut, wantOut)
					}
					if !proto.Equal(gotRes, wantMsg) {
						t.Errorf("unmarshal with outcome %v: got resource %v, want %v", test.name, gotRes, wantMsg)
					}
				})
			}
		})
	}
}

func TestUnmarshalWithErrorReporter_Errors(t *testing.T) {
	tests := []struct {
		name         string
		json         []byte
		wantOutcomes []*errorreporter.MultiVersionOperationOutcome
		wantMsgs     []proto.Message
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
				&errorreporter.MultiVersionOperationOutcome{
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
								Diagnostics: &d5pb.String{Value: `error at "Patient.managingOrganization": invalid reference to a Patient resource, want Organization`},
								Expression: []*d5pb.String{
									&d5pb.String{Value: `Patient.managingOrganization`},
								},
							},
						},
					},
				},
			},
			wantMsgs: []proto.Message{
				&r3pb.ContainedResource{
					OneofResource: &r3pb.ContainedResource_Patient{
						Patient: &r3pb.Patient{
							ManagingOrganization: &d3pb.Reference{
								Reference: &d3pb.Reference_PatientId{
									PatientId: &d3pb.ReferenceId{Value: "1"},
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
									PatientId: &d4pb.ReferenceId{Value: "1"},
								},
							},
						},
					},
				},
				&r5pb.ContainedResource{
					OneofResource: &r5pb.ContainedResource_Patient{
						Patient: &r5patientpb.Patient{
							ManagingOrganization: &d5pb.Reference{
								Reference: &d5pb.Reference_PatientId{
									PatientId: &d5pb.ReferenceId{Value: "1"},
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
					wantMsg := test.wantMsgs[i]
					u := setupUnmarshaller(t, wantOut.Version)
					oer := errorreporter.NewOperationErrorReporter(wantOut.Version)
					gotRes, err := u.UnmarshalWithErrorReporter(test.json, oer)
					if err != nil {
						t.Fatalf("unmarshal with outcome %v failed: %v", test.name, err)
					}
					if diff := cmp.Diff(oer.Outcome, wantOut, protocmp.Transform()); diff != "" {
						t.Errorf("unmarshal with outcome %v: got outcome %v, want %v", test.name, oer.Outcome, wantOut)
					}
					if !proto.Equal(gotRes, wantMsg) {
						t.Errorf("unmarshal with outcome %v: got resource %v, want %v", test.name, gotRes, wantMsg)
					}
				})
			}
		})
	}
}

func TestUnmarshal_ErrorsFromFile(t *testing.T) {
	tests := []struct {
		name     string
		jsonFile string
		vers     []fhirversion.Version
		// Due to the random ordering of map keys we don't know which key will be
		// processed first, but for extension fields it doesn't matter if the
		// primitive or extension is processed first, both will cause an error but
		// the error will be different.
		errs []string
	}{
		{
			name:     "Multiple choice[x] values provided",
			jsonFile: "observation_invalid_multiple_choicex_provided.json",
			vers:     allVers,
			errs:     []string{`error at "Observation": cannot accept multiple values for value field`},
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			for _, v := range test.vers {
				t.Run(v.String(), func(t *testing.T) {
					u := setupUnmarshaller(t, v)
					json, err := readFile(basePath, test.jsonFile)
					if err != nil {
						t.Fatalf("Failed to get JSON data for test case %v", err)
					}
					_, err = u.Unmarshal(json)
					if err == nil {
						t.Fatalf("unmarshal %s failed: got error < nil >, want %q", test.name, test.errs)
					}
					matched := false
					for _, wantErr := range test.errs {
						if err.Error() == wantErr {
							matched = true
							break
						}
					}
					if !matched {
						t.Errorf("unmarshal %s: got error %q, want one of: %q", test.name, err.Error(), strings.Join(test.errs, ", "))
					}
				})
			}
		})
	}
}

func TestUnmarshal_Errors(t *testing.T) {
	tests := []struct {
		name string
		json string
		vers []fhirversion.Version
		// Due to the random ordering of map keys we don't know which key will be
		// processed first, but for extension fields it doesn't matter if the
		// primitive or extension is processed first, both will cause an error but
		// the error will be different.
		errs []string
	}{
		{
			name: "Repeated instead of primitive",
			json: `
		{
      "resourceType": "Patient",
			"gender": ["male", "female"]
    }`,
			vers: allVers,
			errs: []string{`error at "Patient.gender": invalid value \(expected a (AdministrativeGenderCode|GenderCode) object\)`},
		},
		{
			name: "Invalid code",
			json: `
		{
      "resourceType": "Patient",
			"gender": "f"
    }`,
			vers: allVers,
			errs: []string{`error at "Patient.gender": code type mismatch`},
		},
		{
			name: "Incorrect primitive type",
			json: `
		{
      "resourceType": "Patient",
			"gender": true
    }`,
			vers: allVers,
			errs: []string{`error at "Patient.gender": expected code`},
		},
		{
			name: "Invalid JSON",
			json: `
		{
      "resourceType": "Patient",
    }`,
			vers: allVers,
			errs: []string{`invalid JSON`},
		},
		{
			name: "Invalid resource type",
			json: `
		{
      "resourceType": 1
    }`,
			vers: allVers,
			errs: []string{"invalid resource type"},
		},
		{
			name: "Unknown resource type",
			json: `
		{
      "resourceType": "Patient1"
    }`,
			vers: allVers,
			errs: []string{`error at "Patient1": unknown resource type`},
		},
		{
			name: "Missing resource type",
			json: "{}",
			vers: allVers,
			errs: []string{`missing required field "resourceType"`},
		},
		{
			name: "Unknown array field",
			json: `
		{
      "resourceType": "Patient",
			"foo": [1, 2]
    }`,
			vers: allVers,
			errs: []string{`error at "Patient": unknown field`},
		},
		{
			name: "Unknown camel-cased field",
			json: `
		{
      "resourceType": "Patient",
			"fooBar": "1"
    }`,
			vers: allVers,
			errs: []string{`error at "Patient": unknown field`},
		},
		{
			name: "Extension of non-primitive field",
			json: `
		{
      "resourceType": "Patient",
			"managingOrganization": {"reference": "Org/1"},
			"_managingOrganization": {"foo": "bar"}
    }`,
			vers: allVers,
			errs: []string{`error at "Patient": unknown field`},
		},
		{
			name: "Extension type mismatch",
			json: `
		{
      "resourceType": "Patient",
			"name": [{
          "given": ["Pat"],
          "_given": {"id": "1"}
      }]
    }`,
			vers: allVers,
			errs: []string{`error at "Patient.name[0]._given": expected array`},
		},
		{
			name: "Extension length mismatch",
			json: `
			{
				"resourceType": "Patient",
				"name": [{
					"given": ["Pat"],
					"_given": [{"id": "1"}, {"id": "2"}]
				}]
			}`,
			vers: allVers,
			errs: []string{
				`error at "Patient.name[0]._given": array length mismatch, expected 1, found 2`,
				`error at "Patient.name[0].given": array length mismatch, expected 2, found 1`,
			},
		},
		{
			name: "unknown field in oneOf",
			json: `
		{
      "resourceType": "Patient",
			"managingOrganization": {"foo": "bar"}
    }`,
			vers: allVers,
			errs: []string{`error at "Patient.managingOrganization": unknown field`},
		},
		{
			name: "error in array",
			json: `
		{
      "resourceType": "Patient",
			"name": [{
				"given": [1]
			}]
    }`,
			vers: allVers,
			errs: []string{`error at "Patient.name[0].given[0]": expected string`},
		},
		{
			name: "R3 error in nested object",
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
			vers: []fhirversion.Version{fhirversion.STU3},
			errs: []string{`error at "Patient.animal.species.coding": expected array`},
		},
		{
			name: "R4/R5 error in nested object",
			json: `
		{
      "resourceType": "Patient",
      "communication": [
				{
					"language": {
						"coding": 1
					}
      	}
			]
    }
    `,
			vers: []fhirversion.Version{fhirversion.R4, fhirversion.R5},
			errs: []string{`error at "Patient.communication[0].language.coding": expected array`},
		},
		{
			name: "invalid character in string",
			json: `
		{
      "resourceType": "Patient",
			"name": [{
				"given": ["Pat\u0008"]
			}]
    }
    `,
			vers: allVers,
			errs: []string{`error at "Patient.name[0].given[0]": string contains invalid characters: U+0008`},
		},
		{
			name: "invalid uri",
			json: `
		{
      "resourceType": "Patient",
			"implicitRules": "http://\u0000"
    }
    `,
			vers: allVers,
			errs: []string{`error at "Patient.implicitRules": invalid uri`},
		},
		{
			name: "invalid uri 2",
			json: `
		{
      "resourceType": "Patient",
			"implicitRules": " http://example.com/"
    }
    `,
			vers: allVers,
			errs: []string{`error at "Patient.implicitRules": invalid uri`},
		},
		{
			name: "invalid value for choice type",
			json: `
		{
      "resourceType": "Observation",
			"effectiveDateTime": "invalid"
    }
    `,
			vers: allVers,
			errs: []string{`error at "Observation.effectiveDateTime": expected datetime`},
		},
		{
			name: "missing value type for choice type",
			json: `
		{
			"resourceType": "Patient",
			"extension": [{
				"url": "https://example.com",
				"value": "x"
			}]
		}`,
			vers: allVers,
			errs: []string{`error at "Patient.extension[0]": unknown field`},
		},
		{
			name: "string field contains invalid UTF-8",
			json: `
			{
				"resourceType": "Patient",
				"name": [{ "text": "` + "\xa0\xa1" + `"}]
			}
			`,
			vers: allVers,
			errs: []string{`error at "Patient.name[0].text": expected UTF-8 encoding`},
		},
		{
			name: "code field contains invalid UTF-8",
			json: `
			{
				"resourceType": "Patient",
				"language": "` + "\xa0\xa1" + `"
			}
			`,
			vers: allVers,
			errs: []string{`error at "Patient.language": expected UTF-8 encoding`},
		},
		// TODO(b/161479338): add test for rejecting upper camel case fields once deprecated.
		{
			name: "all upper case is invalid",
			json: `
			{
				"resourceType": "Patient",
				"GENDER": "female"
			}`,
			vers: allVers,
			errs: []string{`error at "Patient": unknown field`},
		},
		{
			name: "all lower case is invalid for two or more word field",
			json: `
			{
				"resourceType": "Patient",
				"managingorganization": {"reference": "Org/1"}
			}`,
			vers: allVers,
			errs: []string{`error at "Patient": unknown field`},
		},
		{
			name: "invalid mixed case",
			json: `
			{
				"resourceType": "Patient",
				"gEnDeR": "female"
			}`,
			vers: allVers,
			errs: []string{`error at "Patient": unknown field`},
		},
		{
			name: "leading upper case resourceType",
			json: `
			{
				"ResourceType": "Patient",
				"gender": "female"
			}`,
			vers: allVers,
			errs: []string{`missing required field "resourceType"`},
		},
		{
			name: "leading upper case contained",
			json: `{
			"Contained":[
				{
					"id":"nested",
					"resourceType":"Patient"
				}
			],
			"id":"example",
			"resourceType":"Patient"
			}`,
			vers: []fhirversion.Version{fhirversion.R4},
			errs: []string{`error at "Patient.Contained[0]": unknown field` + "\n" + `error at "Patient.Contained[0]": unknown field`},
		},
		{
			name: "directly assigning primitive type's value field",
			json: `
			{
				"resourceType": "Patient",
				"gender": {
					"value": "female"
				}
			}`,
			vers: allVers,
			errs: []string{`error at "Patient.gender.value": invalid field`},
		},
		{
			name: "invalid reference proto type",
			json: `
    	{
    		"resourceType": "Patient",
				"managingOrganization": {
					"organizationId": "2810efe9-f993-489d-8d07-86ad32e54923"
				}
			}`,
			vers: allVers,
			errs: []string{`error at "Patient.managingOrganization.organizationId": invalid type: ReferenceId`},
		},
		{
			name: "trailing characters",
			json: `{"resourceType": "Patient"}{}`,
			vers: allVers,
			errs: []string{"invalid JSON"},
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			for _, v := range test.vers {
				t.Run(v.String(), func(t *testing.T) {
					u := setupUnmarshaller(t, v)
					_, err := u.Unmarshal([]byte(test.json))
					if err == nil {
						t.Fatalf("unmarshal %s failed: got error < nil >, want %q", test.name, test.errs)
					}
					matched := false
					for _, wantErr := range test.errs {
						if err.Error() == wantErr || regexp.MustCompile(wantErr).Match([]byte(err.Error())) {
							matched = true
							break
						}
					}
					if !matched {
						t.Errorf("unmarshal %s: got error %q, want one of: %q", test.name, err.Error(), strings.Join(test.errs, ", "))
					}
				})
			}
		})
	}
}

func TestUnmarshal_ExtendedValidation_Errors(t *testing.T) {
	tests := []struct {
		name string
		json string
		err  error
		vers []fhirversion.Version
	}{
		{
			"Missing required field",
			`
		{
		  "resourceType": "Patient",
		  "link": [{}]
		}`,
			jsonpbhelper.UnmarshalErrorList{
				{Path: "Patient.link[0]", Details: `missing required field "other"`, Type: jsonpbhelper.RequiredFieldError},
				{Path: "Patient.link[0]", Details: `missing required field "type"`, Type: jsonpbhelper.RequiredFieldError},
			},
			allVers,
		},
		{
			"Missing required field in oneof",
			`
		{
			"valueQuantity": {
				"extension": [{"valueCode": "foo"}]
			},
      "resourceType": "Observation",
			"status": "final",
			"code": {
				"text": "bar"
			}
    }`,
			&jsonpbhelper.UnmarshalError{Path: "Observation.valueQuantity.extension[0]", Details: `missing required field "url"`, Type: jsonpbhelper.RequiredFieldError},
			allVers,
		},
		{
			"Missing required repeated field",
			`
		{
		  "resourceType": "OperationOutcome"
		}`,
			&jsonpbhelper.UnmarshalError{Path: "OperationOutcome", Details: `missing required field "issue"`, Type: jsonpbhelper.RequiredFieldError},
			allVers,
		},
		// Covers possible issues with collecting requirements from datatype protos.
		{
			"Missing required field in datatype",
			`{
				"resourceType": "Patient",
				"extension": [{
					"valueReference": {"reference": "Patient/1"}
				}]
			}`,
			&jsonpbhelper.UnmarshalError{Path: "Patient.extension[0]", Details: `missing required field "url"`, Type: jsonpbhelper.RequiredFieldError},
			allVers,
		},
		{
			"Invalid reference type",
			`
			{
				"resourceType": "Patient",
				"managingOrganization": {"reference": "Patient/2"}
			}`,
			&jsonpbhelper.UnmarshalError{Path: "Patient.managingOrganization", Details: `invalid reference to a Patient resource, want Organization`, Type: jsonpbhelper.ReferenceTypeError},
			allVers,
		},
		{
			"Invalid bundle",
			`
			{
				"resourceType": "Bundle",
				"type": "searchset",
				"entry": [{
					"resource": {
						"resourceType": "Observation",
						"extension": 1
					}
				}]
			}`,
			&jsonpbhelper.UnmarshalError{Path: "Bundle.entry[0].resource.ofType(Observation).extension", Details: `expected array`},
			allVers,
		},
		{
			"Invalid contained resource",
			`
			{
				"resourceType": "Patient",
				"contained": [{
					"resourceType": "Observation",
					"status": "foo"
				}]
			}`,
			&jsonpbhelper.UnmarshalError{Path: "Patient.contained[0].ofType(Observation).status", Details: `code type mismatch`, Diagnostics: `"foo" is not a ObservationStatusCode`},
			[]fhirversion.Version{fhirversion.STU3},
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			for _, v := range test.vers {
				t.Run(v.String(), func(t *testing.T) {
					u := setupUnmarshaller(t, v)
					_, err := u.Unmarshal([]byte(test.json))
					if err == nil {
						t.Fatalf("unmarshal %s got nil error, want: %v", test.name, test.err)
					}
					if diff := cmp.Diff(test.err, err, unnestErrorListOpt); diff != "" {
						t.Errorf("unmarshal %s: got error %q, want %q\ndiff: %s", test.name, err.Error(), test.err, diff)
					}
				})
			}
		})
	}
}

func TestParsePrimitiveType(t *testing.T) {
	tests := []struct {
		pType string
		value json.RawMessage
		wants []mvr
	}{
		{
			pType: "Base64Binary",
			value: json.RawMessage(`"YmFzZTY0IGJ5dGVz"`),
			wants: []mvr{
				{
					ver: fhirversion.STU3,
					r: &d3pb.Base64Binary{
						Value: []byte("base64 bytes"),
					},
				},
				{
					ver: fhirversion.R4,
					r: &d4pb.Base64Binary{
						Value: []byte("base64 bytes"),
					},
				},
				{
					ver: fhirversion.R5,
					r: &d5pb.Base64Binary{
						Value: []byte("base64 bytes"),
					},
				},
			},
		},
		{
			pType: "Boolean",
			value: json.RawMessage(`true`),
			wants: []mvr{
				{
					ver: fhirversion.STU3,
					r: &d3pb.Boolean{
						Value: true,
					},
				},
				{
					ver: fhirversion.R4,
					r: &d4pb.Boolean{
						Value: true,
					},
				},
				{
					ver: fhirversion.R5,
					r: &d5pb.Boolean{
						Value: true,
					},
				},
			},
		},
		{
			pType: "Canonical",
			value: json.RawMessage(`"c"`),
			wants: []mvr{
				{
					ver: fhirversion.R4,
					r: &d4pb.Canonical{
						Value: "c",
					},
				},
				{
					ver: fhirversion.R5,
					r: &d5pb.Canonical{
						Value: "c",
					},
				},
			},
		},
		{
			pType: "Code",
			value: json.RawMessage(`"<some code>"`),
			wants: []mvr{
				{
					ver: fhirversion.STU3,
					r: &d3pb.Code{
						Value: "<some code>",
					},
				},
				{
					ver: fhirversion.R4,
					r: &d4pb.Code{
						Value: "<some code>",
					},
				},
				{
					ver: fhirversion.R5,
					r: &d5pb.Code{
						Value: "<some code>",
					},
				},
			},
		},
		{
			pType: "Id",
			value: json.RawMessage(`"abc123"`),
			wants: []mvr{
				{
					ver: fhirversion.STU3,
					r: &d3pb.Id{
						Value: "abc123",
					},
				},
				{
					ver: fhirversion.R4,
					r: &d4pb.Id{
						Value: "abc123",
					},
				},
				{
					ver: fhirversion.R5,
					r: &d5pb.Id{
						Value: "abc123",
					},
				},
			},
		},
		{
			pType: "Integer",
			value: json.RawMessage(`-1234`),
			wants: []mvr{
				{
					ver: fhirversion.STU3,
					r: &d3pb.Integer{
						Value: -1234,
					},
				},
				{
					ver: fhirversion.R4,
					r: &d4pb.Integer{
						Value: -1234,
					},
				},
				{
					ver: fhirversion.R5,
					r: &d5pb.Integer{
						Value: -1234,
					},
				},
			},
		},
		{
			pType: "Integer64",
			value: json.RawMessage(`-1234`),
			wants: []mvr{
				{
					ver: fhirversion.R5,
					r: &d5pb.Integer64{
						Value: -1234,
					},
				},
			},
		},
		{
			pType: "Markdown",
			value: json.RawMessage(`"# md"`),
			wants: []mvr{
				{
					ver: fhirversion.STU3,
					r: &d3pb.Markdown{
						Value: "# md",
					},
				},
				{
					ver: fhirversion.R4,
					r: &d4pb.Markdown{
						Value: "# md",
					},
				},
				{
					ver: fhirversion.R5,
					r: &d5pb.Markdown{
						Value: "# md",
					},
				},
			},
		},
		{
			pType: "Oid",
			value: json.RawMessage(`"urn:oid:1.23"`),
			wants: []mvr{
				{
					ver: fhirversion.STU3,
					r: &d3pb.Oid{
						Value: "urn:oid:1.23",
					},
				},
				{
					ver: fhirversion.R4,
					r: &d4pb.Oid{
						Value: "urn:oid:1.23",
					},
				},
				{
					ver: fhirversion.R5,
					r: &d5pb.Oid{
						Value: "urn:oid:1.23",
					},
				},
			},
		},
		{
			pType: "Uuid",
			value: json.RawMessage(`"urn:uuid:f81d4fae-7dec-11d0-a765-00a0c91e6bf6"`),
			wants: []mvr{
				{
					ver: fhirversion.STU3,
					r: &d3pb.Uuid{
						Value: "urn:uuid:f81d4fae-7dec-11d0-a765-00a0c91e6bf6",
					},
				},
				{
					ver: fhirversion.R4,
					r: &d4pb.Uuid{
						Value: "urn:uuid:f81d4fae-7dec-11d0-a765-00a0c91e6bf6",
					},
				},
				{
					ver: fhirversion.R5,
					r: &d5pb.Uuid{
						Value: "urn:uuid:f81d4fae-7dec-11d0-a765-00a0c91e6bf6",
					},
				},
			},
		},
		{
			pType: "PositiveInt",
			value: json.RawMessage(`5678`),
			wants: []mvr{
				{
					ver: fhirversion.STU3,
					r: &d3pb.PositiveInt{
						Value: 5678,
					},
				},
				{
					ver: fhirversion.R4,
					r: &d4pb.PositiveInt{
						Value: 5678,
					},
				},
				{
					ver: fhirversion.R5,
					r: &d5pb.PositiveInt{
						Value: 5678,
					},
				},
			},
		},
		{
			pType: "String",
			value: json.RawMessage(`"a given string"`),
			wants: []mvr{
				{
					ver: fhirversion.STU3,
					r: &d3pb.String{
						Value: "a given string",
					},
				},
				{
					ver: fhirversion.R4,
					r: &d4pb.String{
						Value: "a given string",
					},
				},
				{
					ver: fhirversion.R5,
					r: &d5pb.String{
						Value: "a given string",
					},
				},
			},
		},
		{
			pType: "UnsignedInt",
			value: json.RawMessage(`90`),
			wants: []mvr{
				{
					ver: fhirversion.STU3,
					r: &d3pb.UnsignedInt{
						Value: 90,
					},
				},
				{
					ver: fhirversion.R4,
					r: &d4pb.UnsignedInt{
						Value: 90,
					},
				},
				{
					ver: fhirversion.R5,
					r: &d5pb.UnsignedInt{
						Value: 90,
					},
				},
			},
		},
		{
			pType: "Date",
			value: json.RawMessage(`"2017"`),
			wants: []mvr{
				{
					ver: fhirversion.STU3,
					r: &d3pb.Date{
						ValueUs:   1483257600000000,
						Precision: d3pb.Date_YEAR,
						Timezone:  "America/Los_Angeles",
					},
				},
				{
					ver: fhirversion.R4,
					r: &d4pb.Date{
						ValueUs:   1483257600000000,
						Precision: d4pb.Date_YEAR,
						Timezone:  "America/Los_Angeles",
					},
				},
				{
					ver: fhirversion.R5,
					r: &d5pb.Date{
						ValueUs:   1483257600000000,
						Precision: d5pb.Date_YEAR,
						Timezone:  "America/Los_Angeles",
					},
				},
			},
		},
		{
			pType: "DateTime",
			value: json.RawMessage(`"2018"`),
			wants: []mvr{
				{
					ver: fhirversion.STU3,
					r: &d3pb.DateTime{
						ValueUs:   1514793600000000,
						Precision: d3pb.DateTime_YEAR,
						Timezone:  "America/Los_Angeles",
					},
				},
				{
					ver: fhirversion.R4,
					r: &d4pb.DateTime{
						ValueUs:   1514793600000000,
						Precision: d4pb.DateTime_YEAR,
						Timezone:  "America/Los_Angeles",
					},
				},
				{
					ver: fhirversion.R5,
					r: &d5pb.DateTime{
						ValueUs:   1514793600000000,
						Precision: d5pb.DateTime_YEAR,
						Timezone:  "America/Los_Angeles",
					},
				},
			},
		},
		{
			pType: "DateTime2",
			value: json.RawMessage(`"2018-01-01"`),
			wants: []mvr{
				{
					ver: fhirversion.STU3,
					r: &d3pb.DateTime{
						ValueUs:   1514793600000000,
						Precision: d3pb.DateTime_DAY,
						Timezone:  "America/Los_Angeles",
					},
				},
				{
					ver: fhirversion.R4,
					r: &d4pb.DateTime{
						ValueUs:   1514793600000000,
						Precision: d4pb.DateTime_DAY,
						Timezone:  "America/Los_Angeles",
					},
				},
				{
					ver: fhirversion.R5,
					r: &d5pb.DateTime{
						ValueUs:   1514793600000000,
						Precision: d5pb.DateTime_DAY,
						Timezone:  "America/Los_Angeles",
					},
				},
			},
		},
		{
			pType: "Time",
			value: json.RawMessage(`"12:00:00"`),
			wants: []mvr{
				{
					ver: fhirversion.STU3,
					r: &d3pb.Time{
						ValueUs:   43200000000,
						Precision: d3pb.Time_SECOND,
					},
				},
				{
					ver: fhirversion.R4,
					r: &d4pb.Time{
						ValueUs:   43200000000,
						Precision: d4pb.Time_SECOND,
					},
				},
				{
					ver: fhirversion.R5,
					r: &d5pb.Time{
						ValueUs:   43200000000,
						Precision: d5pb.Time_SECOND,
					},
				},
			},
		},
		{
			pType: "Instant",
			value: json.RawMessage(`"2018-01-01T12:00:00.000Z"`),
			wants: []mvr{
				{
					ver: fhirversion.STU3,
					r: &d3pb.Instant{
						ValueUs:   1514808000000000,
						Precision: d3pb.Instant_MILLISECOND,
						Timezone:  "Z",
					},
				},
				{
					ver: fhirversion.R4,
					r: &d4pb.Instant{
						ValueUs:   1514808000000000,
						Precision: d4pb.Instant_MILLISECOND,
						Timezone:  "Z",
					},
				},
				{
					ver: fhirversion.R5,
					r: &d5pb.Instant{
						ValueUs:   1514808000000000,
						Precision: d5pb.Instant_MILLISECOND,
						Timezone:  "Z",
					},
				},
			},
		},
		{
			pType: "Decimal",
			value: json.RawMessage(`1.23`),
			wants: []mvr{
				{
					ver: fhirversion.STU3,
					r: &d3pb.Decimal{
						Value: "1.23",
					},
				},
				{
					ver: fhirversion.R4,
					r: &d4pb.Decimal{
						Value: "1.23",
					},
				},
				{
					ver: fhirversion.R5,
					r: &d5pb.Decimal{
						Value: "1.23",
					},
				},
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
			wants: []mvr{
				{
					ver: fhirversion.STU3,
					r: &c3pb.AdministrativeGenderCode{
						Extension: []*d3pb.Extension{{
							Url: &d3pb.Uri{
								Value: "http://example#gender",
							},
						}, {
							Url: &d3pb.Uri{
								Value: "https://g.co/fhir/StructureDefinition/primitiveHasNoValue",
							},
							Value: &d3pb.Extension_ValueX{
								Choice: &d3pb.Extension_ValueX_Boolean{
									Boolean: &d3pb.Boolean{
										Value: true,
									},
								},
							},
						}},
					},
				},
				{
					ver: fhirversion.R4,
					r: &r4patientpb.Patient_GenderCode{
						Extension: []*d4pb.Extension{{
							Url: &d4pb.Uri{
								Value: "http://example#gender",
							},
						}, {
							Url: &d4pb.Uri{
								Value: "https://g.co/fhir/StructureDefinition/primitiveHasNoValue",
							},
							Value: &d4pb.Extension_ValueX{
								Choice: &d4pb.Extension_ValueX_Boolean{
									Boolean: &d4pb.Boolean{
										Value: true,
									},
								},
							},
						}},
					},
				},
				{
					ver: fhirversion.R5,
					r: &r5patientpb.Patient_GenderCode{
						Extension: []*d5pb.Extension{{
							Url: &d5pb.Uri{
								Value: "http://example#gender",
							},
						}, {
							Url: &d5pb.Uri{
								Value: "https://g.co/fhir/StructureDefinition/primitiveHasNoValue",
							},
							Value: &d5pb.Extension_ValueX{
								Choice: &d5pb.Extension_ValueX_Boolean{
									Boolean: &d5pb.Boolean{
										Value: true,
									},
								},
							},
						}},
					},
				},
			},
		},
	}
	for _, test := range tests {
		t.Run(test.pType, func(t *testing.T) {
			for _, w := range test.wants {
				t.Run(w.ver.String(), func(t *testing.T) {
					u := setupUnmarshaller(t, w.ver)
					value := make([]byte, len(test.value))
					copy(value, test.value)
					got, err := u.parsePrimitiveType("value", w.r.ProtoReflect(), value)
					if err != nil {
						t.Fatalf("parse primitive type: %v", jsonpbhelper.PrintUnmarshalError(err, -1))
					}
					if !cmp.Equal(got, w.r, protocmp.Transform()) {
						t.Errorf("parse primitive type %v: got %v, want %v", test.pType, got, w.r)
					}
				})
			}
		})
	}
}

func TestParseURIs(t *testing.T) {
	inputs := []mvr{
		{
			ver: fhirversion.STU3,
			r:   &d3pb.Uri{},
		},
		{
			ver: fhirversion.R4,
			r:   &d4pb.Uri{},
		},
		{
			ver: fhirversion.R5,
			r:   &d5pb.Uri{},
		},
	}
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
			for _, i := range inputs {
				t.Run(i.ver.String(), func(t *testing.T) {
					u := setupUnmarshaller(t, i.ver)
					r := proto.Clone(i.r)
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
		})
	}
}

func TestParsePrimitiveType_Errors(t *testing.T) {
	tests := []struct {
		name  string
		value json.RawMessage
		msgs  []mvr
	}{
		{
			name:  "Code",
			value: json.RawMessage(`false`),
			msgs: []mvr{
				{
					ver: fhirversion.STU3,
					r:   &d3pb.Code{},
				},
				{
					ver: fhirversion.R4,
					r:   &d4pb.Code{},
				},
				{
					ver: fhirversion.R5,
					r:   &d5pb.Code{},
				},
			},
		},
		{
			name:  "Oid - type",
			value: json.RawMessage(`0`),
			msgs: []mvr{
				{
					ver: fhirversion.STU3,
					r:   &d3pb.Oid{},
				},
				{
					ver: fhirversion.R4,
					r:   &d4pb.Oid{},
				},
				{
					ver: fhirversion.R5,
					r:   &d5pb.Oid{},
				},
			},
		},
		{
			name:  "PositiveInt",
			value: json.RawMessage(`-123`),
			msgs: []mvr{
				{
					ver: fhirversion.STU3,
					r:   &d3pb.PositiveInt{},
				},
				{
					ver: fhirversion.R4,
					r:   &d4pb.PositiveInt{},
				},
				{
					ver: fhirversion.R5,
					r:   &d5pb.PositiveInt{},
				},
			},
		},
		{
			name:  "PositiveInt overflow",
			value: json.RawMessage(strconv.FormatUint(math.MaxUint64, 10)),
			msgs: []mvr{
				{
					ver: fhirversion.R4,
					r:   &d4pb.PositiveInt{},
				},
				{
					ver: fhirversion.STU3,
					r:   &d3pb.PositiveInt{},
				},
				{
					ver: fhirversion.R5,
					r:   &d5pb.PositiveInt{},
				},
			},
		},
		{
			name:  "UnsignedInt 00",
			value: json.RawMessage(`00`),
			msgs: []mvr{
				{
					ver: fhirversion.STU3,
					r:   &d3pb.UnsignedInt{},
				},
			},
		},
		{
			name:  "UnsignedInt -123",
			value: json.RawMessage(`-123`),
			msgs: []mvr{
				{
					ver: fhirversion.STU3,
					r:   &d3pb.UnsignedInt{},
				},
			},
		},
		{
			name:  "UnsignedInt 0123",
			value: json.RawMessage(`0123`),
			msgs: []mvr{
				{
					ver: fhirversion.STU3,
					r:   &d3pb.UnsignedInt{},
				},
			},
		},
		{
			name:  "UnsignedInt overflow",
			value: json.RawMessage(strconv.FormatUint(math.MaxUint64, 10)),
			msgs: []mvr{
				{
					ver: fhirversion.R4,
					r:   &d4pb.UnsignedInt{},
				},
				{
					ver: fhirversion.STU3,
					r:   &d3pb.UnsignedInt{},
				},
				{
					ver: fhirversion.R5,
					r:   &d5pb.UnsignedInt{},
				},
			},
		},
		{
			name:  "BinaryData",
			value: json.RawMessage(`"invalid"`),
			msgs: []mvr{
				{
					ver: fhirversion.STU3,
					r:   &d3pb.Base64Binary{},
				},
			},
		},
		{
			name:  "Boolean",
			value: json.RawMessage(`"invalid"`),
			msgs: []mvr{
				{
					ver: fhirversion.STU3,
					r:   &d3pb.Boolean{},
				},
				{
					ver: fhirversion.R4,
					r:   &d4pb.Boolean{},
				},
				{
					ver: fhirversion.R5,
					r:   &d5pb.Boolean{},
				},
			},
		},
		{
			name:  "Instant no timezone",
			value: json.RawMessage(`"2018-01-01T12:00:00.000"`),
			msgs: []mvr{
				{
					ver: fhirversion.STU3,
					r:   &d3pb.Instant{},
				},
				{
					ver: fhirversion.R4,
					r:   &d4pb.Instant{},
				},
				{
					ver: fhirversion.R5,
					r:   &d5pb.Instant{},
				},
			},
		},
		{
			name:  "Instant no seconds",
			value: json.RawMessage(`"2018-01-01T12:00Z"`),
			msgs: []mvr{
				{
					ver: fhirversion.STU3,
					r:   &d3pb.Instant{},
				},
				{
					ver: fhirversion.R4,
					r:   &d4pb.Instant{},
				},
				{
					ver: fhirversion.R5,
					r:   &d5pb.Instant{},
				},
			},
		},
		{
			name:  "Decimal - leading 0",
			value: json.RawMessage(`01.23`),
			msgs: []mvr{
				{
					ver: fhirversion.STU3,
					r:   &d3pb.Decimal{},
				},
			},
		},
		{
			name:  "Integer",
			value: json.RawMessage(`1.0`),
			msgs: []mvr{
				{
					ver: fhirversion.STU3,
					r:   &d3pb.Integer{},
				},
			},
		},
		{
			name:  "Integer64",
			value: json.RawMessage(`1.0`),
			msgs: []mvr{
				{
					ver: fhirversion.R5,
					r:   &d5pb.Integer64{},
				},
			},
		},
		{
			name:  "Markdown",
			value: json.RawMessage(`0`),
			msgs: []mvr{
				{
					ver: fhirversion.STU3,
					r:   &d3pb.Markdown{},
				},
				{
					ver: fhirversion.R4,
					r:   &d4pb.Markdown{},
				},
				{
					ver: fhirversion.R5,
					r:   &d5pb.Markdown{},
				},
			},
		},
		{
			name:  "Uri",
			value: json.RawMessage(`0`),
			msgs: []mvr{
				{
					ver: fhirversion.STU3,
					r:   &d3pb.Uri{},
				},
				{
					ver: fhirversion.R4,
					r:   &d4pb.Uri{},
				},
				{
					ver: fhirversion.R5,
					r:   &d5pb.Uri{},
				},
			},
		},
		{
			name:  "Uuid",
			value: json.RawMessage(`0`),
			msgs: []mvr{
				{
					ver: fhirversion.STU3,
					r:   &d3pb.Uuid{},
				},
				{
					ver: fhirversion.R4,
					r:   &d4pb.Uuid{},
				},
				{
					ver: fhirversion.R5,
					r:   &d5pb.Uuid{},
				},
			},
		},
		{
			name:  "Xhtml",
			value: json.RawMessage(`0`),
			msgs: []mvr{
				{
					ver: fhirversion.STU3,
					r:   &d3pb.Xhtml{},
				},
				{
					ver: fhirversion.R4,
					r:   &d4pb.Xhtml{},
				},
				{
					ver: fhirversion.R5,
					r:   &d5pb.Xhtml{},
				},
			},
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			for _, msg := range test.msgs {
				t.Run(msg.ver.String(), func(t *testing.T) {
					u := setupUnmarshaller(t, msg.ver)
					_, err := u.parsePrimitiveType("value", msg.r.ProtoReflect(), test.value)
					if err == nil {
						t.Errorf("parsePrimitiveType() %v succeeded, expect error", test.name)
					}
				})
			}
		})
	}
}

func TestUnmarshalVersioned(t *testing.T) {
	patient := `{"resourceType":"Patient"}`

	u3 := setupUnmarshaller(t, fhirversion.STU3)
	if _, err := u3.UnmarshalR3([]byte(patient)); err != nil {
		t.Errorf("UnmarshalR3(%s) returned unexpected error; %v", patient, err)
	}
	if _, err := u3.UnmarshalR4([]byte(patient)); err == nil {
		t.Errorf("UnmarshalR4(%s) didn't return expected error", patient)
	}
	if _, err := u3.UnmarshalR5([]byte(patient)); err == nil {
		t.Errorf("UnmarshalR5(%s) didn't return expected error", patient)
	}

	u4 := setupUnmarshaller(t, fhirversion.R4)
	if _, err := u4.UnmarshalR4([]byte(patient)); err != nil {
		t.Errorf("UnmarshalR4(%s) returned unexpected error; %v", patient, err)
	}
	if _, err := u4.UnmarshalR3([]byte(patient)); err == nil {
		t.Errorf("UnmarshalR3(%s) didn't return expected error", patient)
	}
	if _, err := u4.UnmarshalR5([]byte(patient)); err == nil {
		t.Errorf("UnmarshalR5(%s) didn't return expected error", patient)
	}

	u5 := setupUnmarshaller(t, fhirversion.R5)
	if _, err := u5.UnmarshalR4([]byte(patient)); err == nil {
		t.Errorf("UnmarshalR3(%s) didn't return expected error", patient)
	}
	if _, err := u5.UnmarshalR3([]byte(patient)); err == nil {
		t.Errorf("UnmarshalR3(%s) didn't return expected error", patient)
	}
	if _, err := u5.UnmarshalR5([]byte(patient)); err != nil {
		t.Errorf("UnmarshalR3(%s) returned unexpected error; %v", patient, err)
	}

}

func TestUnmarshal_NestingDepth(t *testing.T) {
	extDepth3 := `
	{
		"resourceType": "Patient",
		"extension": [{
			"url": "depth-2",
			"extension": [{
				"url": "depth-3"
			}]
		}]
    }`
	bdlDepth6 := `
	{
		"resourceType": "Bundle",
		"type": "collection",
		"entry": [{
			"fullUrl": "Bundle/1",
			"resource": {
				"resourceType": "Bundle",
				"type": "collection",
				"entry":[{
					"fullUrl": "Patient/1",
					"resource": {
						"resourceType": "Patient"
					}
				}]
			}
		}]
    }`
	tests := []struct {
		name     string
		json     string
		maxDepth int
		wantErr  bool
	}{
		{
			name:     "check disabled",
			json:     extDepth3,
			maxDepth: 0,
			wantErr:  false,
		},
		{
			name:     "nested extension - below max",
			json:     extDepth3,
			maxDepth: 5,
			wantErr:  false,
		},
		{
			name:     "nested extension - equal max",
			json:     extDepth3,
			maxDepth: 3,
			wantErr:  false,
		},
		{
			name:     "nested extension - exceeded max",
			json:     extDepth3,
			maxDepth: 2,
			wantErr:  true,
		},
		{
			name:     "nested bundle - below max",
			json:     bdlDepth6,
			maxDepth: 7,
			wantErr:  false,
		},
		{
			name:     "nested bundle - equal max",
			json:     bdlDepth6,
			maxDepth: 6,
			wantErr:  false,
		},
		{
			name:     "nested bundle - exceeded max",
			json:     bdlDepth6,
			maxDepth: 5,
			wantErr:  true,
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			for _, v := range allVers {
				t.Run(v.String(), func(t *testing.T) {
					u := setupUnmarshaller(t, v)
					u.MaxNestingDepth = test.maxDepth
					_, err := u.Unmarshal([]byte(test.json))
					if err == nil && test.wantErr {
						t.Fatalf("unmarshal %s should have failed", test.name)
					}
					if err != nil && !test.wantErr {
						t.Fatalf("unmarshal %s failed: got error %v, want no error", test.name, err)
					}
				})
			}
		})
	}
}

func TestUnmarshaller_UnmarshalR4Streaming(t *testing.T) {
	t.Run("streaming unmarshal", func(t *testing.T) {
		json := `{"resourceType":"Patient", "id": "exampleID1"}
	{"resourceType":"Patient", "id": "exampleID2"}
	{"resourceType":"Patient", "id": "exampleID3"}`
		expectedResults := []*r4pb.ContainedResource{
			&r4pb.ContainedResource{
				OneofResource: &r4pb.ContainedResource_Patient{
					Patient: &r4patientpb.Patient{
						Id: &d4pb.Id{Value: "exampleID1"},
					},
				},
			},
			&r4pb.ContainedResource{
				OneofResource: &r4pb.ContainedResource_Patient{
					Patient: &r4patientpb.Patient{
						Id: &d4pb.Id{Value: "exampleID2"},
					},
				},
			},
			&r4pb.ContainedResource{
				OneofResource: &r4pb.ContainedResource_Patient{
					Patient: &r4patientpb.Patient{
						Id: &d4pb.Id{Value: "exampleID3"},
					},
				},
			}}

		u, err := NewUnmarshaller("America/Los_Angeles", fhirversion.R4)
		if err != nil {
			fmt.Println("error")
		}

		jsonReader := bytes.NewReader([]byte(json))
		resourceChan := u.UnmarshalR4Streaming(jsonReader)

		var results []*r4pb.ContainedResource

		for r := range resourceChan {
			if r.Error != nil {
				t.Fatalf("UnmarshalR4Streaming(%s) unexpected error when receiving from the output channel: %v", json, r.Error)
			} else {
				results = append(results, r.ContainedResource)
			}
		}

		// Assert size is correct
		if len(results) != len(expectedResults) {
			t.Fatalf("UnmarshalR4Streaming(%s) channel returned unexpected size of result. want: %d got: %d", json, len(expectedResults), len(results))
		}

		for i, result := range results {
			if !cmp.Equal(result, expectedResults[i], protocmp.Transform()) {
				t.Fatalf("UnmarshalR4Streaming(%s) channel returned unexpected result. want: %v got %v", json, expectedResults[i], result)
			}
		}
	})

}

func ExampleUnmarshaller_UnmarshalR4Streaming() {
	json := `{"resourceType":"Patient", "id": "exampleID1"}
	{"resourceType":"Patient", "id": "exampleID2"}`
	u, err := NewUnmarshaller("America/Los_Angeles", fhirversion.R4)
	if err != nil {
		fmt.Println("error")
	}
	jsonReader := bytes.NewReader([]byte(json))
	resourceChan := u.UnmarshalR4Streaming(jsonReader)

	for r := range resourceChan {
		if r.Error != nil {
			fmt.Printf("err: %v", r.Error)
		} else {
			fmt.Printf("%s\n", r.ContainedResource.GetPatient().Id.GetValue())
		}
	}

	// Output:
	// exampleID1
	// exampleID2
}
