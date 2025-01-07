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

package jsonpbhelper

import (
	"encoding/json"
	"errors"
	"fmt"
	"regexp"
	"strconv"
	"strings"
	"testing"
	"time"

	"github.com/google/go-cmp/cmp"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/reflect/protoreflect"
	"google.golang.org/protobuf/testing/protocmp"

	d4pb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/datatypes_go_proto"
	r4basicpb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/basic_go_proto"
	r4patientpb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/patient_go_proto"
	d5pb "github.com/google/fhir/go/proto/google/fhir/proto/r5/core/datatypes_go_proto"
	r5basicpb "github.com/google/fhir/go/proto/google/fhir/proto/r5/core/resources/basic_go_proto"
	r5patientpb "github.com/google/fhir/go/proto/google/fhir/proto/r5/core/resources/patient_go_proto"
	c3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/codes_go_proto"
	d3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/datatypes_go_proto"
	e3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/fhirproto_extensions_go_proto"
	r3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/resources_go_proto"
)

func TestIsPrimitiveType(t *testing.T) {
	tests := []struct {
		name string
		pb   proto.Message
		want bool
	}{
		{
			"Id_v3",
			&d3pb.Id{},
			true,
		},
		{
			"String_v3",
			&d3pb.String{},
			true,
		},
		{
			"Code_v3",
			&d3pb.Code{},
			true,
		},
		{
			"NarrativeStatusCode_v3",
			&c3pb.NarrativeStatusCode{},
			true,
		},
		{
			"ResearchStudy_v3",
			&r3pb.ResearchStudy{},
			false,
		},
	}
	for _, test := range tests {
		des := test.pb.ProtoReflect().Descriptor()
		if got := IsPrimitiveType(des); got != test.want {
			t.Errorf("IsPrimitiveType for %v: got %v, want %v", test.name, got, test.want)
		}
	}
}

func TestExtractTimezone(t *testing.T) {
	tests := []struct {
		time string
		want string
	}{
		{
			"2017-12-21T12:34:56.789000+00:00",
			"+00:00",
		},
		{
			"2017-12-21T12:34:56.789-03:00",
			"-03:00",
		},
	}

	for _, test := range tests {
		parsedTime, err := time.Parse("2006-01-02T15:04:05-07:00", test.time)
		if err != nil {
			t.Fatalf("failed to parse time %v: %v", test.time, err)
		}
		if got := ExtractTimezone(parsedTime); got != test.want {
			t.Errorf("extractTimezone(\"%v\"): got %v, want %v", test.time, got, test.want)
		}
	}
}

func TestOffsetToSeconds(t *testing.T) {
	tests := []struct {
		name, offset string
		fail         bool
		want         int
	}{
		{
			"positive offset",
			"+11:05",
			false,
			39900, // 11*3600 + 5*60
		},
		{
			"negative offset",
			"-03:30",
			false,
			-12600, // -(3*3600 + 30*60)
		},
		{
			"missing sign",
			"11:00",
			true,
			0,
		},
		{
			"missing minute",
			"+11",
			true,
			0,
		},
		{
			"bad minute",
			"+11:xy",
			true,
			0,
		},
		{
			"extra second",
			"-11:30:20",
			true,
			0,
		},
		{
			"invalid integers",
			"+unexpected:string",
			true,
			0,
		},
	}

	for _, test := range tests {
		got, err := offsetToSeconds(test.offset)
		if err != nil {
			if !test.fail {
				t.Errorf("offsetToSecond %v: %v", test.name, err)
			}
			continue
		}
		if test.fail {
			t.Errorf("offsetToSecond %v: expected error not raised", test.name)
		}
		if got != test.want {
			t.Errorf("offsetToSecond %v: got %v, want %v", test.name, got, test.want)
		}
	}
}

func TestIsSubsecondIsSubmilli(t *testing.T) {
	tests := []struct {
		input               string
		subsecond, submilli bool
	}{
		{
			"",
			false,
			false,
		},
		{
			"2017-01-23T00:00:00",
			false,
			false,
		},
		{
			"2017-01-23T00:00:00+00:00",
			false,
			false,
		},
		{
			"2017-01-23T00:00:00.0",
			true,
			false,
		},
		{
			"2017-01-23T00:00:00.012Z",
			true,
			false,
		},
		{
			"2017-01-23T00:00:00.0123+00:00",
			true,
			true,
		},
		{
			"2017-01-23T12:34:56.789+00:00",
			true,
			false,
		},
		{
			"2017-01-23T12:34:56.7890+00:00",
			true,
			true,
		},
	}
	for _, test := range tests {
		if got := IsSubsecond(test.input); got != test.subsecond {
			t.Errorf("isSubsecond(%v): got %v, want %v", test.input, got, test.subsecond)
		}
		if got := IsSubmilli(test.input); got != test.submilli {
			t.Errorf("isSubmilli(%v): got %v, want %v", test.input, got, test.submilli)
		}
	}
}

func TestGetTimestampUsec(t *testing.T) {
	tests := []struct {
		name string
		time time.Time
		ts   int64
	}{
		{
			"2018-04-13",
			time.Date(2018, time.April, 13, 0, 0, 0, 0, time.UTC),
			1523577600000000,
		},
		{
			"4018-04-13",
			time.Date(4018, time.April, 13, 0, 0, 0, 0, time.UTC),
			64637481600000000,
		},
	}
	for _, test := range tests {
		// Test conversion from time object to timestamp.
		got := GetTimestampUsec(test.time)
		if want := test.ts; got != want {
			t.Errorf("GetTimestampUsec(%v): got %v, want %v", test.name, got, want)
		}
	}
	for _, test := range tests {
		// Test conversion from timestamp to time object.
		got, err := GetTimeFromUsec(test.ts, "UTC")
		if err != nil {
			t.Errorf("Unexpected error in GetTimeFromUsec: %v", err)
		}
		if want := test.time; !got.Equal(want) {
			t.Errorf("getTimeFromUsec(%v): got %v, want %v", test.name, got, want)
		}
	}
}

func TestHasInternalExtension(t *testing.T) {
	tests := []struct {
		name    string
		pb, ext proto.Message
		want    bool
	}{
		{
			"nil proto",
			nil,
			nil,
			false,
		},
		{
			"R3 has extension",
			&d3pb.String{
				Extension: []*d3pb.Extension{{
					Url: &d3pb.Uri{
						Value: "https://g.co/fhir/StructureDefinition/base64Binary-separatorStride",
					},
				}},
			},
			&e3pb.Base64BinarySeparatorStride{},
			true,
		},
		{
			"R3 has different extension",
			&d3pb.String{
				Extension: []*d3pb.Extension{{
					Url: &d3pb.Uri{
						Value: "https://g.co/fhir/StructureDefinition/base64Binary-separatorStride",
					},
				}},
			},
			&e3pb.PrimitiveHasNoValue{
				ValueBoolean: &d3pb.Boolean{
					Value: false,
				},
			},
			false,
		},
	}
	for _, test := range tests {
		if got := HasInternalExtension(test.pb, test.ext); got != test.want {
			t.Errorf("HasInternalExtension(%v): got %v, want %v", test.name, got, test.want)
		}
	}
}

func TestRemoveInternalExtension(t *testing.T) {
	tests := []struct {
		name          string
		pb, ext, want proto.Message
	}{
		{
			"R3 extension removed",
			&d3pb.Base64Binary{
				Value: []byte("val"),
				Extension: []*d3pb.Extension{{
					Url: &d3pb.Uri{
						Value: "https://g.co/fhir/StructureDefinition/base64Binary-separatorStride",
					},
					Extension: []*d3pb.Extension{{
						Url: &d3pb.Uri{
							Value: "separator",
						},
						Value: &d3pb.Extension_ValueX{
							Choice: &d3pb.Extension_ValueX_StringValue{
								StringValue: &d3pb.String{
									Value: "  ",
								},
							},
						},
					}, {
						Url: &d3pb.Uri{
							Value: "stride",
						},
						Value: &d3pb.Extension_ValueX{
							Choice: &d3pb.Extension_ValueX_PositiveInt{
								PositiveInt: &d3pb.PositiveInt{
									Value: 2,
								},
							},
						},
					}},
				}},
			},
			&e3pb.Base64BinarySeparatorStride{
				Stride: &d3pb.PositiveInt{
					Value: 2,
				},
				Separator: &d3pb.String{
					Value: "  ",
				},
			},
			&d3pb.Base64Binary{
				Value: []byte("val"),
			},
		},
		{
			"R3 does not exists",
			&d3pb.Base64Binary{
				Value: []byte("val"),
				Extension: []*d3pb.Extension{{
					Url: &d3pb.Uri{
						Value: "https://g.co/fhir/StructureDefinition/base64Binary-separatorStride",
					},
					Extension: []*d3pb.Extension{{
						Url: &d3pb.Uri{
							Value: "separator",
						},
						Value: &d3pb.Extension_ValueX{
							Choice: &d3pb.Extension_ValueX_StringValue{
								StringValue: &d3pb.String{
									Value: "  ",
								},
							},
						},
					}, {
						Url: &d3pb.Uri{
							Value: "stride",
						},
						Value: &d3pb.Extension_ValueX{
							Choice: &d3pb.Extension_ValueX_PositiveInt{
								PositiveInt: &d3pb.PositiveInt{
									Value: 2,
								},
							},
						},
					}},
				}},
			},
			&e3pb.PrimitiveHasNoValue{
				ValueBoolean: &d3pb.Boolean{
					Value: true,
				},
			},
			&d3pb.Base64Binary{
				Value: []byte("val"),
				Extension: []*d3pb.Extension{{
					Url: &d3pb.Uri{
						Value: "https://g.co/fhir/StructureDefinition/base64Binary-separatorStride",
					},
					Extension: []*d3pb.Extension{{
						Url: &d3pb.Uri{
							Value: "separator",
						},
						Value: &d3pb.Extension_ValueX{
							Choice: &d3pb.Extension_ValueX_StringValue{
								StringValue: &d3pb.String{
									Value: "  ",
								},
							},
						},
					}, {
						Url: &d3pb.Uri{
							Value: "stride",
						},
						Value: &d3pb.Extension_ValueX{
							Choice: &d3pb.Extension_ValueX_PositiveInt{
								PositiveInt: &d3pb.PositiveInt{
									Value: 2,
								},
							},
						},
					}},
				}},
			},
		},
	}
	for _, test := range tests {
		err := RemoveInternalExtension(test.pb, test.ext)
		if err != nil {
			t.Errorf("RemoveInternalExtension(%v): %v", test.name, err)
		}
		if diff := cmp.Diff(test.want, test.pb, protocmp.Transform()); diff != "" {
			t.Errorf("RemoveInternalExtension(%v) returned unexpected diff: (-want, +got) %v", test.name, diff)
		}
	}
}

func TestHasExtension(t *testing.T) {
	tests := []struct {
		name string
		pb   proto.Message
		url  string
		want bool
	}{
		{
			"nil proto",
			nil,
			"",
			false,
		},
		{
			"R3 has extension",
			&d3pb.String{
				Extension: []*d3pb.Extension{{
					Url: &d3pb.Uri{
						Value: "https://g.co/fhir/StructureDefinition/base64Binary-separatorStride",
					},
				}},
			},
			"https://g.co/fhir/StructureDefinition/base64Binary-separatorStride",
			true,
		},
		{
			"R3 has different extension",
			&d3pb.String{
				Extension: []*d3pb.Extension{{
					Url: &d3pb.Uri{
						Value: "https://g.co/fhir/StructureDefinition/base64Binary-separatorStride",
					},
				}},
			},
			"https://g.co/fhir/StructureDefinition/primitiveHasNoValue",
			false,
		},
		{
			"R3 has extension with empty URL",
			&d3pb.String{
				Extension: []*d3pb.Extension{{
					Url: &d3pb.Uri{
						Value: "",
					},
				}},
			},
			"",
			true,
		},
		{
			"R3 has extension with no URL",
			&d3pb.String{
				Extension: []*d3pb.Extension{{
					Value: &d3pb.Extension_ValueX{
						Choice: &d3pb.Extension_ValueX_StringValue{
							StringValue: &d3pb.String{
								Value: "  ",
							},
						},
					},
				},
				},
			},
			"",
			false,
		},
	}

	for _, test := range tests {
		if got := HasExtension(test.pb, test.url); got != test.want {
			t.Errorf("HasExtension(%v): got %v, want %v", test.name, got, test.want)
		}
	}
}

func TestGetExtension(t *testing.T) {
	tests := []struct {
		name string
		pb   proto.Message
		url  string
		want proto.Message
	}{
		{
			"nil proto",
			nil,
			"",
			nil,
		},
		{
			"R3 got extension",
			&d3pb.Base64Binary{
				Value: []byte("val"),
				Extension: []*d3pb.Extension{{
					Url: &d3pb.Uri{
						Value: "https://g.co/fhir/StructureDefinition/base64Binary-separatorStride",
					},
					Extension: []*d3pb.Extension{{
						Url: &d3pb.Uri{
							Value: "separator",
						},
						Value: &d3pb.Extension_ValueX{
							Choice: &d3pb.Extension_ValueX_StringValue{
								StringValue: &d3pb.String{
									Value: "  ",
								},
							},
						},
					}, {
						Url: &d3pb.Uri{
							Value: "stride",
						},
						Value: &d3pb.Extension_ValueX{
							Choice: &d3pb.Extension_ValueX_PositiveInt{
								PositiveInt: &d3pb.PositiveInt{
									Value: 2,
								},
							},
						},
					}},
				}},
			},
			"https://g.co/fhir/StructureDefinition/base64Binary-separatorStride",
			&d3pb.Extension{
				Url: &d3pb.Uri{
					Value: "https://g.co/fhir/StructureDefinition/base64Binary-separatorStride",
				},
				Extension: []*d3pb.Extension{{
					Url: &d3pb.Uri{
						Value: "separator",
					},
					Value: &d3pb.Extension_ValueX{
						Choice: &d3pb.Extension_ValueX_StringValue{
							StringValue: &d3pb.String{
								Value: "  ",
							},
						},
					},
				}, {
					Url: &d3pb.Uri{
						Value: "stride",
					},
					Value: &d3pb.Extension_ValueX{
						Choice: &d3pb.Extension_ValueX_PositiveInt{
							PositiveInt: &d3pb.PositiveInt{
								Value: 2,
							},
						},
					},
				}},
			},
		},
		{
			"R3 does not exists",
			&d3pb.Base64Binary{
				Value: []byte("val"),
				Extension: []*d3pb.Extension{{
					Url: &d3pb.Uri{
						Value: "https://g.co/fhir/StructureDefinition/base64Binary-separatorStride",
					},
					Extension: []*d3pb.Extension{{
						Url: &d3pb.Uri{
							Value: "separator",
						},
						Value: &d3pb.Extension_ValueX{
							Choice: &d3pb.Extension_ValueX_StringValue{
								StringValue: &d3pb.String{
									Value: "  ",
								},
							},
						},
					}, {
						Url: &d3pb.Uri{
							Value: "stride",
						},
						Value: &d3pb.Extension_ValueX{
							Choice: &d3pb.Extension_ValueX_PositiveInt{
								PositiveInt: &d3pb.PositiveInt{
									Value: 2,
								},
							},
						},
					}},
				}},
			},
			"https://g.co/fhir/StructureDefinition/primitiveHasNoValue",
			nil,
		},
		{
			"R3 remove extension with empty URL",
			&d3pb.Base64Binary{
				Value: []byte("val"),
				Extension: []*d3pb.Extension{{
					Url: &d3pb.Uri{
						Value: "",
					},
					Extension: []*d3pb.Extension{{
						Url: &d3pb.Uri{
							Value: "separator",
						},
						Value: &d3pb.Extension_ValueX{
							Choice: &d3pb.Extension_ValueX_StringValue{
								StringValue: &d3pb.String{
									Value: "  ",
								},
							},
						},
					}, {
						Url: &d3pb.Uri{
							Value: "stride",
						},
						Value: &d3pb.Extension_ValueX{
							Choice: &d3pb.Extension_ValueX_PositiveInt{
								PositiveInt: &d3pb.PositiveInt{
									Value: 2,
								},
							},
						},
					}},
				}},
			},
			"",
			&d3pb.Extension{
				Url: &d3pb.Uri{
					Value: "",
				},
				Extension: []*d3pb.Extension{{
					Url: &d3pb.Uri{
						Value: "separator",
					},
					Value: &d3pb.Extension_ValueX{
						Choice: &d3pb.Extension_ValueX_StringValue{
							StringValue: &d3pb.String{
								Value: "  ",
							},
						},
					},
				}, {
					Url: &d3pb.Uri{
						Value: "stride",
					},
					Value: &d3pb.Extension_ValueX{
						Choice: &d3pb.Extension_ValueX_PositiveInt{
							PositiveInt: &d3pb.PositiveInt{
								Value: 2,
							},
						},
					},
				}},
			},
		},
		{
			"R3 extension with no URL",
			&d3pb.Base64Binary{
				Value: []byte("val"),
				Extension: []*d3pb.Extension{{
					Extension: []*d3pb.Extension{{
						Url: &d3pb.Uri{
							Value: "separator",
						},
						Value: &d3pb.Extension_ValueX{
							Choice: &d3pb.Extension_ValueX_StringValue{
								StringValue: &d3pb.String{
									Value: "  ",
								},
							},
						},
					}, {
						Url: &d3pb.Uri{
							Value: "stride",
						},
						Value: &d3pb.Extension_ValueX{
							Choice: &d3pb.Extension_ValueX_PositiveInt{
								PositiveInt: &d3pb.PositiveInt{
									Value: 2,
								},
							},
						},
					}},
				}},
			},
			"",
			nil,
		},
	}

	for _, test := range tests {
		got, err := GetExtension(test.pb, test.url)
		if err != nil {
			t.Errorf("RemoveExtension(%v): %v", test.name, err)
		}
		if diff := cmp.Diff(test.want, got, protocmp.Transform()); diff != "" {
			t.Errorf("RemoveExtension(%v) returned unexpected diff: (-want, +got) %v", test.name, diff)
		}
	}
}

func TestRemoveExtension(t *testing.T) {
	tests := []struct {
		name string
		pb   proto.Message
		url  string
		want proto.Message
	}{
		{
			"nil proto",
			nil,
			"",
			nil,
		},
		{
			"R3 extension removed",
			&d3pb.Base64Binary{
				Value: []byte("val"),
				Extension: []*d3pb.Extension{{
					Url: &d3pb.Uri{
						Value: "https://g.co/fhir/StructureDefinition/base64Binary-separatorStride",
					},
					Extension: []*d3pb.Extension{{
						Url: &d3pb.Uri{
							Value: "separator",
						},
						Value: &d3pb.Extension_ValueX{
							Choice: &d3pb.Extension_ValueX_StringValue{
								StringValue: &d3pb.String{
									Value: "  ",
								},
							},
						},
					}, {
						Url: &d3pb.Uri{
							Value: "stride",
						},
						Value: &d3pb.Extension_ValueX{
							Choice: &d3pb.Extension_ValueX_PositiveInt{
								PositiveInt: &d3pb.PositiveInt{
									Value: 2,
								},
							},
						},
					}},
				}},
			},
			"https://g.co/fhir/StructureDefinition/base64Binary-separatorStride",
			&d3pb.Base64Binary{
				Value: []byte("val"),
			},
		},
		{
			"R3 does not exists",
			&d3pb.Base64Binary{
				Value: []byte("val"),
				Extension: []*d3pb.Extension{{
					Url: &d3pb.Uri{
						Value: "https://g.co/fhir/StructureDefinition/base64Binary-separatorStride",
					},
					Extension: []*d3pb.Extension{{
						Url: &d3pb.Uri{
							Value: "separator",
						},
						Value: &d3pb.Extension_ValueX{
							Choice: &d3pb.Extension_ValueX_StringValue{
								StringValue: &d3pb.String{
									Value: "  ",
								},
							},
						},
					}, {
						Url: &d3pb.Uri{
							Value: "stride",
						},
						Value: &d3pb.Extension_ValueX{
							Choice: &d3pb.Extension_ValueX_PositiveInt{
								PositiveInt: &d3pb.PositiveInt{
									Value: 2,
								},
							},
						},
					}},
				}},
			},
			"https://g.co/fhir/StructureDefinition/primitiveHasNoValue",
			&d3pb.Base64Binary{
				Value: []byte("val"),
				Extension: []*d3pb.Extension{{
					Url: &d3pb.Uri{
						Value: "https://g.co/fhir/StructureDefinition/base64Binary-separatorStride",
					},
					Extension: []*d3pb.Extension{{
						Url: &d3pb.Uri{
							Value: "separator",
						},
						Value: &d3pb.Extension_ValueX{
							Choice: &d3pb.Extension_ValueX_StringValue{
								StringValue: &d3pb.String{
									Value: "  ",
								},
							},
						},
					}, {
						Url: &d3pb.Uri{
							Value: "stride",
						},
						Value: &d3pb.Extension_ValueX{
							Choice: &d3pb.Extension_ValueX_PositiveInt{
								PositiveInt: &d3pb.PositiveInt{
									Value: 2,
								},
							},
						},
					}},
				}},
			},
		},
		{
			"R3 remove extension with empty URL",
			&d3pb.Base64Binary{
				Value: []byte("val"),
				Extension: []*d3pb.Extension{{
					Url: &d3pb.Uri{
						Value: "",
					},
					Extension: []*d3pb.Extension{{
						Url: &d3pb.Uri{
							Value: "separator",
						},
						Value: &d3pb.Extension_ValueX{
							Choice: &d3pb.Extension_ValueX_StringValue{
								StringValue: &d3pb.String{
									Value: "  ",
								},
							},
						},
					}, {
						Url: &d3pb.Uri{
							Value: "stride",
						},
						Value: &d3pb.Extension_ValueX{
							Choice: &d3pb.Extension_ValueX_PositiveInt{
								PositiveInt: &d3pb.PositiveInt{
									Value: 2,
								},
							},
						},
					}},
				}},
			},
			"",
			&d3pb.Base64Binary{
				Value: []byte("val"),
			},
		},
		{
			"R3 extension with no URL",
			&d3pb.Base64Binary{
				Value: []byte("val"),
				Extension: []*d3pb.Extension{{
					Extension: []*d3pb.Extension{{
						Url: &d3pb.Uri{
							Value: "separator",
						},
						Value: &d3pb.Extension_ValueX{
							Choice: &d3pb.Extension_ValueX_StringValue{
								StringValue: &d3pb.String{
									Value: "  ",
								},
							},
						},
					}, {
						Url: &d3pb.Uri{
							Value: "stride",
						},
						Value: &d3pb.Extension_ValueX{
							Choice: &d3pb.Extension_ValueX_PositiveInt{
								PositiveInt: &d3pb.PositiveInt{
									Value: 2,
								},
							},
						},
					}},
				}},
			},
			"",
			&d3pb.Base64Binary{
				Value: []byte("val"),
				Extension: []*d3pb.Extension{{
					Extension: []*d3pb.Extension{{
						Url: &d3pb.Uri{
							Value: "separator",
						},
						Value: &d3pb.Extension_ValueX{
							Choice: &d3pb.Extension_ValueX_StringValue{
								StringValue: &d3pb.String{
									Value: "  ",
								},
							},
						},
					}, {
						Url: &d3pb.Uri{
							Value: "stride",
						},
						Value: &d3pb.Extension_ValueX{
							Choice: &d3pb.Extension_ValueX_PositiveInt{
								PositiveInt: &d3pb.PositiveInt{
									Value: 2,
								},
							},
						},
					}},
				}},
			},
		},
	}

	for _, test := range tests {
		err := RemoveExtension(test.pb, test.url)
		if err != nil {
			t.Errorf("RemoveExtension(%v): %v", test.name, err)
		}
		if diff := cmp.Diff(test.want, test.pb, protocmp.Transform()); diff != "" {
			t.Errorf("RemoveExtension(%v) returned unexpected diff: (-want, +got) %v", test.name, diff)
		}
	}

}

func TestExtensionFieldName(t *testing.T) {
	tests := []struct {
		url, want string
	}{
		{
			"http://example.org/ext1",
			"ext1",
		},
		{
			"http://example.org/ext-1",
			"ext_1",
		},
		{
			"1",
			"_1",
		},
		{
			"http://example.org/ext1/",
			"ext1",
		},
		{
			"http://example.org//",
			"",
		},
		{
			"http://example.com/" + strings.Repeat("a", 301),
			strings.Repeat("a", 300),
		},
	}
	for _, test := range tests {
		if got := ExtensionFieldName(test.url); got != test.want {
			t.Errorf("ExtensionFieldName(%q): got %v, want %v", test.url, got, test.want)
		}
	}
}

func TestFullExtensionFieldName(t *testing.T) {
	tests := []struct {
		url, want string
	}{
		{
			"http://example.org/ext1",
			"example_org_ext1",
		},
		{
			"https://example.org/ext1",
			"example_org_ext1",
		},
		{
			"http://example.org?f1=1&f2=2",
			"example_org_f1_1_f2_2",
		},
		{
			"http://example.org/ext-1",
			"example_org_ext_1",
		},
		{
			"1",
			"_1",
		},
		{
			"http://example.com/" + strings.Repeat("a", 300),
			"example_com_" + strings.Repeat("a", 288),
		},
	}
	for _, test := range tests {
		if got := FullExtensionFieldName(test.url); got != test.want {
			t.Errorf("FullExtensionFieldName(%q): got %v, want %v", test.url, got, test.want)
		}
	}
}

func regexForType(msg proto.Message) *regexp.Regexp {
	return RegexValues[msg.ProtoReflect().Descriptor().FullName()]
}

func TestValidateOID(t *testing.T) {
	tests := []string{
		"urn:oid:1.2.3.4",
		"urn:oid:1.100",
	}
	for _, typ := range []proto.Message{&d4pb.Oid{}, &d5pb.Oid{}} {
		for _, test := range tests {
			t.Run(fmt.Sprintf("%T", typ), func(t *testing.T) {
				if ok := regexForType(typ).MatchString(test); !ok {
					t.Errorf("OID regex: expected %q to be valid", test)
				}
			})
		}
	}
}

func TestValidateOID_Invalid(t *testing.T) {
	tests := []string{
		"urn:oid:abc",
		"urn:oid:999",
		"urn:oid:01",
	}
	for _, typ := range []proto.Message{&d4pb.Oid{}, &d5pb.Oid{}} {
		for _, test := range tests {
			t.Run(fmt.Sprintf("%T", typ), func(t *testing.T) {
				if ok := regexForType(typ).MatchString(test); ok {
					t.Errorf("OID regex: expected %q to be invalid", test)
				}
			})
		}
	}
}

func TestValidateID(t *testing.T) {
	tests := []string{
		"123-456",
		"abc",
		"DEF",
		"1.2.3.4",
		// 64 characters, meets length limit.
		"1234567890123456789012345678901234567890123456789012345678901234",
	}
	for _, typ := range []proto.Message{&d4pb.Id{}, &d5pb.Id{}} {
		for _, test := range tests {
			t.Run(fmt.Sprintf("%T", typ), func(t *testing.T) {
				if ok := regexForType(typ).MatchString(test); !ok {
					t.Errorf("ID regex: expected %q to be valid", test)
				}
			})
		}
	}
}

func TestValidateID_Invalid(t *testing.T) {
	tests := []string{
		"abc\x00def",
		"123|456",
		// 65 characters, exceeds length limit.
		"12345678901234567890123456789012345678901234567890123456789012345",
		"",
	}
	for _, typ := range []proto.Message{&d4pb.Id{}, &d5pb.Id{}} {
		for _, test := range tests {
			t.Run(fmt.Sprintf("%T", typ), func(t *testing.T) {
				if ok := regexForType(typ).MatchString(test); ok {
					t.Errorf("ID regex: expected %q to be invalid", test)
				}
			})
		}
	}
}

func TestValidateCode(t *testing.T) {
	tests := []string{
		"abc",
		"123",
		"http://example.com/code",
		"a value",
	}
	for _, typ := range []proto.Message{&d4pb.Code{}, &d5pb.Code{}} {
		for _, test := range tests {
			t.Run(fmt.Sprintf("%T", typ), func(t *testing.T) {
				if ok := regexForType(typ).MatchString(test); !ok {
					t.Errorf("Code regex: expected %q to be valid", test)
				}
			})
		}
	}
}

func TestValidateCode_Invalid(t *testing.T) {
	tests := []string{
		"    abc",
		"\tabc",
		"abc     ",
		"",
		"bad  value",
	}
	for _, typ := range []proto.Message{&d4pb.Code{}, &d5pb.Code{}} {
		for _, test := range tests {
			t.Run(fmt.Sprintf("%T", typ), func(t *testing.T) {
				if ok := regexForType(typ).MatchString(test); ok {
					t.Errorf("Code regex: expected %q to be invalid", test)
				}
			})
		}
	}
}

func TestValidateUnsignedInt(t *testing.T) {
	tests := []string{
		"0",
		"1",
		"123",
	}
	for _, test := range tests {
		if ok := UnsignedIntCompiledRegex.MatchString(test); !ok {
			t.Errorf("UnsignedInt regex: expected %q to be valid", test)
		}
	}
}

func TestValidateUnsignedInt_Invalid(t *testing.T) {
	tests := []string{
		"1.2",
		"01",
		"1e3",
		"-1",
		"+1",
	}
	for _, test := range tests {
		if ok := UnsignedIntCompiledRegex.MatchString(test); ok {
			t.Errorf("UnsignedInt regex: expected %q to be invalid", test)
		}
	}
}

func TestValidatePositiveInt(t *testing.T) {
	tests := []string{
		"1",
		"+1",
		"123",
	}
	for _, test := range tests {
		if ok := PositiveIntCompiledRegex.MatchString(test); !ok {
			t.Errorf("PositiveInt regex: expected %q to be valid", test)
		}
	}
}

func TestValidatePositiveInt_Invalid(t *testing.T) {
	tests := []string{
		"0",
		"1.2",
		"01",
		"1e3",
		"-1",
	}
	for _, test := range tests {
		if ok := PositiveIntCompiledRegex.MatchString(test); ok {
			t.Errorf("PositiveInt regex: expected %q to be invalid", test)
		}
	}
}

func TestTime(t *testing.T) {
	tests := []struct {
		json string
		time Time
	}{
		{
			"12:00:00",
			Time{
				ValueUs:   43200000000,
				Precision: PrecisionSecond,
			},
		},
		{
			"12:00:00.000",
			Time{
				ValueUs:   43200000000,
				Precision: PrecisionMillisecond,
			},
		},
		{
			"12:00:00.000000",
			Time{
				ValueUs:   43200000000,
				Precision: PrecisionMicrosecond,
			},
		},
	}
	// Test time parsing
	for _, test := range tests {
		got, err := parseTimeToGoTime([]byte(strconv.Quote(test.json)))
		if err != nil {
			t.Fatalf("parseTimeToGoTime(%q): %v", test.json, err)
		}
		if want := test.time; !cmp.Equal(want, got) {
			t.Errorf("parseTimeToGoTime(%q): got %v, want %v", test.json, got, want)
		}
	}
	// Test time serializing
	for _, test := range tests {
		got, err := serializeTime(test.time.ValueUs, test.time.Precision)
		if err != nil {
			t.Fatalf("serializeTime(%q): %v", test.json, err)
		}
		if want := test.json; want != got {
			t.Errorf("serializeTime(%q): got %q, want %q", test.json, got, want)
		}
	}
}

func TestParseTime_Invalid(t *testing.T) {
	tests := []struct {
		time json.RawMessage
	}{
		{
			json.RawMessage(`"24:00:00"`),
		},
		{
			json.RawMessage(`"12:60:61"`),
		},
		{
			json.RawMessage(`"12:00"`),
		},
	}
	for _, test := range tests {
		if _, err := parseTimeToGoTime(test.time); err == nil {
			t.Errorf("parseTimeToGoTime(%q) succeeded, want error", string(test.time))
		}
	}
}

func TestSerializeTime_Invalid(t *testing.T) {
	time := Time{ValueUs: 43200000000, Precision: PrecisionUnspecified}
	if _, err := serializeTime(time.ValueUs, time.Precision); err == nil {
		t.Errorf("ParseTime(%v) succeeded, want error", t)
	}
}

func TestValidateReferenceType(t *testing.T) {
	tests := []struct {
		name      string
		msg       proto.Message
		fieldName string
		ref       proto.Message
		valid     bool
	}{
		{
			"stu3 valid reference",
			&r3pb.Patient{},
			"managing_organization",
			&d3pb.Reference{
				Reference: &d3pb.Reference_OrganizationId{
					OrganizationId: &d3pb.ReferenceId{Value: "1"},
				},
			},
			true,
		},
		{
			"stu3 URI reference",
			&r3pb.Patient{},
			"managing_organization",
			&d3pb.Reference{
				Reference: &d3pb.Reference_Uri{
					Uri: &d3pb.String{Value: "Patient/1"},
				},
			},
			true,
		},
		{
			"stu3 reference of any type",
			&r3pb.Basic{},
			"subject",
			&d3pb.Reference{
				Reference: &d3pb.Reference_AccountId{
					AccountId: &d3pb.ReferenceId{Value: "1"},
				},
			},
			true,
		},
		{
			"stu3 invalid reference",
			&r3pb.Patient{},
			"managing_organization",
			&d3pb.Reference{
				Reference: &d3pb.Reference_PatientId{
					PatientId: &d3pb.ReferenceId{Value: "1"},
				},
			},
			false,
		},
		{
			"r4 valid reference",
			&r4patientpb.Patient{},
			"managing_organization",
			&d4pb.Reference{
				Reference: &d4pb.Reference_OrganizationId{
					OrganizationId: &d4pb.ReferenceId{Value: "1"},
				},
			},
			true,
		},
		{
			"r4 URI reference",
			&r4patientpb.Patient{},
			"managing_organization",
			&d4pb.Reference{
				Reference: &d4pb.Reference_Uri{
					Uri: &d4pb.String{Value: "Patient/1"},
				},
			},
			true,
		},
		{
			"r4 fragment reference",
			&r4patientpb.Patient{},
			"managing_organization",
			&d4pb.Reference{
				Reference: &d4pb.Reference_Fragment{
					Fragment: &d4pb.String{Value: "#1"},
				},
			},
			true,
		},
		{
			"r4 reference of any type",
			&r4basicpb.Basic{},
			"subject",
			&d4pb.Reference{
				Reference: &d4pb.Reference_AccountId{
					AccountId: &d4pb.ReferenceId{Value: "1"},
				},
			},
			true,
		},
		{
			"r4 invalid reference",
			&r4patientpb.Patient{},
			"managing_organization",
			&d4pb.Reference{
				Reference: &d4pb.Reference_PatientId{
					PatientId: &d4pb.ReferenceId{Value: "1"},
				},
			},
			false,
		},
		{
			"r5 valid reference",
			&r5patientpb.Patient{},
			"managing_organization",
			&d5pb.Reference{
				Reference: &d5pb.Reference_OrganizationId{
					OrganizationId: &d5pb.ReferenceId{Value: "1"},
				},
			},
			true,
		},
		{
			"r5 URI reference",
			&r5patientpb.Patient{},
			"managing_organization",
			&d5pb.Reference{
				Reference: &d5pb.Reference_Uri{
					Uri: &d5pb.String{Value: "Patient/1"},
				},
			},
			true,
		},
		{
			"r5 fragment reference",
			&r5patientpb.Patient{},
			"managing_organization",
			&d5pb.Reference{
				Reference: &d5pb.Reference_Fragment{
					Fragment: &d5pb.String{Value: "#1"},
				},
			},
			true,
		},
		{
			"r5 reference of any type",
			&r5basicpb.Basic{},
			"subject",
			&d5pb.Reference{
				Reference: &d5pb.Reference_AccountId{
					AccountId: &d5pb.ReferenceId{Value: "1"},
				},
			},
			true,
		},
		{
			"r5 invalid reference",
			&r5patientpb.Patient{},
			"managing_organization",
			&d5pb.Reference{
				Reference: &d5pb.Reference_PatientId{
					PatientId: &d5pb.ReferenceId{Value: "1"},
				},
			},
			false,
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			msg := test.msg.ProtoReflect()
			field := msg.Descriptor().Fields().ByName(protoreflect.Name(test.fieldName))
			ref := test.ref.ProtoReflect()

			err := ValidateReferenceMessageType(field, ref)
			valid := err == nil
			if test.valid != valid {
				t.Fatalf("ValidateReferenceType on field %s of %+v returned %v, want %v", test.fieldName, test.msg, valid, test.valid)
			}
		})
	}
}

func TestValidateRequiredFields(t *testing.T) {
	tests := []struct {
		name  string
		msg   proto.Message
		valid bool
	}{
		{
			name: "proto has all required fields",
			msg: &r3pb.Patient_Link{
				Other: &d3pb.Reference{
					Reference: &d3pb.Reference_Uri{
						Uri: &d3pb.String{Value: "Patient/2"},
					},
				},
				Type: &c3pb.LinkTypeCode{Value: c3pb.LinkTypeCode_REPLACED_BY},
			},
			valid: true,
		},
		{
			name: "proto missing required",
			msg: &r3pb.Patient_Link{
				Other: &d3pb.Reference{
					Reference: &d3pb.Reference_Uri{
						Uri: &d3pb.String{Value: "Patient/2"},
					},
				},
			},
			valid: false,
		},
		{
			name:  "proto missing required repeated",
			msg:   &r3pb.OperationOutcome{},
			valid: false,
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			msg := test.msg.ProtoReflect()
			err := ValidateRequiredFields(msg, false /* disallowNullRequired */)
			valid := err == nil
			if valid != test.valid {
				t.Errorf("ValidateRequiredFields(%v): got %v, want %v", test.name, valid, test.valid)
			}
		})
	}
}

func TestPrintUnmarshalError(t *testing.T) {
	tests := []struct {
		name        string
		err         error
		wantMessage string
	}{
		{
			name: "full error",
			err: &UnmarshalError{
				Path:        "Patient.name",
				Details:     "invalid type",
				Diagnostics: "want array",
			},
			wantMessage: "at Patient.name: invalid type: want array",
		},
		{
			name: "no diagnostics",
			err: &UnmarshalError{
				Path:    "Patient.name",
				Details: "invalid type",
			},
			wantMessage: "at Patient.name: invalid type",
		},
		{
			name: "no path",
			err: &UnmarshalError{
				Details:     "invalid type",
				Diagnostics: "want array",
			},
			wantMessage: "invalid type: want array",
		},
		{
			name: "error list",
			err: UnmarshalErrorList{
				{
					Path:    "Patient.name[0]",
					Details: "error1",
				},
				{
					Path:    "Patient.name[1]",
					Details: "error2",
				},
			},
			wantMessage: "at Patient.name[0]: error1\nat Patient.name[1]: error2",
		},
		{
			name:        "other error",
			err:         errors.New("other error"),
			wantMessage: "other error",
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			got := PrintUnmarshalError(test.err, -1)
			if !cmp.Equal(test.wantMessage, got) {
				t.Errorf("PrintUnmarshalError(%v, -1) got message %s, want %s", test.err, got, test.wantMessage)
			}
		})
	}
}

func TestPrintUnmarshalError_Limit(t *testing.T) {
	err := UnmarshalErrorList{
		{Details: "error1"},
		{Details: "error2"},
		{Details: "error3"},
	}

	wantMessage := "error1\nerror2\nand 1 other issue(s)"
	got := PrintUnmarshalError(err, 2)
	if !cmp.Equal(wantMessage, got) {
		t.Errorf("PrintUnmarshalError(%v, 2) got message %s, want %s", err, got, wantMessage)
	}
}
