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
	"errors"
	"fmt"
	"testing"

	"github.com/google/fhir/go/fhirversion"
	"github.com/google/fhir/go/jsonformat/internal/jsonpbhelper"
	"github.com/google/go-cmp/cmp"
	"github.com/google/go-cmp/cmp/cmpopts"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/testing/protocmp"

	anypb "google.golang.org/protobuf/types/known/anypb"
	c4pb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/codes_go_proto"
	d4pb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/datatypes_go_proto"
	r4binarypb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/binary_go_proto"
	r4pb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/bundle_and_contained_resource_go_proto"
	r4codesystempb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/code_system_go_proto"
	r4conditionpb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/condition_go_proto"
	r4devicepb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/device_go_proto"
	r4patientpb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/patient_go_proto"
	r4researchstudypb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/research_study_go_proto"
	r4searchparampb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/search_parameter_go_proto"
	c5pb "github.com/google/fhir/go/proto/google/fhir/proto/r5/core/codes_go_proto"
	d5pb "github.com/google/fhir/go/proto/google/fhir/proto/r5/core/datatypes_go_proto"
	r5binarypb "github.com/google/fhir/go/proto/google/fhir/proto/r5/core/resources/binary_go_proto"
	r5pb "github.com/google/fhir/go/proto/google/fhir/proto/r5/core/resources/bundle_and_contained_resource_go_proto"
	r5codesystempb "github.com/google/fhir/go/proto/google/fhir/proto/r5/core/resources/code_system_go_proto"
	r5conditionpb "github.com/google/fhir/go/proto/google/fhir/proto/r5/core/resources/condition_go_proto"
	r5devicepb "github.com/google/fhir/go/proto/google/fhir/proto/r5/core/resources/device_go_proto"
	r5patientpb "github.com/google/fhir/go/proto/google/fhir/proto/r5/core/resources/patient_go_proto"
	r5researchstudypb "github.com/google/fhir/go/proto/google/fhir/proto/r5/core/resources/research_study_go_proto"
	r5searchparampb "github.com/google/fhir/go/proto/google/fhir/proto/r5/core/resources/search_parameter_go_proto"
	c3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/codes_go_proto"
	d3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/datatypes_go_proto"
	m3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/metadatatypes_go_proto"
	r3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/resources_go_proto"
)

// TODO(b/135148603): Find a better way to maintain the versioned unit tests.

var (
	compareJSON = cmp.Options{
		cmp.FilterValues(func(d1, d2 string) bool {
			return json.Valid([]byte(d1)) && json.Valid([]byte(d2))
		}, cmp.Transformer("ParseJSON", func(d string) any {
			var ret any
			if err := json.Unmarshal([]byte(d), &ret); err != nil {
				// Shouldn't fail based on the valid precondition
				panic(err)
			}
			return ret
		})),
		cmpopts.SortMaps(func(k1, k2 string) bool {
			return k1 < k2
		}),
	}
)

type mvr struct {
	ver fhirversion.Version
	r   proto.Message
}

// TODO(b/141131076): merge with other copies of this function
func marshalToAny(t *testing.T, pb proto.Message) *anypb.Any {
	t.Helper()
	any := &anypb.Any{}
	if err := any.MarshalFrom(pb); err != nil {
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
			name: "Normalized reference",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
					r: &r3pb.ContainedResource{
						OneofResource: &r3pb.ContainedResource_Patient{
							Patient: &r3pb.Patient{
								ManagingOrganization: &d3pb.Reference{
									Reference: &d3pb.Reference_PatientId{
										PatientId: &d3pb.ReferenceId{
											Value: "1",
										},
									},
								},
							},
						},
					},
				},
			},
			want: []byte(`{"managingOrganization":{"reference":"Patient/1"},"resourceType":"Patient"}`),
		},
		{
			name: "Primitive no extension",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
					r: &r3pb.ContainedResource{
						OneofResource: &r3pb.ContainedResource_Patient{
							Patient: &r3pb.Patient{
								Active: &d3pb.Boolean{
									Extension: []*d3pb.Extension{
										{
											Url: &d3pb.Uri{Value: jsonpbhelper.PrimitiveHasNoValueURL},
										},
										{
											Url: &d3pb.Uri{Value: "foo"},
											Value: &d3pb.Extension_ValueX{
												Choice: &d3pb.Extension_ValueX_StringValue{
													StringValue: &d3pb.String{
														Value: "bar",
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
			want: []byte(`{"_active":{"extension":[{"url":"foo","valueString":"bar"}]},"resourceType":"Patient"}`),
		},
		{
			name:   "PrimitiveExtension",
			pretty: true,
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
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
					ver: fhirversion.R4,
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
				{
					ver: fhirversion.R5,
					r: &r5pb.ContainedResource{
						OneofResource: &r5pb.ContainedResource_Patient{
							Patient: &r5patientpb.Patient{
								Active: &d5pb.Boolean{
									Value: true,
								},
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
					ver: fhirversion.STU3,
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
			name:   "R4/R5 ResearchStudy",
			pretty: true,
			inputs: []mvr{
				{
					ver: fhirversion.R4,
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
				{
					ver: fhirversion.R5,
					r: &r5pb.ContainedResource{
						OneofResource: &r5pb.ContainedResource_ResearchStudy{
							ResearchStudy: &r5researchstudypb.ResearchStudy{
								Id: &d5pb.Id{
									Value: "example",
								},
								Text: &d5pb.Narrative{
									Status: &d5pb.Narrative_StatusCode{
										Value: c5pb.NarrativeStatusCode_GENERATED,
									},
									Div: &d5pb.Xhtml{
										Value: `<div xmlns="http://www.w3.org/1999/xhtml">[Put rendering here]</div>`,
									},
								},
								Status: &r5researchstudypb.ResearchStudy_StatusCode{
									Value: c5pb.PublicationStatusCode_ACTIVE,
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
					ver: fhirversion.STU3,
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
			want: []byte(`{"id":"example","resourceType":"Device","udi":{"carrierHRF":"test"}}`),
		},
		{
			name: "R4/R5 acronym field",
			inputs: []mvr{
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
				{
					ver: fhirversion.R5,
					r: &r5pb.ContainedResource{
						OneofResource: &r5pb.ContainedResource_Device{
							Device: &r5devicepb.Device{
								Id: &d5pb.Id{
									Value: "example",
								},
								UdiCarrier: []*r5devicepb.Device_UdiCarrier{
									{CarrierHrf: &d5pb.String{Value: "test"}},
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
					ver: fhirversion.R4,
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
					ver: fhirversion.STU3,
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
				{
					ver: fhirversion.R5,
					r: &r5pb.ContainedResource{
						OneofResource: &r5pb.ContainedResource_Patient{
							Patient: &r5patientpb.Patient{
								Id: &d5pb.Id{Value: "wan"},
								Contained: []*anypb.Any{
									marshalToAny(t, &r5pb.ContainedResource{
										OneofResource: &r5pb.ContainedResource_Patient{
											Patient: &r5patientpb.Patient{
												Id: &d5pb.Id{
													Value: "nat",
												},
												Contained: []*anypb.Any{
													marshalToAny(t, &r5pb.ContainedResource{
														OneofResource: &r5pb.ContainedResource_Patient{
															Patient: &r5patientpb.Patient{
																Id: &d5pb.Id{
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
					got, err := marshalAndValidate(marshaller, i.r)
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

func marshalAndValidate(m *Marshaller, pb proto.Message) ([]byte, error) {
	before := proto.Clone(pb)
	got, err := m.Marshal(pb)
	if err != nil {
		return nil, err
	}
	if diff := cmp.Diff(before, pb, protocmp.Transform()); diff != "" {
		return nil, fmt.Errorf("input resource was changed by marshaller: %s", diff)
	}
	return got, nil
}

func marshalResourceAndValidate(m *Marshaller, pb proto.Message) ([]byte, error) {
	before := proto.Clone(pb)
	got, err := m.MarshalResource(pb)
	if err != nil {
		return got, err
	}
	if diff := cmp.Diff(before, pb, protocmp.Transform()); diff != "" {
		return nil, fmt.Errorf("input resource was changed by marshaller: %s", diff)
	}
	return got, nil
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
					ver: fhirversion.STU3,
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
					ver: fhirversion.R4,
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
				{
					ver: fhirversion.R5,
					r: &r5patientpb.Patient{
						Active: &d5pb.Boolean{
							Value: true,
						},
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
					ver: fhirversion.STU3,
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
					got, err := marshalResourceAndValidate(marshaller, i.r)
					if err != nil {
						t.Fatalf("MarshalResource() got err %v; want nil err", err)
					}
					if diff := cmp.Diff(got, tt.want); diff != "" {
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
		want   map[string]interface{}
	}{
		{
			name: "ResearchStudy pretty",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
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
			want: map[string]interface{}{
				"resourceType": "ResearchStudy",
				"id":           "example",
				"status":       "draft",
				"text": map[string]interface{}{
					"div":    "<div xmlns=\"http://www.w3.org/1999/xhtml\">[Put rendering here]</div>",
					"status": "generated",
				},
			},
		},
		{
			name: "Nested Resources",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
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
					ver: fhirversion.R4,
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
				{
					ver: fhirversion.R5,
					r: &r5pb.Bundle{
						Entry: []*r5pb.Bundle_Entry{{
							Resource: &r5pb.ContainedResource{
								OneofResource: &r5pb.ContainedResource_SearchParameter{
									SearchParameter: &r5searchparampb.SearchParameter{
										Id: &d5pb.Id{
											Value: "DomainResource-text",
										},
									},
								},
							}},
							{Resource: &r5pb.ContainedResource{
								OneofResource: &r5pb.ContainedResource_SearchParameter{
									SearchParameter: &r5searchparampb.SearchParameter{
										Id: &d5pb.Id{
											Value: "Resource-content",
										},
									},
								},
							}},
						},
					},
				},
			},
			want: map[string]interface{}{
				"resourceType": "Bundle",
				"entry": []interface{}{
					map[string]interface{}{
						"resource": map[string]interface{}{
							"id":           "DomainResource-text",
							"resourceType": "SearchParameter",
						},
					},
					map[string]interface{}{
						"resource": map[string]interface{}{
							"id":           "Resource-content",
							"resourceType": "SearchParameter",
						},
					},
				},
			},
		},
		{
			name: "Patient with primitive extension",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
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
					ver: fhirversion.R4,
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
				{
					ver: fhirversion.R5,
					r: &r5patientpb.Patient{
						BirthDate: &d5pb.Date{
							ValueUs:   1463529600000000,
							Precision: d5pb.Date_DAY,
							Id: &d5pb.String{
								Value: "a3",
							},
							Extension: []*d5pb.Extension{{
								Url: &d5pb.Uri{
									Value: "http://hl7.org/fhir/StructureDefinition/patient-birthTime",
								},
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
			},
			want: map[string]interface{}{
				"resourceType": "Patient",
				"_birthDate": map[string]interface{}{
					"extension": []interface{}{
						map[string]interface{}{
							"url":           "http://hl7.org/fhir/StructureDefinition/patient-birthTime",
							"valueDateTime": "2016-05-18T10:28:45Z",
						},
					},
					"id": "a3",
				},
				"birthDate": "2016-05-18",
			},
		},
		{
			name: "Patient with extension id",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
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
					ver: fhirversion.R4,
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
				{
					ver: fhirversion.R5,
					r: &r5patientpb.Patient{
						BirthDate: &d5pb.Date{
							ValueUs:   1463529600000000,
							Precision: d5pb.Date_DAY,
							Extension: []*d5pb.Extension{{
								Url: &d5pb.Uri{
									Value: "http://hl7.org/fhir/StructureDefinition/patient-birthTime",
								},
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
			},
			want: map[string]interface{}{
				"resourceType": "Patient",
				"_birthDate": map[string]interface{}{
					"extension": []interface{}{
						map[string]interface{}{
							"url":           "http://hl7.org/fhir/StructureDefinition/patient-birthTime",
							"valueDateTime": "2016-05-18T10:28:45Z",
						},
					},
				},
				"birthDate": "2016-05-18",
			},
		},
		{
			name: "Patient with primitive extension no value",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
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
									Value: jsonpbhelper.PrimitiveHasNoValueURL,
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
					ver: fhirversion.R4,
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
									Value: jsonpbhelper.PrimitiveHasNoValueURL,
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
				{
					ver: fhirversion.R5,
					r: &r5patientpb.Patient{
						BirthDate: &d5pb.Date{
							Extension: []*d5pb.Extension{{
								Url: &d5pb.Uri{
									Value: "http://hl7.org/fhir/StructureDefinition/patient-birthTime",
								},
								Value: &d5pb.Extension_ValueX{
									Choice: &d5pb.Extension_ValueX_DateTime{
										DateTime: &d5pb.DateTime{
											ValueUs:   1463567325000000,
											Timezone:  "Z",
											Precision: d5pb.DateTime_SECOND,
										},
									},
								},
							}, {
								Url: &d5pb.Uri{
									Value: jsonpbhelper.PrimitiveHasNoValueURL,
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
			want: map[string]interface{}{
				"resourceType": "Patient",
				"_birthDate": map[string]interface{}{
					"extension": []interface{}{
						map[string]interface{}{
							"url":           "http://hl7.org/fhir/StructureDefinition/patient-birthTime",
							"valueDateTime": "2016-05-18T10:28:45Z",
						},
					},
				},
			},
		},
		{
			name: "Binary with stride",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
					r: &r3pb.Binary{
						Content: &d3pb.Base64Binary{
							Value: []byte{1, 2, 3, 4},
							Extension: []*d3pb.Extension{{
								Url: &d3pb.Uri{Value: jsonpbhelper.Base64BinarySeparatorStrideURL},
								Extension: []*d3pb.Extension{
									{
										Url: &d3pb.Uri{Value: "separator"},
										Value: &d3pb.Extension_ValueX{
											Choice: &d3pb.Extension_ValueX_StringValue{
												StringValue: &d3pb.String{Value: "  "},
											},
										},
									},
									{
										Url: &d3pb.Uri{Value: "stride"},
										Value: &d3pb.Extension_ValueX{
											Choice: &d3pb.Extension_ValueX_PositiveInt{
												PositiveInt: &d3pb.PositiveInt{Value: 2},
											},
										},
									},
								},
							}},
						},
					},
				},
			},
			want: map[string]interface{}{
				"resourceType": "Binary",
				"content":      "AQ  ID  BA  ==",
			},
		},
		{
			name: "Binary with stride - R4/R5",
			inputs: []mvr{
				{
					ver: fhirversion.R4,
					r: &r4binarypb.Binary{
						Data: &d4pb.Base64Binary{
							Value: []byte{1, 2, 3, 4},
							Extension: []*d4pb.Extension{{
								Url: &d4pb.Uri{Value: jsonpbhelper.Base64BinarySeparatorStrideURL},
								Extension: []*d4pb.Extension{
									{
										Url: &d4pb.Uri{Value: "separator"},
										Value: &d4pb.Extension_ValueX{
											Choice: &d4pb.Extension_ValueX_StringValue{
												StringValue: &d4pb.String{Value: "  "},
											},
										},
									},
									{
										Url: &d4pb.Uri{Value: "stride"},
										Value: &d4pb.Extension_ValueX{
											Choice: &d4pb.Extension_ValueX_PositiveInt{
												PositiveInt: &d4pb.PositiveInt{Value: 2},
											},
										},
									},
								},
							}},
						},
					},
				},
				{
					ver: fhirversion.R5,
					r: &r5binarypb.Binary{
						Data: &d5pb.Base64Binary{
							Value: []byte{1, 2, 3, 4},
							Extension: []*d5pb.Extension{{
								Url: &d5pb.Uri{Value: jsonpbhelper.Base64BinarySeparatorStrideURL},
								Extension: []*d5pb.Extension{
									{
										Url: &d5pb.Uri{Value: "separator"},
										Value: &d5pb.Extension_ValueX{
											Choice: &d5pb.Extension_ValueX_StringValue{
												StringValue: &d5pb.String{Value: "  "},
											},
										},
									},
									{
										Url: &d5pb.Uri{Value: "stride"},
										Value: &d5pb.Extension_ValueX{
											Choice: &d5pb.Extension_ValueX_PositiveInt{
												PositiveInt: &d5pb.PositiveInt{Value: 2},
											},
										},
									},
								},
							}},
						},
					},
				},
			},
			want: map[string]interface{}{
				"resourceType": "Binary",
				"data":         "AQ  ID  BA  ==",
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
					gotJSON, err := marshalResourceAndValidate(marshaller, i.r)
					if err != nil {
						t.Fatalf("marshal failed on %v: %v", test.name, err)
					}
					got := make(map[string]interface{})
					if err := json.Unmarshal(gotJSON, &got); err != nil {
						t.Fatalf("json.Unmarshal(%q) failed: %v", gotJSON, err)
					}
					if diff := cmp.Diff(got, test.want); diff != "" {
						t.Errorf("marshal %v: diff: %s", test.name, diff)
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
		want   map[string]interface{}
	}{
		{
			name: "ID Fields Omitted",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
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
			want: map[string]interface{}{
				"id":     "keep resource id",
				"status": "draft",
				"text": map[string]interface{}{
					"div":    "<div xmlns=\"http://www.w3.org/1999/xhtml\">[Put rendering here]</div>",
					"status": "generated",
				},
			},
		},
		{
			name: "Relative reference",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
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
					ver: fhirversion.R4,
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
				{
					ver: fhirversion.R5,
					r: &r5patientpb.Patient{
						ManagingOrganization: &d5pb.Reference{
							Reference: &d5pb.Reference_Uri{
								Uri: &d5pb.String{
									Value: "Organization/1",
								},
							},
						},
					},
				},
			},
			depth: 10,
			want: map[string]interface{}{
				"managingOrganization": map[string]interface{}{
					"organizationId": "1",
				},
			},
		},
		{
			name: "Choice types",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
					r: &r3pb.Patient{
						Deceased: &r3pb.Patient_Deceased{
							Deceased: &r3pb.Patient_Deceased_Boolean{Boolean: &d3pb.Boolean{Value: false}},
						},
					},
				},
				{
					ver: fhirversion.R4,
					r: &r4patientpb.Patient{
						Deceased: &r4patientpb.Patient_DeceasedX{
							Choice: &r4patientpb.Patient_DeceasedX_Boolean{Boolean: &d4pb.Boolean{Value: false}},
						},
					},
				},
				{
					ver: fhirversion.R5,
					r: &r5patientpb.Patient{
						Deceased: &r5patientpb.Patient_DeceasedX{
							Choice: &r5patientpb.Patient_DeceasedX_Boolean{Boolean: &d5pb.Boolean{Value: false}},
						},
					},
				},
			},
			depth: 10,
			want: map[string]interface{}{
				"deceased": map[string]interface{}{
					"boolean": false,
				},
			},
		},
		{
			name: "Extensions as URL strings",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
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
					ver: fhirversion.R4,
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
				{
					ver: fhirversion.R5,
					r: &r5patientpb.Patient{
						Extension: []*d5pb.Extension{
							{
								Url: &d5pb.Uri{Value: "http://hl7.org/fhir/us/core/StructureDefinition/us-core-race"},
								Extension: []*d5pb.Extension{
									{
										Url: &d5pb.Uri{Value: "ombCategory"},
										Value: &d5pb.Extension_ValueX{
											Choice: &d5pb.Extension_ValueX_Coding{
												Coding: &d5pb.Coding{
													System: &d5pb.Uri{Value: "urn:oid:2.16.840.1.113883.6.238"},
													Code:   &d5pb.Code{Value: "2106-3"},
												},
											},
										},
									},
									{
										Url: &d5pb.Uri{Value: "text"},
										Value: &d5pb.Extension_ValueX{
											Choice: &d5pb.Extension_ValueX_StringValue{StringValue: &d5pb.String{Value: "White"}},
										},
									},
								},
							},
							{
								Url: &d5pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition/patient-mothersMaidenName"},
								Value: &d5pb.Extension_ValueX{
									Choice: &d5pb.Extension_ValueX_StringValue{StringValue: &d5pb.String{Value: "Rosie"}},
								},
							},
						},
					},
				},
			},
			depth: 10,
			want: map[string]interface{}{
				"extension": []interface{}{
					"http://hl7.org/fhir/us/core/StructureDefinition/us-core-race",
					"http://hl7.org/fhir/StructureDefinition/patient-mothersMaidenName",
				},
			},
		},
		{
			name: "Primitive extension",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
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
					ver: fhirversion.R4,
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
				{
					ver: fhirversion.R5,
					r: &r5patientpb.Patient{
						BirthDate: &d5pb.Date{
							ValueUs:   1463529600000000,
							Precision: d5pb.Date_DAY,
							Id: &d5pb.String{
								Value: "a3",
							},
							Extension: []*d5pb.Extension{{
								Url: &d5pb.Uri{
									Value: "http://hl7.org/fhir/StructureDefinition/patient-birthTime",
								},
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
			},
			depth: 10,
			want: map[string]interface{}{
				"birthDate": "2016-05-18",
			},
		},
		{
			name: "Primitive extension no value",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
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
									Value: jsonpbhelper.PrimitiveHasNoValueURL,
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
					ver: fhirversion.R4,
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
									Value: jsonpbhelper.PrimitiveHasNoValueURL,
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
				{
					ver: fhirversion.R5,
					r: &r5patientpb.Patient{
						BirthDate: &d5pb.Date{
							Extension: []*d5pb.Extension{{
								Url: &d5pb.Uri{
									Value: "http://hl7.org/fhir/StructureDefinition/patient-birthTime",
								},
								Value: &d5pb.Extension_ValueX{
									Choice: &d5pb.Extension_ValueX_DateTime{
										DateTime: &d5pb.DateTime{
											ValueUs:   1463567325000000,
											Timezone:  "Z",
											Precision: d5pb.DateTime_SECOND,
										},
									},
								},
							}, {
								Url: &d5pb.Uri{
									Value: jsonpbhelper.PrimitiveHasNoValueURL,
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
			depth: 10,
			want:  map[string]interface{}{},
		},
		{
			name: "max depth less than actual depth",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
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
					ver: fhirversion.R4,
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
				{
					ver: fhirversion.R5,
					r: &r5codesystempb.CodeSystem{
						Concept: []*r5codesystempb.CodeSystem_ConceptDefinition{
							{
								Code: &d5pb.Code{
									Value: "code1",
								},
								Concept: []*r5codesystempb.CodeSystem_ConceptDefinition{
									{
										Code: &d5pb.Code{
											Value: "code2",
										},
										Concept: []*r5codesystempb.CodeSystem_ConceptDefinition{
											{
												Code: &d5pb.Code{
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
			want: map[string]interface{}{
				"concept": []interface{}{
					map[string]interface{}{
						"concept": []interface{}{
							map[string]interface{}{
								"code": "code2",
							},
						},
						"code": "code1",
					},
				},
			},
		},
		{
			name: "default depth",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
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
					ver: fhirversion.R4,
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
				{
					ver: fhirversion.R5,
					r: &r5codesystempb.CodeSystem{
						Concept: []*r5codesystempb.CodeSystem_ConceptDefinition{
							{
								Code: &d5pb.Code{Value: "code1"},
								Concept: []*r5codesystempb.CodeSystem_ConceptDefinition{
									{
										Code: &d5pb.Code{Value: "code2"},
										Concept: []*r5codesystempb.CodeSystem_ConceptDefinition{
											{Code: &d5pb.Code{Value: "code3"}},
										},
									},
								},
							},
						},
					},
				},
			},
			want: map[string]interface{}{
				"concept": []interface{}{
					map[string]interface{}{
						"concept": []interface{}{
							map[string]interface{}{
								"code": "code2",
							},
						},
						"code": "code1",
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
					gotJSON, err := marshalResourceAndValidate(marshaller, i.r)
					if err != nil {
						t.Fatalf("marshal failed on %v: %v", test.name, err)
					}
					got := make(map[string]interface{})
					if err := json.Unmarshal(gotJSON, &got); err != nil {
						t.Fatalf("json.Unmarshal(%q) failed: %v", gotJSON, err)
					}
					if diff := cmp.Diff(got, test.want); diff != "" {
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
		want   map[string]interface{}
	}{
		{
			name: "ID Fields Omitted",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
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
			want: map[string]interface{}{
				"id":     "keep resource id",
				"status": "draft",
				"text": map[string]interface{}{
					"div":    "<div xmlns=\"http://www.w3.org/1999/xhtml\">[Put rendering here]</div>",
					"status": "generated",
				},
			},
		},
		{
			name: "Relative reference",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
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
					ver: fhirversion.R4,
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
				{
					ver: fhirversion.R5,
					r: &r5patientpb.Patient{
						ManagingOrganization: &d5pb.Reference{
							Reference: &d5pb.Reference_Uri{
								Uri: &d5pb.String{
									Value: "Organization/1",
								},
							},
						},
					},
				},
			},
			depth: 10,
			want: map[string]interface{}{
				"managingOrganization": map[string]interface{}{
					"organizationId": "1",
				},
			},
		},
		{
			name: "Choice types",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
					r: &r3pb.Patient{
						Deceased: &r3pb.Patient_Deceased{
							Deceased: &r3pb.Patient_Deceased_Boolean{Boolean: &d3pb.Boolean{Value: false}},
						},
					},
				},
				{
					ver: fhirversion.R4,
					r: &r4patientpb.Patient{
						Deceased: &r4patientpb.Patient_DeceasedX{
							Choice: &r4patientpb.Patient_DeceasedX_Boolean{Boolean: &d4pb.Boolean{Value: false}},
						},
					},
				},
				{
					ver: fhirversion.R5,
					r: &r5patientpb.Patient{
						Deceased: &r5patientpb.Patient_DeceasedX{
							Choice: &r5patientpb.Patient_DeceasedX_Boolean{Boolean: &d5pb.Boolean{Value: false}},
						},
					},
				},
			},
			depth: 10,
			want: map[string]interface{}{
				"deceased": map[string]interface{}{
					"boolean": false,
				},
			},
		},
		{
			name: "Extensions as first-class fields",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
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
					ver: fhirversion.R4,
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
				{
					ver: fhirversion.R5,
					r: &r5patientpb.Patient{
						Extension: []*d5pb.Extension{
							{
								Url: &d5pb.Uri{Value: "http://hl7.org/fhir/us/core/StructureDefinition/us-core-race"},
								Extension: []*d5pb.Extension{
									{
										Url: &d5pb.Uri{Value: "ombCategory"},
										Value: &d5pb.Extension_ValueX{
											Choice: &d5pb.Extension_ValueX_Coding{
												Coding: &d5pb.Coding{
													System: &d5pb.Uri{Value: "urn:oid:2.16.840.1.113883.6.238"},
													Code:   &d5pb.Code{Value: "2106-3"},
												},
											},
										},
									},
									{
										Url: &d5pb.Uri{Value: "text"},
										Value: &d5pb.Extension_ValueX{
											Choice: &d5pb.Extension_ValueX_StringValue{StringValue: &d5pb.String{Value: "White"}},
										},
									},
								},
							},
							{
								Url: &d5pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition/patient-mothersMaidenName"},
								Value: &d5pb.Extension_ValueX{
									Choice: &d5pb.Extension_ValueX_StringValue{StringValue: &d5pb.String{Value: "Rosie"}},
								},
							},
						},
					},
				},
			},
			depth: 10,
			want: map[string]interface{}{
				"us_core_race": map[string]interface{}{
					"ombCategory": map[string]interface{}{
						"value": map[string]interface{}{
							"coding": map[string]interface{}{
								"system": "urn:oid:2.16.840.1.113883.6.238",
								"code":   "2106-3",
							},
						},
					},
					"text": map[string]interface{}{
						"value": map[string]interface{}{
							"string": "White",
						},
					},
				},
				"patient_mothersMaidenName": map[string]interface{}{
					"value": map[string]interface{}{
						"string": "Rosie",
					},
				},
			},
		},
		{
			name: "Extensions with both value and sub-extension - sub-extension is skipped",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
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
										Extension: []*d3pb.Extension{
											{
												Url: &d3pb.Uri{Value: "text"},
												Value: &d3pb.Extension_ValueX{
													Choice: &d3pb.Extension_ValueX_StringValue{StringValue: &d3pb.String{Value: "White"}},
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
					ver: fhirversion.R4,
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
										Extension: []*d4pb.Extension{
											{
												Url: &d4pb.Uri{Value: "text"},
												Value: &d4pb.Extension_ValueX{
													Choice: &d4pb.Extension_ValueX_StringValue{StringValue: &d4pb.String{Value: "White"}},
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
					r: &r5patientpb.Patient{
						Extension: []*d5pb.Extension{
							{
								Url: &d5pb.Uri{Value: "http://hl7.org/fhir/us/core/StructureDefinition/us-core-race"},
								Extension: []*d5pb.Extension{
									{
										Url: &d5pb.Uri{Value: "ombCategory"},
										Value: &d5pb.Extension_ValueX{
											Choice: &d5pb.Extension_ValueX_Coding{
												Coding: &d5pb.Coding{
													System: &d5pb.Uri{Value: "urn:oid:2.16.840.1.113883.6.238"},
													Code:   &d5pb.Code{Value: "2106-3"},
												},
											},
										},
										Extension: []*d5pb.Extension{
											{
												Url: &d5pb.Uri{Value: "text"},
												Value: &d5pb.Extension_ValueX{
													Choice: &d5pb.Extension_ValueX_StringValue{StringValue: &d5pb.String{Value: "White"}},
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
			depth: 10,
			want: map[string]interface{}{
				"us_core_race": map[string]interface{}{
					"ombCategory": map[string]interface{}{
						"value": map[string]interface{}{
							"coding": map[string]interface{}{
								"system": "urn:oid:2.16.840.1.113883.6.238",
								"code":   "2106-3",
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
					ver: fhirversion.STU3,
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
					ver: fhirversion.R4,
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
				{
					ver: fhirversion.R5,
					r: &r5patientpb.Patient{
						BirthDate: &d5pb.Date{
							ValueUs:   1463529600000000,
							Precision: d5pb.Date_DAY,
							Id: &d5pb.String{
								Value: "a3",
							},
							Extension: []*d5pb.Extension{{
								Url: &d5pb.Uri{
									Value: "http://hl7.org/fhir/StructureDefinition/patient-birthTime",
								},
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
			},
			depth: 10,
			want: map[string]interface{}{
				"_birthDate": map[string]interface{}{
					"patient_birthTime": map[string]interface{}{
						"value": map[string]interface{}{
							"dateTime": "2016-05-18T10:28:45Z",
						},
					},
				},
				"birthDate": "2016-05-18",
			},
		},
		{
			name: "Primitive extension no value",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
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
									Value: jsonpbhelper.PrimitiveHasNoValueURL,
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
					ver: fhirversion.R4,
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
									Value: jsonpbhelper.PrimitiveHasNoValueURL,
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
				{
					ver: fhirversion.R5,
					r: &r5patientpb.Patient{
						BirthDate: &d5pb.Date{
							Extension: []*d5pb.Extension{{
								Url: &d5pb.Uri{
									Value: "http://hl7.org/fhir/StructureDefinition/patient-birthTime",
								},
								Value: &d5pb.Extension_ValueX{
									Choice: &d5pb.Extension_ValueX_DateTime{
										DateTime: &d5pb.DateTime{
											ValueUs:   1463567325000000,
											Timezone:  "Z",
											Precision: d5pb.DateTime_SECOND,
										},
									},
								},
							}, {
								Url: &d5pb.Uri{
									Value: jsonpbhelper.PrimitiveHasNoValueURL,
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
			depth: 10,
			want: map[string]interface{}{
				"_birthDate": map[string]interface{}{
					"patient_birthTime": map[string]interface{}{
						"value": map[string]interface{}{
							"dateTime": "2016-05-18T10:28:45Z",
						},
					},
				},
			},
		},
		{
			name: "max depth less than actual depth",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
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
					ver: fhirversion.R4,
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
				{
					ver: fhirversion.R5,
					r: &r5codesystempb.CodeSystem{
						Concept: []*r5codesystempb.CodeSystem_ConceptDefinition{
							{
								Code: &d5pb.Code{
									Value: "code1",
								},
								Concept: []*r5codesystempb.CodeSystem_ConceptDefinition{
									{
										Code: &d5pb.Code{
											Value: "code2",
										},
										Concept: []*r5codesystempb.CodeSystem_ConceptDefinition{
											{
												Code: &d5pb.Code{
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
			want: map[string]interface{}{
				"concept": []interface{}{
					map[string]interface{}{
						"concept": []interface{}{
							map[string]interface{}{
								"code": "code2",
							},
						},
						"code": "code1",
					},
				},
			},
		},
		{
			name: "default depth",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
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
					ver: fhirversion.R4,
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
				{
					ver: fhirversion.R5,
					r: &r5codesystempb.CodeSystem{
						Concept: []*r5codesystempb.CodeSystem_ConceptDefinition{
							{
								Code: &d5pb.Code{Value: "code1"},
								Concept: []*r5codesystempb.CodeSystem_ConceptDefinition{
									{
										Code: &d5pb.Code{Value: "code2"},
										Concept: []*r5codesystempb.CodeSystem_ConceptDefinition{
											{Code: &d5pb.Code{Value: "code3"}},
										},
									},
								},
							},
						},
					},
				},
			},
			want: map[string]interface{}{
				"concept": []interface{}{
					map[string]interface{}{
						"concept": []interface{}{
							map[string]interface{}{
								"code": "code2",
							},
						},
						"code": "code1",
					},
				},
			},
		},
		{
			name: "Extension last token collides with first-class field",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
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
					ver: fhirversion.R4,
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
				{
					ver: fhirversion.R5,
					r: &r5patientpb.Patient{
						Id: &d5pb.Id{Value: "id1"},
						Extension: []*d5pb.Extension{
							{
								Url: &d5pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition/id"},
								Value: &d5pb.Extension_ValueX{
									Choice: &d5pb.Extension_ValueX_StringValue{StringValue: &d5pb.String{Value: "id2"}},
								},
							},
						},
					},
				},
			},
			want: map[string]interface{}{
				"id": "id1",
				"hl7_org_fhir_StructureDefinition_id": map[string]interface{}{
					"value": map[string]interface{}{
						"string": "id2",
					},
				},
			},
		},
		{
			name: "Extension last token collides with other extensions",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
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
					ver: fhirversion.R4,
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
				{
					ver: fhirversion.R5,
					r: &r5patientpb.Patient{
						Extension: []*d5pb.Extension{
							{
								Url: &d5pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition1/id"},
								Value: &d5pb.Extension_ValueX{
									Choice: &d5pb.Extension_ValueX_StringValue{StringValue: &d5pb.String{Value: "id1"}},
								},
							},
							{
								Url: &d5pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition2/id"},
								Value: &d5pb.Extension_ValueX{
									Choice: &d5pb.Extension_ValueX_StringValue{StringValue: &d5pb.String{Value: "id2"}},
								},
							},
						},
					},
				},
			},
			want: map[string]interface{}{
				"hl7_org_fhir_StructureDefinition1_id": map[string]interface{}{
					"value": map[string]interface{}{
						"string": "id1",
					},
				},
				"hl7_org_fhir_StructureDefinition2_id": map[string]interface{}{
					"value": map[string]interface{}{
						"string": "id2",
					},
				},
			},
		},
		{
			name: "Extension collides with other extension with different case",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
					r: &r3pb.Patient{
						Extension: []*d3pb.Extension{
							{
								Url: &d3pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition1/id"},
								Value: &d3pb.Extension_ValueX{
									Choice: &d3pb.Extension_ValueX_StringValue{StringValue: &d3pb.String{Value: "id1"}},
								},
							},
							{
								Url: &d3pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition2/ID"},
								Value: &d3pb.Extension_ValueX{
									Choice: &d3pb.Extension_ValueX_StringValue{StringValue: &d3pb.String{Value: "id2"}},
								},
							},
						},
					},
				},
				{
					ver: fhirversion.R4,
					r: &r4patientpb.Patient{
						Extension: []*d4pb.Extension{
							{
								Url: &d4pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition1/id"},
								Value: &d4pb.Extension_ValueX{
									Choice: &d4pb.Extension_ValueX_StringValue{StringValue: &d4pb.String{Value: "id1"}},
								},
							},
							{
								Url: &d4pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition2/ID"},
								Value: &d4pb.Extension_ValueX{
									Choice: &d4pb.Extension_ValueX_StringValue{StringValue: &d4pb.String{Value: "id2"}},
								},
							},
						},
					},
				},
				{
					ver: fhirversion.R5,
					r: &r5patientpb.Patient{
						Extension: []*d5pb.Extension{
							{
								Url: &d5pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition1/id"},
								Value: &d5pb.Extension_ValueX{
									Choice: &d5pb.Extension_ValueX_StringValue{StringValue: &d5pb.String{Value: "id1"}},
								},
							},
							{
								Url: &d5pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition2/ID"},
								Value: &d5pb.Extension_ValueX{
									Choice: &d5pb.Extension_ValueX_StringValue{StringValue: &d5pb.String{Value: "id2"}},
								},
							},
						},
					},
				},
			},
			want: map[string]interface{}{
				"hl7_org_fhir_StructureDefinition1_id": map[string]interface{}{
					"value": map[string]interface{}{
						"string": "id1",
					},
				},
				"hl7_org_fhir_StructureDefinition2_ID": map[string]interface{}{
					"value": map[string]interface{}{
						"string": "id2",
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
					gotJSON, err := marshalResourceAndValidate(marshaller, i.r)
					if err != nil {
						t.Fatalf("marshal failed on %v: %v", test.name, err)
					}
					got := make(map[string]interface{})
					if err := json.Unmarshal(gotJSON, &got); err != nil {
						t.Fatalf("json.Unmarshal(%q) failed: %v", gotJSON, err)
					}
					if diff := cmp.Diff(got, test.want); diff != "" {
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
			name: "Repetitive extension",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
					r: &r3pb.Patient{
						Extension: []*d3pb.Extension{
							{
								Url: &d3pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition/extension-field"},
								Value: &d3pb.Extension_ValueX{
									Choice: &d3pb.Extension_ValueX_StringValue{StringValue: &d3pb.String{Value: "id1"}},
								},
							},
							{
								Url: &d3pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition/extension-field"},
								Value: &d3pb.Extension_ValueX{
									Choice: &d3pb.Extension_ValueX_StringValue{StringValue: &d3pb.String{Value: "id2"}},
								},
							},
						},
					},
				},
				{
					ver: fhirversion.R4,
					r: &r4patientpb.Patient{
						Extension: []*d4pb.Extension{
							{
								Url: &d4pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition/extension-field"},
								Value: &d4pb.Extension_ValueX{
									Choice: &d4pb.Extension_ValueX_StringValue{StringValue: &d4pb.String{Value: "id1"}},
								},
							},
							{
								Url: &d4pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition/extension-field"},
								Value: &d4pb.Extension_ValueX{
									Choice: &d4pb.Extension_ValueX_StringValue{StringValue: &d4pb.String{Value: "id2"}},
								},
							},
						},
					},
				},
				{
					ver: fhirversion.R5,
					r: &r5patientpb.Patient{
						Extension: []*d5pb.Extension{
							{
								Url: &d5pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition/extension-field"},
								Value: &d5pb.Extension_ValueX{
									Choice: &d5pb.Extension_ValueX_StringValue{StringValue: &d5pb.String{Value: "id1"}},
								},
							},
							{
								Url: &d5pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition/extension-field"},
								Value: &d5pb.Extension_ValueX{
									Choice: &d5pb.Extension_ValueX_StringValue{StringValue: &d5pb.String{Value: "id2"}},
								},
							},
						},
					},
				},
			},
		},
		{
			name: "Extension with empty url",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
					r: &r3pb.Patient{
						Extension: []*d3pb.Extension{
							{
								Url: &d3pb.Uri{Value: ""},
								Value: &d3pb.Extension_ValueX{
									Choice: &d3pb.Extension_ValueX_StringValue{StringValue: &d3pb.String{Value: "id1"}},
								},
							},
						},
					},
				},
				{
					ver: fhirversion.R4,
					r: &r4patientpb.Patient{
						Extension: []*d4pb.Extension{
							{
								Url: &d4pb.Uri{Value: ""},
								Value: &d4pb.Extension_ValueX{
									Choice: &d4pb.Extension_ValueX_StringValue{StringValue: &d4pb.String{Value: "id1"}},
								},
							},
						},
					},
				},
				{
					ver: fhirversion.R5,
					r: &r5patientpb.Patient{
						Extension: []*d5pb.Extension{
							{
								Url: &d5pb.Uri{Value: ""},
								Value: &d5pb.Extension_ValueX{
									Choice: &d5pb.Extension_ValueX_StringValue{StringValue: &d5pb.String{Value: "id1"}},
								},
							},
						},
					},
				},
			},
		},
		{
			name: "Extension with no url",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
					r: &r3pb.Patient{
						Extension: []*d3pb.Extension{
							{
								Value: &d3pb.Extension_ValueX{
									Choice: &d3pb.Extension_ValueX_StringValue{StringValue: &d3pb.String{Value: "id1"}},
								},
							},
						},
					},
				},
				{
					ver: fhirversion.R4,
					r: &r4patientpb.Patient{
						Extension: []*d4pb.Extension{
							{
								Value: &d4pb.Extension_ValueX{
									Choice: &d4pb.Extension_ValueX_StringValue{StringValue: &d4pb.String{Value: "id1"}},
								},
							},
						},
					},
				},
				{
					ver: fhirversion.R5,
					r: &r5patientpb.Patient{
						Extension: []*d5pb.Extension{
							{
								Value: &d5pb.Extension_ValueX{
									Choice: &d5pb.Extension_ValueX_StringValue{StringValue: &d5pb.String{Value: "id1"}},
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
					_, err = marshaller.marshalMessageToMap(i.r.ProtoReflect())
					if err == nil {
						t.Errorf("marshalMessageToMap on %v did not return an error", test.name)
					}
					var e *ExtensionError
					if !errors.As(err, &e) {
						t.Errorf("marshalMessageToMap on %v expect ResourceError, got %T ", test.name, err)
					}
				})
			}
		})
	}
}

func TestMarshalMessageForAnalyticsV2_InferredSchema(t *testing.T) {
	tests := []struct {
		name   string
		inputs []mvr
		want   map[string]interface{}
	}{
		{
			name: "Repetitive extension",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
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
													Code:   &d3pb.Code{Value: "2076-8"},
												},
											},
										},
									},
									{
										Url: &d3pb.Uri{Value: "text"},
										Value: &d3pb.Extension_ValueX{
											Choice: &d3pb.Extension_ValueX_StringValue{StringValue: &d3pb.String{Value: "Native Hawaiian or Other Pacific Islander"}},
										},
									},
									{
										Url: &d3pb.Uri{Value: "ombCategory"},
										Value: &d3pb.Extension_ValueX{
											Choice: &d3pb.Extension_ValueX_Coding{
												Coding: &d3pb.Coding{
													System: &d3pb.Uri{Value: "urn:oid:2.16.840.1.113883.6.238"},
													Code:   &d3pb.Code{Value: "2028-9"},
												},
											},
										},
									},
									{
										Url: &d3pb.Uri{Value: "text"},
										Value: &d3pb.Extension_ValueX{
											Choice: &d3pb.Extension_ValueX_StringValue{StringValue: &d3pb.String{Value: "Asian"}},
										},
									},
								},
							},
						},
					},
				},
				{
					ver: fhirversion.R4,
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
													Code:   &d4pb.Code{Value: "2076-8"},
												},
											},
										},
									},
									{
										Url: &d4pb.Uri{Value: "text"},
										Value: &d4pb.Extension_ValueX{
											Choice: &d4pb.Extension_ValueX_StringValue{StringValue: &d4pb.String{Value: "Native Hawaiian or Other Pacific Islander"}},
										},
									},
									{
										Url: &d4pb.Uri{Value: "ombCategory"},
										Value: &d4pb.Extension_ValueX{
											Choice: &d4pb.Extension_ValueX_Coding{
												Coding: &d4pb.Coding{
													System: &d4pb.Uri{Value: "urn:oid:2.16.840.1.113883.6.238"},
													Code:   &d4pb.Code{Value: "2028-9"},
												},
											},
										},
									},
									{
										Url: &d4pb.Uri{Value: "text"},
										Value: &d4pb.Extension_ValueX{
											Choice: &d4pb.Extension_ValueX_StringValue{StringValue: &d4pb.String{Value: "Asian"}},
										},
									},
								},
							},
						},
					},
				},
				{
					ver: fhirversion.R5,
					r: &r5patientpb.Patient{
						Extension: []*d5pb.Extension{
							{
								Url: &d5pb.Uri{Value: "http://hl7.org/fhir/us/core/StructureDefinition/us-core-race"},
								Extension: []*d5pb.Extension{
									{
										Url: &d5pb.Uri{Value: "ombCategory"},
										Value: &d5pb.Extension_ValueX{
											Choice: &d5pb.Extension_ValueX_Coding{
												Coding: &d5pb.Coding{
													System: &d5pb.Uri{Value: "urn:oid:2.16.840.1.113883.6.238"},
													Code:   &d5pb.Code{Value: "2076-8"},
												},
											},
										},
									},
									{
										Url: &d5pb.Uri{Value: "text"},
										Value: &d5pb.Extension_ValueX{
											Choice: &d5pb.Extension_ValueX_StringValue{StringValue: &d5pb.String{Value: "Native Hawaiian or Other Pacific Islander"}},
										},
									},
									{
										Url: &d5pb.Uri{Value: "ombCategory"},
										Value: &d5pb.Extension_ValueX{
											Choice: &d5pb.Extension_ValueX_Coding{
												Coding: &d5pb.Coding{
													System: &d5pb.Uri{Value: "urn:oid:2.16.840.1.113883.6.238"},
													Code:   &d5pb.Code{Value: "2028-9"},
												},
											},
										},
									},
									{
										Url: &d5pb.Uri{Value: "text"},
										Value: &d5pb.Extension_ValueX{
											Choice: &d5pb.Extension_ValueX_StringValue{StringValue: &d5pb.String{Value: "Asian"}},
										},
									},
								},
							},
						},
					},
				},
			},
			want: map[string]interface{}{
				"us_core_race": []interface{}{
					map[string]interface{}{
						"ombCategory": []interface{}{
							map[string]interface{}{
								"value": map[string]interface{}{
									"coding": map[string]interface{}{
										"system": "urn:oid:2.16.840.1.113883.6.238",
										"code":   "2076-8",
									},
								},
							},
							map[string]interface{}{
								"value": map[string]interface{}{
									"coding": map[string]interface{}{
										"system": "urn:oid:2.16.840.1.113883.6.238",
										"code":   "2028-9",
									},
								},
							},
						},
						"text": []interface{}{map[string]interface{}{
							"value": map[string]interface{}{
								"string": "Native Hawaiian or Other Pacific Islander",
							},
						},
							map[string]interface{}{
								"value": map[string]interface{}{
									"string": "Asian",
								},
							},
						},
					},
				},
			},
		},
		{
			name: "Repetitive extension - mixed levels",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
					r: &r3pb.Patient{
						Extension: []*d3pb.Extension{
							{
								Url: &d3pb.Uri{Value: "http://hl7.org/StructureDefinition/test"},
								Value: &d3pb.Extension_ValueX{
									Choice: &d3pb.Extension_ValueX_StringValue{
										StringValue: &d3pb.String{Value: "a"},
									},
								},
							},
							{
								Url: &d3pb.Uri{Value: "http://hl7.org/StructureDefinition/test"},
								Value: &d3pb.Extension_ValueX{
									Choice: &d3pb.Extension_ValueX_StringValue{
										StringValue: &d3pb.String{Value: "b"},
									},
								},
							},
							{
								Url: &d3pb.Uri{Value: "http://hl7.org/StructureDefinition/test"},
								Extension: []*d3pb.Extension{
									{
										Url: &d3pb.Uri{Value: "test"},
										Value: &d3pb.Extension_ValueX{
											Choice: &d3pb.Extension_ValueX_StringValue{
												StringValue: &d3pb.String{Value: "c"},
											},
										},
									},
									{
										Url: &d3pb.Uri{Value: "test"},
										Value: &d3pb.Extension_ValueX{
											Choice: &d3pb.Extension_ValueX_StringValue{
												StringValue: &d3pb.String{Value: "d"},
											},
										},
									},
									{
										Url: &d3pb.Uri{Value: "test"},
										Extension: []*d3pb.Extension{
											{
												Url: &d3pb.Uri{Value: "test"},
												Value: &d3pb.Extension_ValueX{
													Choice: &d3pb.Extension_ValueX_StringValue{
														StringValue: &d3pb.String{Value: "e"},
													},
												},
											},
											{
												Url: &d3pb.Uri{Value: "test"},
												Value: &d3pb.Extension_ValueX{
													Choice: &d3pb.Extension_ValueX_StringValue{
														StringValue: &d3pb.String{Value: "f"},
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
					ver: fhirversion.R4,
					r: &r4patientpb.Patient{
						Extension: []*d4pb.Extension{
							{
								Url: &d4pb.Uri{Value: "http://hl7.org/StructureDefinition/test"},
								Value: &d4pb.Extension_ValueX{
									Choice: &d4pb.Extension_ValueX_StringValue{
										StringValue: &d4pb.String{Value: "a"},
									},
								},
							},
							{
								Url: &d4pb.Uri{Value: "http://hl7.org/StructureDefinition/test"},
								Value: &d4pb.Extension_ValueX{
									Choice: &d4pb.Extension_ValueX_StringValue{
										StringValue: &d4pb.String{Value: "b"},
									},
								},
							},
							{
								Url: &d4pb.Uri{Value: "http://hl7.org/StructureDefinition/test"},
								Extension: []*d4pb.Extension{
									{
										Url: &d4pb.Uri{Value: "test"},
										Value: &d4pb.Extension_ValueX{
											Choice: &d4pb.Extension_ValueX_StringValue{
												StringValue: &d4pb.String{Value: "c"},
											},
										},
									},
									{
										Url: &d4pb.Uri{Value: "test"},
										Value: &d4pb.Extension_ValueX{
											Choice: &d4pb.Extension_ValueX_StringValue{
												StringValue: &d4pb.String{Value: "d"},
											},
										},
									},
									{
										Url: &d4pb.Uri{Value: "test"},
										Extension: []*d4pb.Extension{
											{
												Url: &d4pb.Uri{Value: "test"},
												Value: &d4pb.Extension_ValueX{
													Choice: &d4pb.Extension_ValueX_StringValue{
														StringValue: &d4pb.String{Value: "e"},
													},
												},
											},
											{
												Url: &d4pb.Uri{Value: "test"},
												Value: &d4pb.Extension_ValueX{
													Choice: &d4pb.Extension_ValueX_StringValue{
														StringValue: &d4pb.String{Value: "f"},
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
					r: &r5patientpb.Patient{
						Extension: []*d5pb.Extension{
							{
								Url: &d5pb.Uri{Value: "http://hl7.org/StructureDefinition/test"},
								Value: &d5pb.Extension_ValueX{
									Choice: &d5pb.Extension_ValueX_StringValue{
										StringValue: &d5pb.String{Value: "a"},
									},
								},
							},
							{
								Url: &d5pb.Uri{Value: "http://hl7.org/StructureDefinition/test"},
								Value: &d5pb.Extension_ValueX{
									Choice: &d5pb.Extension_ValueX_StringValue{
										StringValue: &d5pb.String{Value: "b"},
									},
								},
							},
							{
								Url: &d5pb.Uri{Value: "http://hl7.org/StructureDefinition/test"},
								Extension: []*d5pb.Extension{
									{
										Url: &d5pb.Uri{Value: "test"},
										Value: &d5pb.Extension_ValueX{
											Choice: &d5pb.Extension_ValueX_StringValue{
												StringValue: &d5pb.String{Value: "c"},
											},
										},
									},
									{
										Url: &d5pb.Uri{Value: "test"},
										Value: &d5pb.Extension_ValueX{
											Choice: &d5pb.Extension_ValueX_StringValue{
												StringValue: &d5pb.String{Value: "d"},
											},
										},
									},
									{
										Url: &d5pb.Uri{Value: "test"},
										Extension: []*d5pb.Extension{
											{
												Url: &d5pb.Uri{Value: "test"},
												Value: &d5pb.Extension_ValueX{
													Choice: &d5pb.Extension_ValueX_StringValue{
														StringValue: &d5pb.String{Value: "e"},
													},
												},
											},
											{
												Url: &d5pb.Uri{Value: "test"},
												Value: &d5pb.Extension_ValueX{
													Choice: &d5pb.Extension_ValueX_StringValue{
														StringValue: &d5pb.String{Value: "f"},
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
			want: map[string]interface{}{
				"test": []interface{}{
					map[string]interface{}{
						"value": map[string]interface{}{
							"string": "a",
						},
					},
					map[string]interface{}{
						"value": map[string]interface{}{
							"string": "b",
						},
					},
					map[string]interface{}{
						"test": []interface{}{
							map[string]interface{}{
								"value": map[string]interface{}{
									"string": "c",
								},
							},
							map[string]interface{}{
								"value": map[string]interface{}{
									"string": "d",
								},
							},
							map[string]interface{}{
								"test": []interface{}{
									map[string]interface{}{
										"value": map[string]interface{}{
											"string": "e",
										},
									},
									map[string]interface{}{
										"value": map[string]interface{}{
											"string": "f",
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
			name: "Nested repetitive extension",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
					r: &r3pb.Patient{
						Extension: []*d3pb.Extension{
							{
								Url: &d3pb.Uri{Value: "http://example.com/Extension/QueryData"},
								Extension: []*d3pb.Extension{
									{
										Url: &d3pb.Uri{Value: "https://example.com/Extension/QueryMnemonic"},
										Value: &d3pb.Extension_ValueX{
											Choice: &d3pb.Extension_ValueX_StringValue{
												StringValue: &d3pb.String{Value: "a"},
											},
										},
									},
									{
										Url: &d3pb.Uri{Value: "https://example.com/Extension/QueryQuestion"},
										Value: &d3pb.Extension_ValueX{
											Choice: &d3pb.Extension_ValueX_StringValue{
												StringValue: &d3pb.String{Value: "b"},
											},
										},
									},
									{
										Url: &d3pb.Uri{Value: "https://example.com/Extension/QueryResponse"},
										Value: &d3pb.Extension_ValueX{
											Choice: &d3pb.Extension_ValueX_StringValue{
												StringValue: &d3pb.String{Value: "c"},
											},
										},
									},
								},
							},
							{
								Url: &d3pb.Uri{Value: "http://example.com/Extension/QueryData"},
								Extension: []*d3pb.Extension{
									{
										Url: &d3pb.Uri{Value: "https://example.com/Extension/QueryMnemonic"},
										Value: &d3pb.Extension_ValueX{
											Choice: &d3pb.Extension_ValueX_StringValue{
												StringValue: &d3pb.String{Value: "d"},
											},
										},
									},
									{
										Url: &d3pb.Uri{Value: "https://example.com/Extension/QueryQuestion"},
										Value: &d3pb.Extension_ValueX{
											Choice: &d3pb.Extension_ValueX_StringValue{
												StringValue: &d3pb.String{Value: "e"},
											},
										},
									},
									{
										Url: &d3pb.Uri{Value: "https://example.com/Extension/QueryResponse"},
										Value: &d3pb.Extension_ValueX{
											Choice: &d3pb.Extension_ValueX_StringValue{
												StringValue: &d3pb.String{Value: "f"},
											},
										},
									},
								},
							},
						},
					},
				},
				{
					ver: fhirversion.R4,
					r: &r4patientpb.Patient{
						Extension: []*d4pb.Extension{
							{
								Url: &d4pb.Uri{Value: "http://example.com/Extension/QueryData"},
								Extension: []*d4pb.Extension{
									{
										Url: &d4pb.Uri{Value: "https://example.com/Extension/QueryMnemonic"},
										Value: &d4pb.Extension_ValueX{
											Choice: &d4pb.Extension_ValueX_StringValue{
												StringValue: &d4pb.String{Value: "a"},
											},
										},
									},
									{
										Url: &d4pb.Uri{Value: "https://example.com/Extension/QueryQuestion"},
										Value: &d4pb.Extension_ValueX{
											Choice: &d4pb.Extension_ValueX_StringValue{
												StringValue: &d4pb.String{Value: "b"},
											},
										},
									},
									{
										Url: &d4pb.Uri{Value: "https://example.com/Extension/QueryResponse"},
										Value: &d4pb.Extension_ValueX{
											Choice: &d4pb.Extension_ValueX_StringValue{
												StringValue: &d4pb.String{Value: "c"},
											},
										},
									},
								},
							},
							{
								Url: &d4pb.Uri{Value: "http://example.com/Extension/QueryData"},
								Extension: []*d4pb.Extension{
									{
										Url: &d4pb.Uri{Value: "https://example.com/Extension/QueryMnemonic"},
										Value: &d4pb.Extension_ValueX{
											Choice: &d4pb.Extension_ValueX_StringValue{
												StringValue: &d4pb.String{Value: "d"},
											},
										},
									},
									{
										Url: &d4pb.Uri{Value: "https://example.com/Extension/QueryQuestion"},
										Value: &d4pb.Extension_ValueX{
											Choice: &d4pb.Extension_ValueX_StringValue{
												StringValue: &d4pb.String{Value: "e"},
											},
										},
									},
									{
										Url: &d4pb.Uri{Value: "https://example.com/Extension/QueryResponse"},
										Value: &d4pb.Extension_ValueX{
											Choice: &d4pb.Extension_ValueX_StringValue{
												StringValue: &d4pb.String{Value: "f"},
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
					r: &r5patientpb.Patient{
						Extension: []*d5pb.Extension{
							{
								Url: &d5pb.Uri{Value: "http://example.com/Extension/QueryData"},
								Extension: []*d5pb.Extension{
									{
										Url: &d5pb.Uri{Value: "https://example.com/Extension/QueryMnemonic"},
										Value: &d5pb.Extension_ValueX{
											Choice: &d5pb.Extension_ValueX_StringValue{
												StringValue: &d5pb.String{Value: "a"},
											},
										},
									},
									{
										Url: &d5pb.Uri{Value: "https://example.com/Extension/QueryQuestion"},
										Value: &d5pb.Extension_ValueX{
											Choice: &d5pb.Extension_ValueX_StringValue{
												StringValue: &d5pb.String{Value: "b"},
											},
										},
									},
									{
										Url: &d5pb.Uri{Value: "https://example.com/Extension/QueryResponse"},
										Value: &d5pb.Extension_ValueX{
											Choice: &d5pb.Extension_ValueX_StringValue{
												StringValue: &d5pb.String{Value: "c"},
											},
										},
									},
								},
							},
							{
								Url: &d5pb.Uri{Value: "http://example.com/Extension/QueryData"},
								Extension: []*d5pb.Extension{
									{
										Url: &d5pb.Uri{Value: "https://example.com/Extension/QueryMnemonic"},
										Value: &d5pb.Extension_ValueX{
											Choice: &d5pb.Extension_ValueX_StringValue{
												StringValue: &d5pb.String{Value: "d"},
											},
										},
									},
									{
										Url: &d5pb.Uri{Value: "https://example.com/Extension/QueryQuestion"},
										Value: &d5pb.Extension_ValueX{
											Choice: &d5pb.Extension_ValueX_StringValue{
												StringValue: &d5pb.String{Value: "e"},
											},
										},
									},
									{
										Url: &d5pb.Uri{Value: "https://example.com/Extension/QueryResponse"},
										Value: &d5pb.Extension_ValueX{
											Choice: &d5pb.Extension_ValueX_StringValue{
												StringValue: &d5pb.String{Value: "f"},
											},
										},
									},
								},
							},
						},
					},
				},
			},
			want: map[string]interface{}{
				"QueryData": []interface{}{
					map[string]interface{}{
						"QueryMnemonic": []interface{}{
							map[string]interface{}{
								"value": map[string]interface{}{
									"string": "a",
								},
							},
						},
						"QueryQuestion": []interface{}{
							map[string]interface{}{
								"value": map[string]interface{}{
									"string": "b",
								},
							},
						},
						"QueryResponse": []interface{}{
							map[string]interface{}{
								"value": map[string]interface{}{
									"string": "c",
								},
							},
						},
					},
					map[string]interface{}{
						"QueryMnemonic": []interface{}{
							map[string]interface{}{
								"value": map[string]interface{}{
									"string": "d",
								},
							},
						},
						"QueryQuestion": []interface{}{
							map[string]interface{}{
								"value": map[string]interface{}{
									"string": "e",
								},
							},
						},
						"QueryResponse": []interface{}{
							map[string]interface{}{
								"value": map[string]interface{}{
									"string": "f",
								},
							},
						},
					},
				},
			},
		},
		{
			name: "Extension last token collides with first-class field",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
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
					ver: fhirversion.R4,
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
				{
					ver: fhirversion.R5,
					r: &r5patientpb.Patient{
						Id: &d5pb.Id{Value: "id1"},
						Extension: []*d5pb.Extension{
							{
								Url: &d5pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition/id"},
								Value: &d5pb.Extension_ValueX{
									Choice: &d5pb.Extension_ValueX_StringValue{StringValue: &d5pb.String{Value: "id2"}},
								},
							},
						},
					},
				},
			},
			want: map[string]interface{}{
				"id": "id1",
				"hl7_org_fhir_StructureDefinition_id": []interface{}{
					map[string]interface{}{
						"value": map[string]interface{}{
							"string": "id2",
						},
					},
				},
			},
		},
		{
			name: "Extension last token collides with other extensions",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
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
					ver: fhirversion.R4,
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
				{
					ver: fhirversion.R5,
					r: &r5patientpb.Patient{
						Extension: []*d5pb.Extension{
							{
								Url: &d5pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition1/id"},
								Value: &d5pb.Extension_ValueX{
									Choice: &d5pb.Extension_ValueX_StringValue{StringValue: &d5pb.String{Value: "id1"}},
								},
							},
							{
								Url: &d5pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition2/id"},
								Value: &d5pb.Extension_ValueX{
									Choice: &d5pb.Extension_ValueX_StringValue{StringValue: &d5pb.String{Value: "id2"}},
								},
							},
						},
					},
				},
			},
			want: map[string]interface{}{
				"hl7_org_fhir_StructureDefinition1_id": []interface{}{map[string]interface{}{
					"value": map[string]interface{}{
						"string": "id1",
					},
				}},
				"hl7_org_fhir_StructureDefinition2_id": []interface{}{map[string]interface{}{
					"value": map[string]interface{}{
						"string": "id2",
					},
				}},
			},
		},
		{
			name: "Extension collides with other extension with different case",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
					r: &r3pb.Patient{
						Extension: []*d3pb.Extension{
							{
								Url: &d3pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition1/id"},
								Value: &d3pb.Extension_ValueX{
									Choice: &d3pb.Extension_ValueX_StringValue{StringValue: &d3pb.String{Value: "id1"}},
								},
							},
							{
								Url: &d3pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition2/ID"},
								Value: &d3pb.Extension_ValueX{
									Choice: &d3pb.Extension_ValueX_StringValue{StringValue: &d3pb.String{Value: "id2"}},
								},
							},
						},
					},
				},
				{
					ver: fhirversion.R4,
					r: &r4patientpb.Patient{
						Extension: []*d4pb.Extension{
							{
								Url: &d4pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition1/id"},
								Value: &d4pb.Extension_ValueX{
									Choice: &d4pb.Extension_ValueX_StringValue{StringValue: &d4pb.String{Value: "id1"}},
								},
							},
							{
								Url: &d4pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition2/ID"},
								Value: &d4pb.Extension_ValueX{
									Choice: &d4pb.Extension_ValueX_StringValue{StringValue: &d4pb.String{Value: "id2"}},
								},
							},
						},
					},
				},
				{
					ver: fhirversion.R5,
					r: &r5patientpb.Patient{
						Extension: []*d5pb.Extension{
							{
								Url: &d5pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition1/id"},
								Value: &d5pb.Extension_ValueX{
									Choice: &d5pb.Extension_ValueX_StringValue{StringValue: &d5pb.String{Value: "id1"}},
								},
							},
							{
								Url: &d5pb.Uri{Value: "http://hl7.org/fhir/StructureDefinition2/ID"},
								Value: &d5pb.Extension_ValueX{
									Choice: &d5pb.Extension_ValueX_StringValue{StringValue: &d5pb.String{Value: "id2"}},
								},
							},
						},
					},
				},
			},
			want: map[string]interface{}{
				"hl7_org_fhir_StructureDefinition1_id": []interface{}{map[string]interface{}{
					"value": map[string]interface{}{
						"string": "id1",
					},
				}},
				"hl7_org_fhir_StructureDefinition2_ID": []interface{}{map[string]interface{}{
					"value": map[string]interface{}{
						"string": "id2",
					},
				}},
			},
		},
		{
			name: "Extensions with both value and sub-extension - sub-extension is not skipped",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
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
										Extension: []*d3pb.Extension{
											{
												Url: &d3pb.Uri{Value: "text"},
												Value: &d3pb.Extension_ValueX{
													Choice: &d3pb.Extension_ValueX_StringValue{StringValue: &d3pb.String{Value: "White"}},
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
					ver: fhirversion.R4,
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
										Extension: []*d4pb.Extension{
											{
												Url: &d4pb.Uri{Value: "text"},
												Value: &d4pb.Extension_ValueX{
													Choice: &d4pb.Extension_ValueX_StringValue{StringValue: &d4pb.String{Value: "White"}},
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
					r: &r5patientpb.Patient{
						Extension: []*d5pb.Extension{
							{
								Url: &d5pb.Uri{Value: "http://hl7.org/fhir/us/core/StructureDefinition/us-core-race"},
								Extension: []*d5pb.Extension{
									{
										Url: &d5pb.Uri{Value: "ombCategory"},
										Value: &d5pb.Extension_ValueX{
											Choice: &d5pb.Extension_ValueX_Coding{
												Coding: &d5pb.Coding{
													System: &d5pb.Uri{Value: "urn:oid:2.16.840.1.113883.6.238"},
													Code:   &d5pb.Code{Value: "2106-3"},
												},
											},
										},
										Extension: []*d5pb.Extension{
											{
												Url: &d5pb.Uri{Value: "text"},
												Value: &d5pb.Extension_ValueX{
													Choice: &d5pb.Extension_ValueX_StringValue{StringValue: &d5pb.String{Value: "White"}},
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
			want: map[string]interface{}{
				"us_core_race": []interface{}{
					map[string]interface{}{
						"ombCategory": []interface{}{
							map[string]interface{}{
								"text": []interface{}{map[string]interface{}{
									"value": map[string]interface{}{"string": "White"},
								},
								},
								"value": map[string]interface{}{
									"coding": map[string]interface{}{
										"system": "urn:oid:2.16.840.1.113883.6.238",
										"code":   "2106-3",
									},
								},
							},
						},
					},
				},
			},
		},
		{
			name: "Extension field name collides with value - use full field name",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
					r: &r3pb.Patient{
						Extension: []*d3pb.Extension{
							{
								Url: &d3pb.Uri{Value: "http://hl7.org/fhir/us/core/StructureDefinition/us-core-race"},
								Value: &d3pb.Extension_ValueX{
									Choice: &d3pb.Extension_ValueX_StringValue{StringValue: &d3pb.String{Value: "Asian"}},
								},
								Extension: []*d3pb.Extension{
									{
										Url: &d3pb.Uri{Value: "http://hl7.org/fhir/us/core/StructureDefinition/us-core-race/value"},
										Value: &d3pb.Extension_ValueX{
											Choice: &d3pb.Extension_ValueX_StringValue{StringValue: &d3pb.String{Value: "Asian"}},
										},
									},
								},
							},
						},
					},
				},
				{
					ver: fhirversion.R4,
					r: &r4patientpb.Patient{
						Extension: []*d4pb.Extension{
							{
								Url: &d4pb.Uri{Value: "http://hl7.org/fhir/us/core/StructureDefinition/us-core-race"},
								Value: &d4pb.Extension_ValueX{
									Choice: &d4pb.Extension_ValueX_StringValue{StringValue: &d4pb.String{Value: "Asian"}},
								},
								Extension: []*d4pb.Extension{
									{
										Url: &d4pb.Uri{Value: "http://hl7.org/fhir/us/core/StructureDefinition/us-core-race/value"},
										Value: &d4pb.Extension_ValueX{
											Choice: &d4pb.Extension_ValueX_StringValue{StringValue: &d4pb.String{Value: "Asian"}},
										},
									},
								},
							},
						},
					},
				},
				{
					ver: fhirversion.R5,
					r: &r5patientpb.Patient{
						Extension: []*d5pb.Extension{
							{
								Url: &d5pb.Uri{Value: "http://hl7.org/fhir/us/core/StructureDefinition/us-core-race"},
								Value: &d5pb.Extension_ValueX{
									Choice: &d5pb.Extension_ValueX_StringValue{StringValue: &d5pb.String{Value: "Asian"}},
								},
								Extension: []*d5pb.Extension{
									{
										Url: &d5pb.Uri{Value: "http://hl7.org/fhir/us/core/StructureDefinition/us-core-race/value"},
										Value: &d5pb.Extension_ValueX{
											Choice: &d5pb.Extension_ValueX_StringValue{StringValue: &d5pb.String{Value: "Asian"}},
										},
									},
								},
							},
						},
					},
				},
			},
			want: map[string]interface{}{
				"us_core_race": []interface{}{
					map[string]interface{}{
						"value": map[string]interface{}{"string": "Asian"},
						"hl7_org_fhir_us_core_StructureDefinition_us_core_race_value": []interface{}{
							map[string]interface{}{
								"value": map[string]interface{}{"string": "Asian"},
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
					ver: fhirversion.STU3,
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
					ver: fhirversion.R4,
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
				{
					ver: fhirversion.R5,
					r: &r5patientpb.Patient{
						BirthDate: &d5pb.Date{
							ValueUs:   1463529600000000,
							Precision: d5pb.Date_DAY,
							Id: &d5pb.String{
								Value: "a3",
							},
							Extension: []*d5pb.Extension{{
								Url: &d5pb.Uri{
									Value: "http://hl7.org/fhir/StructureDefinition/patient-birthTime",
								},
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
			},
			want: map[string]interface{}{
				"_birthDate": map[string]interface{}{
					"patient_birthTime": []interface{}{map[string]interface{}{
						"value": map[string]interface{}{
							"dateTime": "2016-05-18T10:28:45Z",
						},
					}},
				},
				"birthDate": "2016-05-18",
			},
		},
		{
			name: "Primitive extension no value",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
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
									Value: jsonpbhelper.PrimitiveHasNoValueURL,
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
					ver: fhirversion.R4,
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
									Value: jsonpbhelper.PrimitiveHasNoValueURL,
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
				{
					ver: fhirversion.R5,
					r: &r5patientpb.Patient{
						BirthDate: &d5pb.Date{
							Extension: []*d5pb.Extension{{
								Url: &d5pb.Uri{
									Value: "http://hl7.org/fhir/StructureDefinition/patient-birthTime",
								},
								Value: &d5pb.Extension_ValueX{
									Choice: &d5pb.Extension_ValueX_DateTime{
										DateTime: &d5pb.DateTime{
											ValueUs:   1463567325000000,
											Timezone:  "Z",
											Precision: d5pb.DateTime_SECOND,
										},
									},
								},
							}, {
								Url: &d5pb.Uri{
									Value: jsonpbhelper.PrimitiveHasNoValueURL,
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
			want: map[string]interface{}{
				"_birthDate": map[string]interface{}{
					"patient_birthTime": []interface{}{
						map[string]interface{}{
							"value": map[string]interface{}{
								"dateTime": "2016-05-18T10:28:45Z",
							},
						},
					},
				},
			},
		},
		{
			name: "Contained Resource",
			inputs: []mvr{
				{
					ver: fhirversion.R4,
					r: &r4conditionpb.Condition{
						Contained: []*anypb.Any{
							marshalToAny(t, &r4pb.ContainedResource{
								OneofResource: &r4pb.ContainedResource_Patient{
									Patient: &r4patientpb.Patient{
										Id: &d4pb.Id{
											Value: "p1",
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
							}),
						},
						Asserter: &d4pb.Reference{
							Reference: &d4pb.Reference_Fragment{
								Fragment: &d4pb.String{Value: "p1"},
							},
						},
					},
				},
				{
					ver: fhirversion.STU3,
					r: &r3pb.Condition{
						Contained: []*r3pb.ContainedResource{
							{
								OneofResource: &r3pb.ContainedResource_Patient{
									Patient: &r3pb.Patient{
										Id: &d3pb.Id{
											Value: "p1",
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
						},
						Asserter: &d3pb.Reference{
							Reference: &d3pb.Reference_Fragment{
								Fragment: &d3pb.String{Value: "p1"},
							},
						},
					},
				},
			},
			want: map[string]any{
				"contained": []any{
					`{"resourceType":"Patient","id":"p1","patient_birthTime":[{"value":{"dateTime":"2016-05-18T10:28:45Z"}}]}`,
				},
				"asserter": map[string]any{
					"reference": "#p1",
				},
			},
		},
		{
			name: "Contained Resource R5",
			inputs: []mvr{
				{
					ver: fhirversion.R5,
					r: &r5conditionpb.Condition{
						Contained: []*anypb.Any{
							marshalToAny(t, &r5pb.ContainedResource{
								OneofResource: &r5pb.ContainedResource_Patient{
									Patient: &r5patientpb.Patient{
										Id: &d5pb.Id{
											Value: "p1",
										},
										Extension: []*d5pb.Extension{{
											Url: &d5pb.Uri{
												Value: "http://hl7.org/fhir/StructureDefinition/patient-birthTime",
											},
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
							}),
						},
						Participant: []*r5conditionpb.Condition_Participant{
							{
								Actor: &d5pb.Reference{
									Reference: &d5pb.Reference_Fragment{
										Fragment: &d5pb.String{Value: "p1"},
									},
								},
							},
						},
					},
				},
			},
			want: map[string]any{
				"contained": []any{
					`{"resourceType":"Patient","id":"p1","patient_birthTime":[{"value":{"dateTime":"2016-05-18T10:28:45Z"}}]}`,
				},
				"participant": []any{
					map[string]any{
						"actor": map[string]any{
							"reference": "#p1",
						},
					},
				},
			},
		},
		{
			name: "extension with valueString",
			inputs: []mvr{
				{
					ver: fhirversion.R4,
					r: &r4patientpb.Patient{
						Extension: []*d4pb.Extension{
							{
								Url:   &d4pb.Uri{Value: "http://hl7.org/extension_url"},
								Value: &d4pb.Extension_ValueX{Choice: &d4pb.Extension_ValueX_StringValue{StringValue: &d4pb.String{Value: "extension_value"}}},
							},
						},
					},
				},
				{
					ver: fhirversion.STU3,
					r: &r3pb.Patient{
						Extension: []*d3pb.Extension{
							{
								Url:   &d3pb.Uri{Value: "http://hl7.org/extension_url"},
								Value: &d3pb.Extension_ValueX{Choice: &d3pb.Extension_ValueX_StringValue{StringValue: &d3pb.String{Value: "extension_value"}}},
							},
						},
					},
				},
				{
					ver: fhirversion.R5,
					r: &r5patientpb.Patient{
						Extension: []*d5pb.Extension{
							{
								Url:   &d5pb.Uri{Value: "http://hl7.org/extension_url"},
								Value: &d5pb.Extension_ValueX{Choice: &d5pb.Extension_ValueX_StringValue{StringValue: &d5pb.String{Value: "extension_value"}}},
							},
						},
					},
				},
			},
			want: map[string]any{
				"extension_url": []any{
					map[string]any{
						"value": map[string]any{"string": "extension_value"},
					},
				},
			},
		},
		{
			name: "extension with valueId",
			inputs: []mvr{
				{
					ver: fhirversion.R4,
					r: &r4patientpb.Patient{
						Extension: []*d4pb.Extension{
							{
								Url:   &d4pb.Uri{Value: "http://hl7.org/extension_url"},
								Value: &d4pb.Extension_ValueX{Choice: &d4pb.Extension_ValueX_Id{Id: &d4pb.Id{Value: "extension_value"}}},
							},
						},
					},
				},
				{
					ver: fhirversion.STU3,
					r: &r3pb.Patient{
						Extension: []*d3pb.Extension{
							{
								Url:   &d3pb.Uri{Value: "http://hl7.org/extension_url"},
								Value: &d3pb.Extension_ValueX{Choice: &d3pb.Extension_ValueX_Id{Id: &d3pb.Id{Value: "extension_value"}}},
							},
						},
					},
				},
				{
					ver: fhirversion.R5,
					r: &r5patientpb.Patient{
						Extension: []*d5pb.Extension{
							{
								Url:   &d5pb.Uri{Value: "http://hl7.org/extension_url"},
								Value: &d5pb.Extension_ValueX{Choice: &d5pb.Extension_ValueX_Id{Id: &d5pb.Id{Value: "extension_value"}}},
							},
						},
					},
				},
			},
			want: map[string]any{
				"extension_url": []any{
					map[string]any{
						"value": map[string]any{"id": "extension_value"},
					},
				},
			},
		},
		{
			name: "extension with Id and valueId", // Id should be omitted but valueId should be kept
			inputs: []mvr{
				{
					ver: fhirversion.R4,
					r: &r4patientpb.Patient{
						Extension: []*d4pb.Extension{
							{
								Id:    &d4pb.String{Value: "exampleExtension"},
								Url:   &d4pb.Uri{Value: "http://hl7.org/extension_url"},
								Value: &d4pb.Extension_ValueX{Choice: &d4pb.Extension_ValueX_Id{Id: &d4pb.Id{Value: "extension_value"}}},
							},
						},
					},
				},
				{
					ver: fhirversion.STU3,
					r: &r3pb.Patient{
						Extension: []*d3pb.Extension{
							{
								Id:    &d3pb.String{Value: "exampleExtension"},
								Url:   &d3pb.Uri{Value: "http://hl7.org/extension_url"},
								Value: &d3pb.Extension_ValueX{Choice: &d3pb.Extension_ValueX_Id{Id: &d3pb.Id{Value: "extension_value"}}},
							},
						},
					},
				},
				{
					ver: fhirversion.R5,
					r: &r5patientpb.Patient{
						Extension: []*d5pb.Extension{
							{
								Id:    &d5pb.String{Value: "exampleExtension"},
								Url:   &d5pb.Uri{Value: "http://hl7.org/extension_url"},
								Value: &d5pb.Extension_ValueX{Choice: &d5pb.Extension_ValueX_Id{Id: &d5pb.Id{Value: "extension_value"}}},
							},
						},
					},
				},
			},
			want: map[string]any{
				"extension_url": []any{
					map[string]any{
						"value": map[string]any{"id": "extension_value"},
					},
				},
			},
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			for _, i := range test.inputs {
				t.Run(i.ver.String(), func(t *testing.T) {
					marshaller, err := NewAnalyticsV2MarshallerWithInferredSchema(10, i.ver)
					if err != nil {
						t.Fatalf("failed to create marshaller %v: %v", test.name, err)
					}
					gotJSON, err := marshalResourceAndValidate(marshaller, i.r)
					if err != nil {
						t.Fatalf("marshal failed on %v: %v", test.name, err)
					}
					var got interface{}
					if err := json.Unmarshal(gotJSON, &got); err != nil {
						t.Fatalf("json.Unmarshal(%q) failed: %v", gotJSON, err)
					}

					if diff := cmp.Diff(test.want, got, compareJSON); diff != "" {
						t.Errorf("marshal %v: diff: %s", test.name, diff)
					}
				})
			}
		})
	}
}

func TestMarshalMessageForAnalyticsV2_InferredSchema_Error(t *testing.T) {
	tests := []struct {
		name   string
		inputs []mvr
	}{
		{
			name: "Extension collides with value for Analytics V2",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
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
										Extension: []*d3pb.Extension{
											{
												Url: &d3pb.Uri{Value: "value"},
												Value: &d3pb.Extension_ValueX{
													Choice: &d3pb.Extension_ValueX_StringValue{StringValue: &d3pb.String{Value: "White"}},
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
					ver: fhirversion.R4,
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
										Extension: []*d4pb.Extension{
											{
												Url: &d4pb.Uri{Value: "value"},
												Value: &d4pb.Extension_ValueX{
													Choice: &d4pb.Extension_ValueX_StringValue{StringValue: &d4pb.String{Value: "White"}},
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
					r: &r5patientpb.Patient{
						Extension: []*d5pb.Extension{
							{
								Url: &d5pb.Uri{Value: "http://hl7.org/fhir/us/core/StructureDefinition/us-core-race"},
								Extension: []*d5pb.Extension{
									{
										Url: &d5pb.Uri{Value: "ombCategory"},
										Value: &d5pb.Extension_ValueX{
											Choice: &d5pb.Extension_ValueX_Coding{
												Coding: &d5pb.Coding{
													System: &d5pb.Uri{Value: "urn:oid:2.16.840.1.113883.6.238"},
													Code:   &d5pb.Code{Value: "2106-3"},
												},
											},
										},
										Extension: []*d5pb.Extension{
											{
												Url: &d5pb.Uri{Value: "value"},
												Value: &d5pb.Extension_ValueX{
													Choice: &d5pb.Extension_ValueX_StringValue{StringValue: &d5pb.String{Value: "White"}},
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
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			for _, i := range test.inputs {
				t.Run(i.ver.String(), func(t *testing.T) {
					marshaller, err := NewAnalyticsV2MarshallerWithInferredSchema(10, i.ver)
					if err != nil {
						t.Fatalf("failed to create marshaller %v: %v", test.name, err)
					}
					_, err = marshalResourceAndValidate(marshaller, i.r)
					if err == nil {
						t.Errorf("marshalMessageToMap on %v did not return an error", test.name)
					}
					var e *ExtensionError
					if !errors.As(err, &e) {
						t.Errorf("marshalMessageToMap on %v expect ResourceError, got %T ", test.name, err)
					}
				})
			}
		})
	}
}
