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
	"testing"

	"github.com/golang/protobuf/proto"
	"github.com/golang/protobuf/ptypes"
	"github.com/google/fhir/go/jsonformat/internal/jsonpbhelper"
	"github.com/google/go-cmp/cmp"

	d2pb "github.com/google/fhir/go/proto/google/fhir/proto/dstu2/datatypes_go_proto"
	r2pb "github.com/google/fhir/go/proto/google/fhir/proto/dstu2/resources_go_proto"
	c4pb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/codes_go_proto"
	d4pb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/datatypes_go_proto"
	r4pb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/bundle_and_contained_resource_go_proto"
	r4codesystempb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/code_system_go_proto"
	r4devicepb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/device_go_proto"
	r4patientpb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/patient_go_proto"
	r4researchstudypb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/research_study_go_proto"
	r4searchparampb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/search_parameter_go_proto"
	c3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/codes_go_proto"
	d3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/datatypes_go_proto"
	m3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/metadatatypes_go_proto"
	r3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/resources_go_proto"
	anypb "google.golang.org/protobuf/types/known/anypb"
)

// TODO: Find a better way to maintain the versioned unit tests.

type mvr struct {
	ver Version
	r   proto.Message
}

// TODO: merge with other copies of this function
func marshalToAny(t *testing.T, pb proto.Message) *anypb.Any {
	t.Helper()
	any, err := ptypes.MarshalAny(pb)
	if err != nil {
		t.Errorf("failed to marshal %T:%+v to Any", pb, pb)
	}
	return any
}

func TestMarshalContainedResource(t *testing.T) {
	tests := []struct {
		name   string
		pretty bool
		inputs []mvr
		want   []byte
	}{
		{
			name:   "PrimitiveExtension",
			pretty: true,
			inputs: []mvr{
				{
					ver: DSTU2,
					r: &r2pb.ContainedResource{
						OneofResource: &r2pb.ContainedResource_Patient{
							Patient: &r2pb.Patient{
								Active: &d2pb.Boolean{
									Value: true,
								},
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
					ver: STU3,
					r: &r3pb.ContainedResource{
						OneofResource: &r3pb.ContainedResource_Patient{
							Patient: &r3pb.Patient{
								Active: &d3pb.Boolean{
									Value: true,
								},
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
					ver: R4,
					r: &r4pb.ContainedResource{
						OneofResource: &r4pb.ContainedResource_Patient{
							Patient: &r4patientpb.Patient{
								Active: &d4pb.Boolean{
									Value: true,
								},
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
			},
			want: []byte(`{
        "active": true,
        "name": [
          {
            "_given": [
              {
                "extension": [
                  {
                    "url": "http://hl7.org/fhir/StructureDefinition/qualifier",
                    "valueCode": "MID"
                  }
                ],
                "id": "a3"
              }
            ],
            "given": [
              "Toby"
            ]
          }
        ],
        "resourceType": "Patient"
      }`),
		},
		{
			name:   "R3 ResearchStudy",
			pretty: true,
			inputs: []mvr{
				{
					ver: STU3,
					r: &r3pb.ContainedResource{
						OneofResource: &r3pb.ContainedResource_ResearchStudy{
							ResearchStudy: &r3pb.ResearchStudy{
								Id: &d3pb.Id{
									Value: "example",
								},
								Text: &m3pb.Narrative{
									Status: &c3pb.NarrativeStatusCode{
										Value: c3pb.NarrativeStatusCode_GENERATED,
									},
									Div: &d3pb.Xhtml{
										Value: `<div xmlns="http://www.w3.org/1999/xhtml">[Put rendering here]</div>`,
									},
								},
								Status: &c3pb.ResearchStudyStatusCode{
									Value: c3pb.ResearchStudyStatusCode_DRAFT,
								},
							},
						},
					},
				},
			},
			want: []byte(`{
        "id": "example",
        "resourceType": "ResearchStudy",
        "status": "draft",
        "text": {
          "div": "<div xmlns=\"http://www.w3.org/1999/xhtml\">[Put rendering here]</div>",
          "status": "generated"
        }
      }`),
		},
		{
			name:   "R4 ResearchStudy",
			pretty: true,
			inputs: []mvr{
				{
					ver: R4,
					r: &r4pb.ContainedResource{
						OneofResource: &r4pb.ContainedResource_ResearchStudy{
							ResearchStudy: &r4researchstudypb.ResearchStudy{
								Id: &d4pb.Id{
									Value: "example",
								},
								Text: &d4pb.Narrative{
									Status: &d4pb.Narrative_StatusCode{
										Value: c4pb.NarrativeStatusCode_GENERATED,
									},
									Div: &d4pb.Xhtml{
										Value: `<div xmlns="http://www.w3.org/1999/xhtml">[Put rendering here]</div>`,
									},
								},
								Status: &r4researchstudypb.ResearchStudy_StatusCode{
									Value: c4pb.ResearchStudyStatusCode_ACTIVE,
								},
							},
						},
					},
				},
			},
			want: []byte(`{
        "id": "example",
        "resourceType": "ResearchStudy",
        "status": "active",
        "text": {
          "div": "<div xmlns=\"http://www.w3.org/1999/xhtml\">[Put rendering here]</div>",
          "status": "generated"
        }
      }`),
		},
		{
			name:   "ResearchStudy - ugly",
			pretty: false,
			inputs: []mvr{
				{
					ver: STU3,
					r: &r3pb.ContainedResource{
						OneofResource: &r3pb.ContainedResource_ResearchStudy{
							ResearchStudy: &r3pb.ResearchStudy{
								Id: &d3pb.Id{
									Value: "example",
								},
								Text: &m3pb.Narrative{
									Status: &c3pb.NarrativeStatusCode{
										Value: c3pb.NarrativeStatusCode_GENERATED,
									},
									Div: &d3pb.Xhtml{
										Value: `<div xmlns="http://www.w3.org/1999/xhtml">[Put rendering here]</div>`,
									},
								},
								Status: &c3pb.ResearchStudyStatusCode{
									Value: c3pb.ResearchStudyStatusCode_DRAFT,
								},
							},
						},
					},
				},
			},
			want: []byte(`{"id":"example","resourceType":"ResearchStudy","status":"draft","text":{"div":"<div xmlns=\"http://www.w3.org/1999/xhtml\">[Put rendering here]</div>","status":"generated"}}`),
		},
		{
			name: "STU3 acronym field",
			inputs: []mvr{
				{
					ver: STU3,
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
			want: []byte(`{"id":"example","resourceType":"Device","udi":{"carrierHRF":"test"}}`),
		},
		{
			name: "R4 acronym field",
			inputs: []mvr{
				{
					ver: R4,
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
			want: []byte(`{"id":"example","resourceType":"Device","udiCarrier":[{"carrierHRF":"test"}]}`),
		},
		{
			name: "inline resource",
			inputs: []mvr{
				{
					ver: R4,
					r: &r4pb.ContainedResource{
						OneofResource: &r4pb.ContainedResource_Patient{
							Patient: &r4patientpb.Patient{
								Id: &d4pb.Id{Value: "wan"},
								Contained: []*anypb.Any{
									marshalToAny(t, &r4pb.ContainedResource{
										OneofResource: &r4pb.ContainedResource_Patient{
											Patient: &r4patientpb.Patient{
												Id: &d4pb.Id{
													Value: "nat",
												},
												Contained: []*anypb.Any{
													marshalToAny(t, &r4pb.ContainedResource{
														OneofResource: &r4pb.ContainedResource_Patient{
															Patient: &r4patientpb.Patient{
																Id: &d4pb.Id{
																	Value: "double-nat",
																},
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
					ver: STU3,
					r: &r3pb.ContainedResource{
						OneofResource: &r3pb.ContainedResource_Patient{
							Patient: &r3pb.Patient{
								Id: &d3pb.Id{Value: "wan"},
								Contained: []*r3pb.ContainedResource{
									{
										OneofResource: &r3pb.ContainedResource_Patient{
											Patient: &r3pb.Patient{
												Id: &d3pb.Id{Value: "nat"},
												Contained: []*r3pb.ContainedResource{
													{
														OneofResource: &r3pb.ContainedResource_Patient{
															Patient: &r3pb.Patient{
																Id: &d3pb.Id{Value: "double-nat"},
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
			},
			want: []byte(`{"contained":[{"contained":[{"id":"double-nat","resourceType":"Patient"}],"id":"nat","resourceType":"Patient"}],"id":"wan","resourceType":"Patient"}`),
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			for _, i := range test.inputs {
				t.Run(i.ver.String(), func(t *testing.T) {
					marshaller, err := NewMarshaller(test.pretty, "      ", "  ", i.ver)
					if err != nil {
						t.Fatalf("failed to create marshaler; %v", err)
					}
					got, err := marshaller.Marshal(i.r)
					if err != nil {
						t.Fatalf("marshal failed on %v: %v", test.name, err)
					}
					if bytes.Compare(got, test.want) != 0 {
						t.Errorf("marshal %v: got %v, want %v", test.name, string(got), string(test.want))
					}
				})
			}
		})
	}
}

func TestMarshalResource(t *testing.T) {
	tests := []struct {
		name   string
		inputs []mvr
		pretty bool
		want   []byte
	}{
		{
			name: "Patient",
			inputs: []mvr{
				{
					ver: STU3,
					r: &r3pb.Patient{
						Active: &d3pb.Boolean{
							Value: true,
						},
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
				{
					ver: R4,
					r: &r4patientpb.Patient{
						Active: &d4pb.Boolean{
							Value: true,
						},
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
			pretty: true,
			want: []byte(`{
        "active": true,
        "name": [
          {
            "_given": [
              {
                "extension": [
                  {
                    "url": "http://hl7.org/fhir/StructureDefinition/qualifier",
                    "valueCode": "MID"
                  }
                ],
                "id": "a3"
              }
            ],
            "given": [
              "Toby"
            ]
          }
        ],
        "resourceType": "Patient"
      }`),
		},
		{
			name: "ResearchStudy",
			inputs: []mvr{
				{
					ver: STU3,
					r: &r3pb.ResearchStudy{
						Id: &d3pb.Id{
							Value: "example",
						},
						Text: &m3pb.Narrative{
							Status: &c3pb.NarrativeStatusCode{
								Value: c3pb.NarrativeStatusCode_GENERATED,
							},
							Div: &d3pb.Xhtml{
								Value: `<div xmlns="http://www.w3.org/1999/xhtml">[Put rendering here]</div>`,
							},
						},
						Status: &c3pb.ResearchStudyStatusCode{
							Value: c3pb.ResearchStudyStatusCode_DRAFT,
						},
					},
				},
			},
			pretty: false,
			want:   []byte(`{"id":"example","resourceType":"ResearchStudy","status":"draft","text":{"div":"<div xmlns=\"http://www.w3.org/1999/xhtml\">[Put rendering here]</div>","status":"generated"}}`),
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			for _, i := range tt.inputs {
				t.Run(i.ver.String(), func(t *testing.T) {
					marshaller, err := NewMarshaller(tt.pretty, "      ", "  ", i.ver)
					if err != nil {
						t.Fatalf("failed to create marshaler; %v", err)
					}
					got, err := marshaller.MarshalResource(i.r)
					if err != nil {
						t.Fatalf("MarshalResource() got err %v; want nil err", err)
					}
					if bytes.Compare(got, tt.want) != 0 {
						t.Errorf("MarshalResource() got:\n%s\nwant:\n%s", got, tt.want)
					}
				})
			}
		})
	}
}

func TestMarshalMessage(t *testing.T) {
	tests := []struct {
		name   string
		inputs []mvr
		want   jsonpbhelper.IsJSON
	}{
		{
			name: "ResearchStudy pretty",
			inputs: []mvr{
				{
					ver: STU3,
					r: &r3pb.ResearchStudy{
						Id: &d3pb.Id{
							Value: "example",
						},
						Text: &m3pb.Narrative{
							Status: &c3pb.NarrativeStatusCode{
								Value: c3pb.NarrativeStatusCode_GENERATED,
							},
							Div: &d3pb.Xhtml{
								Value: `<div xmlns="http://www.w3.org/1999/xhtml">[Put rendering here]</div>`,
							},
						},
						Status: &c3pb.ResearchStudyStatusCode{
							Value: c3pb.ResearchStudyStatusCode_DRAFT,
						},
					},
				},
			},
			want: jsonpbhelper.JSONObject{
				"id":     jsonpbhelper.JSONString("example"),
				"status": jsonpbhelper.JSONString("draft"),
				"text": jsonpbhelper.JSONObject{
					"div":    jsonpbhelper.JSONString("<div xmlns=\"http://www.w3.org/1999/xhtml\">[Put rendering here]</div>"),
					"status": jsonpbhelper.JSONString("generated"),
				},
			},
		},
		{
			name: "Nested Resources",
			inputs: []mvr{
				{
					ver: STU3,
					r: &r3pb.Bundle{
						Entry: []*r3pb.Bundle_Entry{{
							Resource: &r3pb.ContainedResource{
								OneofResource: &r3pb.ContainedResource_SearchParameter{
									SearchParameter: &r3pb.SearchParameter{
										Id: &d3pb.Id{
											Value: "DomainResource-text",
										},
									},
								},
							}},
							{Resource: &r3pb.ContainedResource{
								OneofResource: &r3pb.ContainedResource_SearchParameter{
									SearchParameter: &r3pb.SearchParameter{
										Id: &d3pb.Id{
											Value: "Resource-content",
										},
									},
								},
							}},
						},
					},
				},
				{
					ver: R4,
					r: &r4pb.Bundle{
						Entry: []*r4pb.Bundle_Entry{{
							Resource: &r4pb.ContainedResource{
								OneofResource: &r4pb.ContainedResource_SearchParameter{
									SearchParameter: &r4searchparampb.SearchParameter{
										Id: &d4pb.Id{
											Value: "DomainResource-text",
										},
									},
								},
							}},
							{Resource: &r4pb.ContainedResource{
								OneofResource: &r4pb.ContainedResource_SearchParameter{
									SearchParameter: &r4searchparampb.SearchParameter{
										Id: &d4pb.Id{
											Value: "Resource-content",
										},
									},
								},
							}},
						},
					},
				},
			},
			want: jsonpbhelper.JSONObject{
				"entry": jsonpbhelper.JSONArray{
					jsonpbhelper.JSONObject{
						"resource": jsonpbhelper.JSONObject{
							"id":           jsonpbhelper.JSONString("DomainResource-text"),
							"resourceType": jsonpbhelper.JSONString("SearchParameter"),
						},
					},
					jsonpbhelper.JSONObject{
						"resource": jsonpbhelper.JSONObject{
							"id":           jsonpbhelper.JSONString("Resource-content"),
							"resourceType": jsonpbhelper.JSONString("SearchParameter"),
						},
					},
				},
			},
		},
		{
			name: "Patient with primitive extension",
			inputs: []mvr{
				{
					ver: STU3,
					r: &r3pb.Patient{
						BirthDate: &d3pb.Date{
							ValueUs:   1463529600000000,
							Precision: d3pb.Date_DAY,
							Id: &d3pb.String{
								Value: "a3",
							},
							Extension: []*d3pb.Extension{{
								Url: &d3pb.Uri{
									Value: "http://hl7.org/fhir/StructureDefinition/patient-birthTime",
								},
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
				{
					ver: R4,
					r: &r4patientpb.Patient{
						BirthDate: &d4pb.Date{
							ValueUs:   1463529600000000,
							Precision: d4pb.Date_DAY,
							Id: &d4pb.String{
								Value: "a3",
							},
							Extension: []*d4pb.Extension{{
								Url: &d4pb.Uri{
									Value: "http://hl7.org/fhir/StructureDefinition/patient-birthTime",
								},
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
			},
			want: jsonpbhelper.JSONObject{
				"_birthDate": jsonpbhelper.JSONObject{
					"extension": jsonpbhelper.JSONArray{
						jsonpbhelper.JSONObject{
							"url":           jsonpbhelper.JSONString("http://hl7.org/fhir/StructureDefinition/patient-birthTime"),
							"valueDateTime": jsonpbhelper.JSONString("2016-05-18T10:28:45Z"),
						},
					},
					"id": jsonpbhelper.JSONString("a3"),
				},
				"birthDate": jsonpbhelper.JSONString("2016-05-18"),
			},
		},
		{
			name: "Patient with extension id",
			inputs: []mvr{
				{
					ver: STU3,
					r: &r3pb.Patient{
						BirthDate: &d3pb.Date{
							ValueUs:   1463529600000000,
							Precision: d3pb.Date_DAY,
							Extension: []*d3pb.Extension{{
								Url: &d3pb.Uri{
									Value: "http://hl7.org/fhir/StructureDefinition/patient-birthTime",
								},
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
				{
					ver: R4,
					r: &r4patientpb.Patient{
						BirthDate: &d4pb.Date{
							ValueUs:   1463529600000000,
							Precision: d4pb.Date_DAY,
							Extension: []*d4pb.Extension{{
								Url: &d4pb.Uri{
									Value: "http://hl7.org/fhir/StructureDefinition/patient-birthTime",
								},
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
			},
			want: jsonpbhelper.JSONObject{
				"_birthDate": jsonpbhelper.JSONObject{
					"extension": jsonpbhelper.JSONArray{
						jsonpbhelper.JSONObject{
							"url":           jsonpbhelper.JSONString("http://hl7.org/fhir/StructureDefinition/patient-birthTime"),
							"valueDateTime": jsonpbhelper.JSONString("2016-05-18T10:28:45Z"),
						},
					},
				},
				"birthDate": jsonpbhelper.JSONString("2016-05-18"),
			},
		},
		{
			name: "Patient with primitive extension no value",
			inputs: []mvr{
				{
					ver: STU3,
					r: &r3pb.Patient{
						BirthDate: &d3pb.Date{
							Extension: []*d3pb.Extension{{
								Url: &d3pb.Uri{
									Value: "http://hl7.org/fhir/StructureDefinition/patient-birthTime",
								},
								Value: &d3pb.Extension_ValueX{
									Choice: &d3pb.Extension_ValueX_DateTime{
										DateTime: &d3pb.DateTime{
											ValueUs:   1463567325000000,
											Timezone:  "Z",
											Precision: d3pb.DateTime_SECOND,
										},
									},
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
				},
				{
					ver: R4,
					r: &r4patientpb.Patient{
						BirthDate: &d4pb.Date{
							Extension: []*d4pb.Extension{{
								Url: &d4pb.Uri{
									Value: "http://hl7.org/fhir/StructureDefinition/patient-birthTime",
								},
								Value: &d4pb.Extension_ValueX{
									Choice: &d4pb.Extension_ValueX_DateTime{
										DateTime: &d4pb.DateTime{
											ValueUs:   1463567325000000,
											Timezone:  "Z",
											Precision: d4pb.DateTime_SECOND,
										},
									},
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
				},
			},
			want: jsonpbhelper.JSONObject{
				"_birthDate": jsonpbhelper.JSONObject{
					"extension": jsonpbhelper.JSONArray{
						jsonpbhelper.JSONObject{
							"url":           jsonpbhelper.JSONString("http://hl7.org/fhir/StructureDefinition/patient-birthTime"),
							"valueDateTime": jsonpbhelper.JSONString("2016-05-18T10:28:45Z"),
						},
					},
				},
			},
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			for _, i := range test.inputs {
				t.Run(i.ver.String(), func(t *testing.T) {
					marshaller, err := NewMarshaller(true, "      ", "  ", i.ver)
					if err != nil {
						t.Fatalf("failed to create marshaler; %v", err)
					}
					got, err := marshaller.marshalMessageToMap(proto.MessageReflect(i.r))
					if err != nil {
						t.Fatalf("marshal failed on %v: %v", test.name, err)
					}
					if !cmp.Equal(got, test.want) {
						t.Errorf("marshal %v: got %v, want %v", test.name, got, test.want)
					}
				})
			}
		})
	}
}

func TestMarshalMessageForAnalytics(t *testing.T) {
	tests := []struct {
		name   string
		inputs []mvr
		depth  int
		want   jsonpbhelper.IsJSON
	}{
		{
			name: "ID Fields Omitted",
			inputs: []mvr{
				{
					ver: STU3,
					r: &r3pb.ResearchStudy{
						Id: &d3pb.Id{
							Value: "keep resource id",
						},
						Text: &m3pb.Narrative{
							Status: &c3pb.NarrativeStatusCode{
								Value: c3pb.NarrativeStatusCode_GENERATED,
							},
							Div: &d3pb.Xhtml{
								Value: `<div xmlns="http://www.w3.org/1999/xhtml">[Put rendering here]</div>`,
							},
							Id: &d3pb.String{
								Value: "omit id for complex type element",
							},
						},
						Status: &c3pb.ResearchStudyStatusCode{
							Value: c3pb.ResearchStudyStatusCode_DRAFT,
							Id: &d3pb.String{
								Value: "omit id for primitive type element",
							},
						},
					},
				},
			},
			depth: 10,
			want: jsonpbhelper.JSONObject{
				"id":     jsonpbhelper.JSONString("keep resource id"),
				"status": jsonpbhelper.JSONString("draft"),
				"text": jsonpbhelper.JSONObject{
					"div":    jsonpbhelper.JSONString("<div xmlns=\"http://www.w3.org/1999/xhtml\">[Put rendering here]</div>"),
					"status": jsonpbhelper.JSONString("generated"),
				},
			},
		},
		{
			name: "Relative reference",
			inputs: []mvr{
				{
					ver: STU3,
					r: &r3pb.Patient{
						ManagingOrganization: &d3pb.Reference{
							Reference: &d3pb.Reference_Uri{
								Uri: &d3pb.String{
									Value: "Organization/1",
								},
							},
						},
					},
				},
				{
					ver: R4,
					r: &r4patientpb.Patient{
						ManagingOrganization: &d4pb.Reference{
							Reference: &d4pb.Reference_Uri{
								Uri: &d4pb.String{
									Value: "Organization/1",
								},
							},
						},
					},
				},
			},
			depth: 10,
			want: jsonpbhelper.JSONObject{
				"managingOrganization": jsonpbhelper.JSONObject{
					"organizationId": jsonpbhelper.JSONString("1"),
				},
			},
		},
		{
			name: "Choice types",
			inputs: []mvr{
				{
					ver: STU3,
					r: &r3pb.Patient{
						Deceased: &r3pb.Patient_Deceased{
							Deceased: &r3pb.Patient_Deceased_Boolean{Boolean: &d3pb.Boolean{Value: false}},
						},
					},
				},
				{
					ver: R4,
					r: &r4patientpb.Patient{
						Deceased: &r4patientpb.Patient_DeceasedX{
							Choice: &r4patientpb.Patient_DeceasedX_Boolean{Boolean: &d4pb.Boolean{Value: false}},
						},
					},
				},
			},
			depth: 10,
			want: jsonpbhelper.JSONObject{
				"deceased": jsonpbhelper.JSONObject{
					"boolean": jsonpbhelper.JSONRawValue(`false`),
				},
			},
		},
		{
			name: "Extensions as URL strings",
			inputs: []mvr{
				{
					ver: STU3,
					r: &r3pb.Patient{
						Extension: []*d3pb.Extension{
							{
								Url: &d3pb.Uri{Value: "http://hl7.org/fhir/us/core/StructureDefinition/us-core-race"},
								Extension: []*d3pb.Extension{
									{
										Url: &d3pb.Uri{Value: "ombCategory"},
										Value: &d3pb.Extension_ValueX{
											Choice: &d3pb.Extension_ValueX_Coding{
												Coding: &d3pb.Coding{
													System: &d3pb.Uri{Value: "urn:oid:2.16.840.1.113883.6.238"},
													Code:   &d3pb.Code{Value: "2106-3"},
												},
											},
										},
									},
									{
										Url: &d3pb.Uri{Value: "text"},
										Value: &d3pb.Extension_ValueX{
											Choice: &d3pb.Extension_ValueX_StringValue{StringValue: &d3pb.String{Value: "White"}},
										},
									},
								},
							},
							{
								Url: &d3pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition/patient-mothersMaidenName"},
								Value: &d3pb.Extension_ValueX{
									Choice: &d3pb.Extension_ValueX_StringValue{StringValue: &d3pb.String{Value: "Rosie"}},
								},
							},
						},
					},
				},
				{
					ver: R4,
					r: &r4patientpb.Patient{
						Extension: []*d4pb.Extension{
							{
								Url: &d4pb.Uri{Value: "http://hl7.org/fhir/us/core/StructureDefinition/us-core-race"},
								Extension: []*d4pb.Extension{
									{
										Url: &d4pb.Uri{Value: "ombCategory"},
										Value: &d4pb.Extension_ValueX{
											Choice: &d4pb.Extension_ValueX_Coding{
												Coding: &d4pb.Coding{
													System: &d4pb.Uri{Value: "urn:oid:2.16.840.1.113883.6.238"},
													Code:   &d4pb.Code{Value: "2106-3"},
												},
											},
										},
									},
									{
										Url: &d4pb.Uri{Value: "text"},
										Value: &d4pb.Extension_ValueX{
											Choice: &d4pb.Extension_ValueX_StringValue{StringValue: &d4pb.String{Value: "White"}},
										},
									},
								},
							},
							{
								Url: &d4pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition/patient-mothersMaidenName"},
								Value: &d4pb.Extension_ValueX{
									Choice: &d4pb.Extension_ValueX_StringValue{StringValue: &d4pb.String{Value: "Rosie"}},
								},
							},
						},
					},
				},
			},
			depth: 10,
			want: jsonpbhelper.JSONObject{
				"extension": jsonpbhelper.JSONArray{
					jsonpbhelper.JSONString("http://hl7.org/fhir/us/core/StructureDefinition/us-core-race"),
					jsonpbhelper.JSONString("http://hl7.org/fhir/StructureDefinition/patient-mothersMaidenName"),
				},
			},
		},
		{
			name: "Primitive extension",
			inputs: []mvr{
				{
					ver: STU3,
					r: &r3pb.Patient{
						BirthDate: &d3pb.Date{
							ValueUs:   1463529600000000,
							Precision: d3pb.Date_DAY,
							Id: &d3pb.String{
								Value: "a3",
							},
							Extension: []*d3pb.Extension{{
								Url: &d3pb.Uri{
									Value: "http://hl7.org/fhir/StructureDefinition/patient-birthTime",
								},
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
				{
					ver: R4,
					r: &r4patientpb.Patient{
						BirthDate: &d4pb.Date{
							ValueUs:   1463529600000000,
							Precision: d4pb.Date_DAY,
							Id: &d4pb.String{
								Value: "a3",
							},
							Extension: []*d4pb.Extension{{
								Url: &d4pb.Uri{
									Value: "http://hl7.org/fhir/StructureDefinition/patient-birthTime",
								},
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
			},
			depth: 10,
			want: jsonpbhelper.JSONObject{
				"birthDate": jsonpbhelper.JSONString("2016-05-18"),
			},
		},
		{
			name: "Primitive extension no value",
			inputs: []mvr{
				{
					ver: STU3,
					r: &r3pb.Patient{
						BirthDate: &d3pb.Date{
							Extension: []*d3pb.Extension{{
								Url: &d3pb.Uri{
									Value: "http://hl7.org/fhir/StructureDefinition/patient-birthTime",
								},
								Value: &d3pb.Extension_ValueX{
									Choice: &d3pb.Extension_ValueX_DateTime{
										DateTime: &d3pb.DateTime{
											ValueUs:   1463567325000000,
											Timezone:  "Z",
											Precision: d3pb.DateTime_SECOND,
										},
									},
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
				},
				{
					ver: R4,
					r: &r4patientpb.Patient{
						BirthDate: &d4pb.Date{
							Extension: []*d4pb.Extension{{
								Url: &d4pb.Uri{
									Value: "http://hl7.org/fhir/StructureDefinition/patient-birthTime",
								},
								Value: &d4pb.Extension_ValueX{
									Choice: &d4pb.Extension_ValueX_DateTime{
										DateTime: &d4pb.DateTime{
											ValueUs:   1463567325000000,
											Timezone:  "Z",
											Precision: d4pb.DateTime_SECOND,
										},
									},
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
				},
			},
			depth: 10,
			want:  jsonpbhelper.JSONObject{},
		},
		{
			name: "max depth less than actual depth",
			inputs: []mvr{
				{
					ver: STU3,
					r: &r3pb.CodeSystem{
						Concept: []*r3pb.CodeSystem_ConceptDefinition{
							{
								Code: &d3pb.Code{
									Value: "code1",
								},
								Concept: []*r3pb.CodeSystem_ConceptDefinition{
									{
										Code: &d3pb.Code{
											Value: "code2",
										},
										Concept: []*r3pb.CodeSystem_ConceptDefinition{
											{
												Code: &d3pb.Code{
													Value: "code3",
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
					ver: R4,
					r: &r4codesystempb.CodeSystem{
						Concept: []*r4codesystempb.CodeSystem_ConceptDefinition{
							{
								Code: &d4pb.Code{
									Value: "code1",
								},
								Concept: []*r4codesystempb.CodeSystem_ConceptDefinition{
									{
										Code: &d4pb.Code{
											Value: "code2",
										},
										Concept: []*r4codesystempb.CodeSystem_ConceptDefinition{
											{
												Code: &d4pb.Code{
													Value: "code3",
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
			depth: 2,
			want: jsonpbhelper.JSONObject{
				"concept": jsonpbhelper.JSONArray{
					jsonpbhelper.JSONObject{
						"concept": jsonpbhelper.JSONArray{
							jsonpbhelper.JSONObject{
								"code": jsonpbhelper.JSONString("code2"),
							},
						},
						"code": jsonpbhelper.JSONString("code1"),
					},
				},
			},
		},
		{
			name: "default depth",
			inputs: []mvr{
				{
					ver: STU3,
					r: &r3pb.CodeSystem{
						Concept: []*r3pb.CodeSystem_ConceptDefinition{
							{
								Code: &d3pb.Code{
									Value: "code1",
								},
								Concept: []*r3pb.CodeSystem_ConceptDefinition{
									{
										Code: &d3pb.Code{
											Value: "code2",
										},
										Concept: []*r3pb.CodeSystem_ConceptDefinition{
											{
												Code: &d3pb.Code{
													Value: "code3",
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
					ver: R4,
					r: &r4codesystempb.CodeSystem{
						Concept: []*r4codesystempb.CodeSystem_ConceptDefinition{
							{
								Code: &d4pb.Code{Value: "code1"},
								Concept: []*r4codesystempb.CodeSystem_ConceptDefinition{
									{
										Code: &d4pb.Code{Value: "code2"},
										Concept: []*r4codesystempb.CodeSystem_ConceptDefinition{
											{Code: &d4pb.Code{Value: "code3"}},
										},
									},
								},
							},
						},
					},
				},
			},
			want: jsonpbhelper.JSONObject{
				"concept": jsonpbhelper.JSONArray{
					jsonpbhelper.JSONObject{
						"concept": jsonpbhelper.JSONArray{
							jsonpbhelper.JSONObject{
								"code": jsonpbhelper.JSONString("code2"),
							},
						},
						"code": jsonpbhelper.JSONString("code1"),
					},
				},
			},
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			for _, i := range test.inputs {
				t.Run(i.ver.String(), func(t *testing.T) {
					marshaller, err := NewAnalyticsMarshaller(test.depth, i.ver)
					if err != nil {
						t.Fatalf("failed to create marshaller %v: %v", test.name, err)
					}
					got, err := marshaller.marshalMessageToMap(proto.MessageReflect(i.r))
					if err != nil {
						t.Fatalf("marshal failed on %v: %v", test.name, err)
					}
					if !cmp.Equal(got, test.want) {
						t.Errorf("marshal %v: got %v, want %v", test.name, got, test.want)
					}
				})
			}
		})
	}
}

func TestMarshalMessageForAnalytics_InferredSchema(t *testing.T) {
	tests := []struct {
		name   string
		inputs []mvr
		depth  int
		want   jsonpbhelper.IsJSON
	}{
		{
			name: "ID Fields Omitted",
			inputs: []mvr{
				{
					ver: STU3,
					r: &r3pb.ResearchStudy{
						Id: &d3pb.Id{
							Value: "keep resource id",
						},
						Text: &m3pb.Narrative{
							Status: &c3pb.NarrativeStatusCode{
								Value: c3pb.NarrativeStatusCode_GENERATED,
							},
							Div: &d3pb.Xhtml{
								Value: `<div xmlns="http://www.w3.org/1999/xhtml">[Put rendering here]</div>`,
							},
							Id: &d3pb.String{
								Value: "omit id for complex type element",
							},
						},
						Status: &c3pb.ResearchStudyStatusCode{
							Value: c3pb.ResearchStudyStatusCode_DRAFT,
							Id: &d3pb.String{
								Value: "omit id for primitive type element",
							},
						},
					},
				},
			},
			depth: 10,
			want: jsonpbhelper.JSONObject{
				"id":     jsonpbhelper.JSONString("keep resource id"),
				"status": jsonpbhelper.JSONString("draft"),
				"text": jsonpbhelper.JSONObject{
					"div":    jsonpbhelper.JSONString("<div xmlns=\"http://www.w3.org/1999/xhtml\">[Put rendering here]</div>"),
					"status": jsonpbhelper.JSONString("generated"),
				},
			},
		},
		{
			name: "Relative reference",
			inputs: []mvr{
				{
					ver: STU3,
					r: &r3pb.Patient{
						ManagingOrganization: &d3pb.Reference{
							Reference: &d3pb.Reference_Uri{
								Uri: &d3pb.String{
									Value: "Organization/1",
								},
							},
						},
					},
				},
				{
					ver: R4,
					r: &r4patientpb.Patient{
						ManagingOrganization: &d4pb.Reference{
							Reference: &d4pb.Reference_Uri{
								Uri: &d4pb.String{
									Value: "Organization/1",
								},
							},
						},
					},
				},
			},
			depth: 10,
			want: jsonpbhelper.JSONObject{
				"managingOrganization": jsonpbhelper.JSONObject{
					"organizationId": jsonpbhelper.JSONString("1"),
				},
			},
		},
		{
			name: "Choice types",
			inputs: []mvr{
				{
					ver: STU3,
					r: &r3pb.Patient{
						Deceased: &r3pb.Patient_Deceased{
							Deceased: &r3pb.Patient_Deceased_Boolean{Boolean: &d3pb.Boolean{Value: false}},
						},
					},
				},
				{
					ver: R4,
					r: &r4patientpb.Patient{
						Deceased: &r4patientpb.Patient_DeceasedX{
							Choice: &r4patientpb.Patient_DeceasedX_Boolean{Boolean: &d4pb.Boolean{Value: false}},
						},
					},
				},
			},
			depth: 10,
			want: jsonpbhelper.JSONObject{
				"deceased": jsonpbhelper.JSONObject{
					"boolean": jsonpbhelper.JSONRawValue(`false`),
				},
			},
		},
		{
			name: "Extensions as first-class fields",
			inputs: []mvr{
				{
					ver: STU3,
					r: &r3pb.Patient{
						Extension: []*d3pb.Extension{
							{
								Url: &d3pb.Uri{Value: "http://hl7.org/fhir/us/core/StructureDefinition/us-core-race"},
								Extension: []*d3pb.Extension{
									{
										Url: &d3pb.Uri{Value: "ombCategory"},
										Value: &d3pb.Extension_ValueX{
											Choice: &d3pb.Extension_ValueX_Coding{
												Coding: &d3pb.Coding{
													System: &d3pb.Uri{Value: "urn:oid:2.16.840.1.113883.6.238"},
													Code:   &d3pb.Code{Value: "2106-3"},
												},
											},
										},
									},
									{
										Url: &d3pb.Uri{Value: "text"},
										Value: &d3pb.Extension_ValueX{
											Choice: &d3pb.Extension_ValueX_StringValue{StringValue: &d3pb.String{Value: "White"}},
										},
									},
								},
							},
							{
								Url: &d3pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition/patient-mothersMaidenName"},
								Value: &d3pb.Extension_ValueX{
									Choice: &d3pb.Extension_ValueX_StringValue{StringValue: &d3pb.String{Value: "Rosie"}},
								},
							},
						},
					},
				},
				{
					ver: R4,
					r: &r4patientpb.Patient{
						Extension: []*d4pb.Extension{
							{
								Url: &d4pb.Uri{Value: "http://hl7.org/fhir/us/core/StructureDefinition/us-core-race"},
								Extension: []*d4pb.Extension{
									{
										Url: &d4pb.Uri{Value: "ombCategory"},
										Value: &d4pb.Extension_ValueX{
											Choice: &d4pb.Extension_ValueX_Coding{
												Coding: &d4pb.Coding{
													System: &d4pb.Uri{Value: "urn:oid:2.16.840.1.113883.6.238"},
													Code:   &d4pb.Code{Value: "2106-3"},
												},
											},
										},
									},
									{
										Url: &d4pb.Uri{Value: "text"},
										Value: &d4pb.Extension_ValueX{
											Choice: &d4pb.Extension_ValueX_StringValue{StringValue: &d4pb.String{Value: "White"}},
										},
									},
								},
							},
							{
								Url: &d4pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition/patient-mothersMaidenName"},
								Value: &d4pb.Extension_ValueX{
									Choice: &d4pb.Extension_ValueX_StringValue{StringValue: &d4pb.String{Value: "Rosie"}},
								},
							},
						},
					},
				},
			},
			depth: 10,
			want: jsonpbhelper.JSONObject{
				"us_core_race": jsonpbhelper.JSONObject{
					"ombCategory": jsonpbhelper.JSONObject{
						"value": jsonpbhelper.JSONObject{
							"coding": jsonpbhelper.JSONObject{
								"system": jsonpbhelper.JSONString("urn:oid:2.16.840.1.113883.6.238"),
								"code":   jsonpbhelper.JSONString("2106-3"),
							},
						},
					},
					"text": jsonpbhelper.JSONObject{
						"value": jsonpbhelper.JSONObject{
							"string": jsonpbhelper.JSONString("White"),
						},
					},
				},
				"patient_mothersMaidenName": jsonpbhelper.JSONObject{
					"value": jsonpbhelper.JSONObject{
						"string": jsonpbhelper.JSONString("Rosie"),
					},
				},
			},
		},
		{
			name: "Primitive extension",
			inputs: []mvr{
				{
					ver: STU3,
					r: &r3pb.Patient{
						BirthDate: &d3pb.Date{
							ValueUs:   1463529600000000,
							Precision: d3pb.Date_DAY,
							Id: &d3pb.String{
								Value: "a3",
							},
							Extension: []*d3pb.Extension{{
								Url: &d3pb.Uri{
									Value: "http://hl7.org/fhir/StructureDefinition/patient-birthTime",
								},
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
				{
					ver: R4,
					r: &r4patientpb.Patient{
						BirthDate: &d4pb.Date{
							ValueUs:   1463529600000000,
							Precision: d4pb.Date_DAY,
							Id: &d4pb.String{
								Value: "a3",
							},
							Extension: []*d4pb.Extension{{
								Url: &d4pb.Uri{
									Value: "http://hl7.org/fhir/StructureDefinition/patient-birthTime",
								},
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
			},
			depth: 10,
			want: jsonpbhelper.JSONObject{
				"_birthDate": jsonpbhelper.JSONObject{
					"patient_birthTime": jsonpbhelper.JSONObject{
						"value": jsonpbhelper.JSONObject{
							"dateTime": jsonpbhelper.JSONString("2016-05-18T10:28:45Z"),
						},
					},
				},
				"birthDate": jsonpbhelper.JSONString("2016-05-18"),
			},
		},
		{
			name: "Primitive extension no value",
			inputs: []mvr{
				{
					ver: STU3,
					r: &r3pb.Patient{
						BirthDate: &d3pb.Date{
							Extension: []*d3pb.Extension{{
								Url: &d3pb.Uri{
									Value: "http://hl7.org/fhir/StructureDefinition/patient-birthTime",
								},
								Value: &d3pb.Extension_ValueX{
									Choice: &d3pb.Extension_ValueX_DateTime{
										DateTime: &d3pb.DateTime{
											ValueUs:   1463567325000000,
											Timezone:  "Z",
											Precision: d3pb.DateTime_SECOND,
										},
									},
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
				},
				{
					ver: R4,
					r: &r4patientpb.Patient{
						BirthDate: &d4pb.Date{
							Extension: []*d4pb.Extension{{
								Url: &d4pb.Uri{
									Value: "http://hl7.org/fhir/StructureDefinition/patient-birthTime",
								},
								Value: &d4pb.Extension_ValueX{
									Choice: &d4pb.Extension_ValueX_DateTime{
										DateTime: &d4pb.DateTime{
											ValueUs:   1463567325000000,
											Timezone:  "Z",
											Precision: d4pb.DateTime_SECOND,
										},
									},
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
				},
			},
			depth: 10,
			want: jsonpbhelper.JSONObject{
				"_birthDate": jsonpbhelper.JSONObject{
					"patient_birthTime": jsonpbhelper.JSONObject{
						"value": jsonpbhelper.JSONObject{
							"dateTime": jsonpbhelper.JSONString("2016-05-18T10:28:45Z"),
						},
					},
				},
			},
		},
		{
			name: "max depth less than actual depth",
			inputs: []mvr{
				{
					ver: STU3,
					r: &r3pb.CodeSystem{
						Concept: []*r3pb.CodeSystem_ConceptDefinition{
							{
								Code: &d3pb.Code{
									Value: "code1",
								},
								Concept: []*r3pb.CodeSystem_ConceptDefinition{
									{
										Code: &d3pb.Code{
											Value: "code2",
										},
										Concept: []*r3pb.CodeSystem_ConceptDefinition{
											{
												Code: &d3pb.Code{
													Value: "code3",
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
					ver: R4,
					r: &r4codesystempb.CodeSystem{
						Concept: []*r4codesystempb.CodeSystem_ConceptDefinition{
							{
								Code: &d4pb.Code{
									Value: "code1",
								},
								Concept: []*r4codesystempb.CodeSystem_ConceptDefinition{
									{
										Code: &d4pb.Code{
											Value: "code2",
										},
										Concept: []*r4codesystempb.CodeSystem_ConceptDefinition{
											{
												Code: &d4pb.Code{
													Value: "code3",
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
			depth: 2,
			want: jsonpbhelper.JSONObject{
				"concept": jsonpbhelper.JSONArray{
					jsonpbhelper.JSONObject{
						"concept": jsonpbhelper.JSONArray{
							jsonpbhelper.JSONObject{
								"code": jsonpbhelper.JSONString("code2"),
							},
						},
						"code": jsonpbhelper.JSONString("code1"),
					},
				},
			},
		},
		{
			name: "default depth",
			inputs: []mvr{
				{
					ver: STU3,
					r: &r3pb.CodeSystem{
						Concept: []*r3pb.CodeSystem_ConceptDefinition{
							{
								Code: &d3pb.Code{
									Value: "code1",
								},
								Concept: []*r3pb.CodeSystem_ConceptDefinition{
									{
										Code: &d3pb.Code{
											Value: "code2",
										},
										Concept: []*r3pb.CodeSystem_ConceptDefinition{
											{
												Code: &d3pb.Code{
													Value: "code3",
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
					ver: R4,
					r: &r4codesystempb.CodeSystem{
						Concept: []*r4codesystempb.CodeSystem_ConceptDefinition{
							{
								Code: &d4pb.Code{Value: "code1"},
								Concept: []*r4codesystempb.CodeSystem_ConceptDefinition{
									{
										Code: &d4pb.Code{Value: "code2"},
										Concept: []*r4codesystempb.CodeSystem_ConceptDefinition{
											{Code: &d4pb.Code{Value: "code3"}},
										},
									},
								},
							},
						},
					},
				},
			},
			want: jsonpbhelper.JSONObject{
				"concept": jsonpbhelper.JSONArray{
					jsonpbhelper.JSONObject{
						"concept": jsonpbhelper.JSONArray{
							jsonpbhelper.JSONObject{
								"code": jsonpbhelper.JSONString("code2"),
							},
						},
						"code": jsonpbhelper.JSONString("code1"),
					},
				},
			},
		},
		{
			name: "Extension last token collides with first-class field",
			inputs: []mvr{
				{
					ver: STU3,
					r: &r3pb.Patient{
						Id: &d3pb.Id{Value: "id1"},
						Extension: []*d3pb.Extension{
							{
								Url: &d3pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition/id"},
								Value: &d3pb.Extension_ValueX{
									Choice: &d3pb.Extension_ValueX_StringValue{StringValue: &d3pb.String{Value: "id2"}},
								},
							},
						},
					},
				},
				{
					ver: R4,
					r: &r4patientpb.Patient{
						Id: &d4pb.Id{Value: "id1"},
						Extension: []*d4pb.Extension{
							{
								Url: &d4pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition/id"},
								Value: &d4pb.Extension_ValueX{
									Choice: &d4pb.Extension_ValueX_StringValue{StringValue: &d4pb.String{Value: "id2"}},
								},
							},
						},
					},
				},
			},
			want: jsonpbhelper.JSONObject{
				"id": jsonpbhelper.JSONString("id1"),
				"hl7_org_fhir_StructureDefinition_id": jsonpbhelper.JSONObject{
					"value": jsonpbhelper.JSONObject{
						"string": jsonpbhelper.JSONString("id2"),
					},
				},
			},
		},
		{
			name: "Extension last token collides with other extensions",
			inputs: []mvr{
				{
					ver: STU3,
					r: &r3pb.Patient{
						Extension: []*d3pb.Extension{
							{
								Url: &d3pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition1/id"},
								Value: &d3pb.Extension_ValueX{
									Choice: &d3pb.Extension_ValueX_StringValue{StringValue: &d3pb.String{Value: "id1"}},
								},
							},
							{
								Url: &d3pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition2/id"},
								Value: &d3pb.Extension_ValueX{
									Choice: &d3pb.Extension_ValueX_StringValue{StringValue: &d3pb.String{Value: "id2"}},
								},
							},
						},
					},
				},
				{
					ver: R4,
					r: &r4patientpb.Patient{
						Extension: []*d4pb.Extension{
							{
								Url: &d4pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition1/id"},
								Value: &d4pb.Extension_ValueX{
									Choice: &d4pb.Extension_ValueX_StringValue{StringValue: &d4pb.String{Value: "id1"}},
								},
							},
							{
								Url: &d4pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition2/id"},
								Value: &d4pb.Extension_ValueX{
									Choice: &d4pb.Extension_ValueX_StringValue{StringValue: &d4pb.String{Value: "id2"}},
								},
							},
						},
					},
				},
			},
			want: jsonpbhelper.JSONObject{
				"hl7_org_fhir_StructureDefinition1_id": jsonpbhelper.JSONObject{
					"value": jsonpbhelper.JSONObject{
						"string": jsonpbhelper.JSONString("id1"),
					},
				},
				"hl7_org_fhir_StructureDefinition2_id": jsonpbhelper.JSONObject{
					"value": jsonpbhelper.JSONObject{
						"string": jsonpbhelper.JSONString("id2"),
					},
				},
			},
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			for _, i := range test.inputs {
				t.Run(i.ver.String(), func(t *testing.T) {
					marshaller, err := NewAnalyticsMarshallerWithInferredSchema(test.depth, i.ver)
					if err != nil {
						t.Fatalf("failed to create marshaller %v: %v", test.name, err)
					}
					got, err := marshaller.marshalMessageToMap(proto.MessageReflect(i.r))
					if err != nil {
						t.Fatalf("marshal failed on %v: %v", test.name, err)
					}
					if !cmp.Equal(got, test.want) {
						t.Errorf("marshal %v: got %v, want %v", test.name, got, test.want)
					}
				})
			}
		})
	}
}

func TestMarshalMessageForAnalytics_InferredSchema_Error(t *testing.T) {
	tests := []struct {
		name   string
		inputs []mvr
	}{
		{
			name: "Extensions as first-class fields",
			inputs: []mvr{
				{
					ver: STU3,
					r: &r3pb.Patient{
						Id: &d3pb.Id{Value: "id1"},
						Extension: []*d3pb.Extension{
							{
								Url: &d3pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition/id"},
								Value: &d3pb.Extension_ValueX{
									Choice: &d3pb.Extension_ValueX_StringValue{StringValue: &d3pb.String{Value: "id2"}},
								},
							},
							{
								Url: &d3pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition/id"},
								Value: &d3pb.Extension_ValueX{
									Choice: &d3pb.Extension_ValueX_StringValue{StringValue: &d3pb.String{Value: "id3"}},
								},
							},
						},
					},
				},
				{
					ver: R4,
					r: &r4patientpb.Patient{
						Id: &d4pb.Id{Value: "id1"},
						Extension: []*d4pb.Extension{
							{
								Url: &d4pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition/id"},
								Value: &d4pb.Extension_ValueX{
									Choice: &d4pb.Extension_ValueX_StringValue{StringValue: &d4pb.String{Value: "id2"}},
								},
							},
							{
								Url: &d4pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition/id"},
								Value: &d4pb.Extension_ValueX{
									Choice: &d4pb.Extension_ValueX_StringValue{StringValue: &d4pb.String{Value: "id3"}},
								},
							},
						},
					},
				},
			},
		},
		{
			name: "Primitive extension",
			inputs: []mvr{
				{
					ver: STU3,
					r: &r3pb.Patient{
						BirthDate: &d3pb.Date{
							ValueUs:   1463529600000000,
							Precision: d3pb.Date_DAY,
							Id: &d3pb.String{
								Value: "a3",
							},
							Extension: []*d3pb.Extension{
								{
									Url: &d3pb.Uri{
										Value: "http://hl7.org/fhir/StructureDefinition/patient-birthTime",
									},
									Value: &d3pb.Extension_ValueX{
										Choice: &d3pb.Extension_ValueX_DateTime{
											DateTime: &d3pb.DateTime{
												ValueUs:   1463567325000000,
												Timezone:  "Z",
												Precision: d3pb.DateTime_SECOND,
											},
										},
									},
								},
								{
									Url: &d3pb.Uri{
										Value: "http://hl7.org/fhir/StructureDefinition/patient-birthTime",
									},
									Value: &d3pb.Extension_ValueX{
										Choice: &d3pb.Extension_ValueX_DateTime{
											DateTime: &d3pb.DateTime{
												ValueUs:   1463567325000012,
												Timezone:  "Z",
												Precision: d3pb.DateTime_SECOND,
											},
										},
									},
								},
							},
						},
					},
				},
				{
					ver: R4,
					r: &r4patientpb.Patient{
						BirthDate: &d4pb.Date{
							ValueUs:   1463529600000000,
							Precision: d4pb.Date_DAY,
							Id: &d4pb.String{
								Value: "a3",
							},
							Extension: []*d4pb.Extension{
								{
									Url: &d4pb.Uri{
										Value: "http://hl7.org/fhir/StructureDefinition/patient-birthTime",
									},
									Value: &d4pb.Extension_ValueX{
										Choice: &d4pb.Extension_ValueX_DateTime{
											DateTime: &d4pb.DateTime{
												ValueUs:   1463567325000000,
												Timezone:  "Z",
												Precision: d4pb.DateTime_SECOND,
											},
										},
									},
								},
								{
									Url: &d4pb.Uri{
										Value: "http://hl7.org/fhir/StructureDefinition/patient-birthTime",
									},
									Value: &d4pb.Extension_ValueX{
										Choice: &d4pb.Extension_ValueX_DateTime{
											DateTime: &d4pb.DateTime{
												ValueUs:   1463567325000012,
												Timezone:  "Z",
												Precision: d4pb.DateTime_SECOND,
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
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			for _, i := range test.inputs {
				t.Run(i.ver.String(), func(t *testing.T) {
					marshaller, err := NewAnalyticsMarshallerWithInferredSchema(10, i.ver)
					if err != nil {
						t.Fatalf("failed to create marshaller %v: %v", test.name, err)
					}
					_, err = marshaller.marshalMessageToMap(proto.MessageReflect(i.r))
					if err == nil {
						t.Errorf("marshalMessageToMap on %v did not return an error", test.name)
					}
				})
			}
		})
	}
}

func TestMarshalPrimitiveType(t *testing.T) {
	tests := []struct {
		name   string
		inputs []mvr
		want   jsonpbhelper.IsJSON
	}{
		{
			name: "Boolean",
			inputs: []mvr{
				{
					ver: STU3,
					r: &d3pb.Boolean{
						Value: true,
					},
				},
				{
					ver: R4,
					r: &d4pb.Boolean{
						Value: true,
					},
				},
			},
			want: jsonpbhelper.JSONRawValue(`true`),
		},
		{
			name: "Integer",
			inputs: []mvr{
				{
					ver: STU3,
					r: &d3pb.Integer{
						Value: 1,
					},
				},
				{
					ver: R4,
					r: &d4pb.Integer{
						Value: 1,
					},
				},
			},
			want: jsonpbhelper.JSONRawValue(`1`),
		},
		{
			name: "Canonical",
			inputs: []mvr{
				{
					ver: R4,
					r: &d4pb.Canonical{
						Value: "c",
					},
				},
			},
			want: jsonpbhelper.JSONString("c"),
		},
		{
			name: "Code",
			inputs: []mvr{
				{
					ver: STU3,
					r: &d3pb.Code{
						Value: "some code",
					},
				},
				{
					ver: R4,
					r: &d4pb.Code{
						Value: "some code",
					},
				},
			},
			want: jsonpbhelper.JSONString("some code"),
		},
		{
			name: "Id",
			inputs: []mvr{
				{
					ver: STU3,
					r: &d3pb.Id{
						Value: "patient1234",
					},
				},
				{
					ver: R4,
					r: &d4pb.Id{
						Value: "patient1234",
					},
				},
			},
			want: jsonpbhelper.JSONString("patient1234"),
		},
		{
			name: "String",
			inputs: []mvr{
				{
					ver: STU3,
					r: &d3pb.String{
						Value: "This is a string",
					},
				},
				{
					ver: R4,
					r: &d4pb.String{
						Value: "This is a string",
					},
				},
			},
			want: jsonpbhelper.JSONString("This is a string"),
		},
		{
			name: "Markdown",
			inputs: []mvr{
				{
					ver: STU3,
					r: &d3pb.Markdown{
						Value: "md",
					},
				},
				{
					ver: R4,
					r: &d4pb.Markdown{
						Value: "md",
					},
				},
			},
			want: jsonpbhelper.JSONString("md"),
		},
		{
			name: "Url",
			inputs: []mvr{
				{
					ver: R4,
					r: &d4pb.Url{
						Value: "u",
					},
				},
			},
			want: jsonpbhelper.JSONString("u"),
		},
		{
			name: "Uuid",
			inputs: []mvr{
				{
					ver: STU3,
					r: &d3pb.Uuid{
						Value: "uuid",
					},
				},
				{
					ver: R4,
					r: &d4pb.Uuid{
						Value: "uuid",
					},
				},
			},
			want: jsonpbhelper.JSONString("uuid"),
		},
		{
			name: "ResearchStudyStatusCode",
			inputs: []mvr{
				{
					ver: STU3,
					r: &c3pb.ResearchStudyStatusCode{
						Value: c3pb.ResearchStudyStatusCode_IN_PROGRESS,
					},
				},
			},
			want: jsonpbhelper.JSONString("in-progress"),
		},
		{
			name: "ResearchStudyStatusCode uninitialized",
			inputs: []mvr{
				{
					ver: STU3,
					r: &c3pb.ResearchStudyStatusCode{
						Value: c3pb.ResearchStudyStatusCode_INVALID_UNINITIALIZED,
					},
				},
			},
			want: nil,
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			for _, i := range test.inputs {
				t.Run(i.ver.String(), func(t *testing.T) {
					marshaller, err := NewPrettyMarshaller(i.ver)
					if err != nil {
						t.Fatalf("failed to create marshaler; %v", err)
					}
					got, err := marshaller.marshalPrimitiveType(proto.MessageReflect(i.r))
					if err != nil {
						t.Fatalf("marshalPrimitiveType(%v): %v", test.name, err)
					}
					if !cmp.Equal(got, test.want) {
						t.Errorf("found diff for marshalPrimitiveType(%v): got %v, want %v",
							test.name, got, test.want)
					}
				})
			}
		})
	}
}
