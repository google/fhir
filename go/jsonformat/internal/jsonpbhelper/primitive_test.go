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
	"strconv"
	"testing"

	"github.com/google/fhir/go/fhirversion"
	"github.com/google/fhir/go/jsonformat/internal/accessor"
	"github.com/google/go-cmp/cmp"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/reflect/protoreflect"
	"google.golang.org/protobuf/testing/protocmp"

	c4pb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/codes_go_proto"
	d4pb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/datatypes_go_proto"
	rs4pb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/research_study_go_proto"
	e4pb "github.com/google/fhir/go/proto/google/fhir/proto/r4/fhirproto_extensions_go_proto"
	c3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/codes_go_proto"
	d3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/datatypes_go_proto"
	e3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/fhirproto_extensions_go_proto"
)

func appendSeparatorAndStrideExtensions(extListOwner protoreflect.Message, sep string, stride uint32) {
	extList, _ := accessor.GetList(extListOwner, "extension")
	first := extList.NewElement().Message()
	_ = accessor.SetValue(first, "separator", "url", "value")
	_ = accessor.SetValue(first, sep, "value", "choice", "string_value", "value")
	_ = accessor.AppendValue(extListOwner, first.Interface().(proto.Message), "extension")
	second := extList.NewElement().Message()
	_ = accessor.SetValue(second, "stride", "url", "value")
	_ = accessor.SetValue(second, stride, "value", "choice", "positive_int", "value")
	_ = accessor.AppendValue(extListOwner, second.Interface().(proto.Message), "extension")
}

func addExtensionWithNestingSeparatorAndStrideExtensions(masterExtListOwner protoreflect.Message, sep string, stride uint32) {
	masterList, _ := accessor.GetList(masterExtListOwner, "extension")
	masterExt := masterList.NewElement().Message()
	_ = accessor.SetValue(masterExt, "https://g.co/fhir/StructureDefinition/base64Binary-separatorStride", "url", "value")
	appendSeparatorAndStrideExtensions(masterExt, sep, stride)
	masterList.Append(protoreflect.ValueOf(masterExt))
}

type protoToExtensionTestData struct {
	srcProto     proto.Message
	dstExtension proto.Message
}

// newBinarySeparatorStrideAndExtensionTestData generates test data across all FHIR versions for
// TestProtoToExtension.
//
// The srcProto will be created as following:
//
//	epb.Base64BinarySeparatorStride{
//		Separator: &dpb.String{
//			Value: <sep>,
//		},
//		Stride: &dpb.PositiveInt{
//			Value: <stride>,
//		},
//	}
//
// The dstExtension will be created as following:
//
//	&dpb.Extension{
//		Url: &dpb.Uri{
//			Value: "https://g.co/fhir/StructureDefinition/base64Binary-separatorStride",
//		},
//		Extension: []*dpb.Extension{{
//			Url: &dpb.Uri{
//				Value: "separator",
//			},
//			Value: &dpb.Extension_ValueX{
//				Choice: &dpb.Extension_ValueX_StringValue{
//					StringValue: &dpb.String{
//						Value: <sep>,
//					},
//				},
//			},
//		}, {
//			Url: &dpb.Uri{
//				Value: "stride",
//			},
//			Value: &dpb.Extension_ValueX{
//				Choice: &dpb.Extension_ValueX_PositiveInt{
//					PositiveInt: &dpb.PositiveInt{
//						Value: <stride>,
//					},
//				},
//			},
//		}},
//	},
func newBinarySeparatorStrideAndExtensionTestData(sep string, stride uint32) []protoToExtensionTestData {
	var ret []protoToExtensionTestData
	ret = append(ret,
		protoToExtensionTestData{
			srcProto:     &e3pb.Base64BinarySeparatorStride{},
			dstExtension: &d3pb.Extension{},
		},
		protoToExtensionTestData{
			srcProto:     &e4pb.Base64BinarySeparatorStride{},
			dstExtension: &d4pb.Extension{},
		},
	)
	for _, r := range ret {
		rSrc := r.srcProto.ProtoReflect()
		rDst := r.dstExtension.ProtoReflect()
		_ = accessor.SetValue(rSrc, sep, "separator", "value")
		_ = accessor.SetValue(rSrc, stride, "stride", "value")
		_ = accessor.SetValue(rDst, "https://g.co/fhir/StructureDefinition/base64Binary-separatorStride", "url", "value")
		appendSeparatorAndStrideExtensions(rDst, sep, stride)
	}
	return ret
}

// newPrimitiveHasNoValueTestData generates test data across all FHIR versions for
// TestProtoToExtension.
//
// The srcProto will be created as following:
//
//	&epb.PrimitiveHasNoValue{
//		ValueBoolean: &dpb.Boolean{
//			Value: true,
//		},
//	}
//
// The dstExtension will be created as following:
//
//	&dpb.Extension{
//		Url: &dpb.Uri{
//			Value: "https://g.co/fhir/StructureDefinition/primitiveHasNoValue",
//		},
//		Value: &dpb.Extension_ValueX{
//			Choice: &dpb.Extension_ValueX_Boolean{
//				Boolean: &dpb.Boolean{
//					Value: <b>,
//				},
//			},
//		},
//	}
func newPrimitiveHasNoValueTestData(b bool) []protoToExtensionTestData {
	var ret []protoToExtensionTestData
	ret = append(ret,
		protoToExtensionTestData{
			srcProto:     &e3pb.PrimitiveHasNoValue{},
			dstExtension: &d3pb.Extension{},
		},
	)
	for _, r := range ret {
		rSrc := r.srcProto.ProtoReflect()
		rDst := r.dstExtension.ProtoReflect()
		_ = accessor.SetValue(rSrc, b, "value_boolean", "value")
		_ = accessor.SetValue(rDst, "https://g.co/fhir/StructureDefinition/primitiveHasNoValue", "url", "value")
		_ = accessor.SetValue(rDst, b, "value", "choice", "boolean", "value")
	}
	return ret
}

func TestProtoExtension(t *testing.T) {
	tests := []struct {
		name string
		data []protoToExtensionTestData
	}{
		{
			"Base64BinarySeparatorStride",
			newBinarySeparatorStrideAndExtensionTestData("   ", 3),
		},
		{
			"PrimitiveHasNoValue",
			newPrimitiveHasNoValueTestData(true),
		},
	}
	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			for _, d := range tc.data {
				got := d.dstExtension.ProtoReflect().New().Interface().(proto.Message)
				if err := ProtoToExtension(d.srcProto, got); err != nil {
					t.Fatalf("ProtoToExtension(%v, %T): %v", d.srcProto, got, err)
				}
				if diff := cmp.Diff(d.dstExtension, got, protocmp.Transform()); diff != "" {
					t.Errorf("ProtoToExtension(%v, %T) returned unexpected diff: (-want, +got) %v", d.srcProto, got, diff)
				}
			}
		})
	}
}

type addInternalExtensionTestData struct {
	protoToExtensionTestData
	srcBinary proto.Message
	dstBinary proto.Message
}

// newAddInternalextensionTestData generates test data for TestAddInternalExtension
//
// If alreadydExist is false, the srcBinary will be created as following:
//
//	&dpb.Base64Binary{
//		Value: <bin>,
//	}
//
// # If alreadyExist is true, the srcBinary will be the same as dstBinary
//
// The dstBinary will be created as following:
//
//	&dpb.Base64Binary{
//		Value: <bin>,
//		Extension: []*dpb.Extension{{
//			Url: &dpb.Uri{
//				Value: "https://g.co/fhir/StructureDefinition/base64Binary-separatorStride",
//			},
//			Extension: []*dpb.Extension{{
//				Url: &dpb.Uri{
//					Value: "separator",
//				},
//				Value: &dpb.Extension_ValueX{
//					Choice: &dpb.Extension_ValueX_StringValue{
//						StringValue: &dpb.String{
//							Value: <sep>,
//						},
//					},
//				},
//			}, {
//				Url: &dpb.Uri{
//					Value: "stride",
//				},
//				Value: &dpb.Extension_ValueX{
//					Choice: &dpb.Extension_ValueX_PositiveInt{
//						PositiveInt: &dpb.PositiveInt{
//							Value: <stride>,
//						},
//					},
//				},
//			}},
//		}},
//	}
func newAddInternalExtensionTestData(bin []byte, sep string, stride uint32, alreadyExist bool) []addInternalExtensionTestData {
	var ret []addInternalExtensionTestData
	ret = append(ret,
		addInternalExtensionTestData{
			srcBinary: &d3pb.Base64Binary{Value: bin},
			dstBinary: &d3pb.Base64Binary{Value: bin},
		},
		addInternalExtensionTestData{
			srcBinary: &d4pb.Base64Binary{Value: bin},
			dstBinary: &d4pb.Base64Binary{Value: bin},
		},
	)
	protoToExtData := newBinarySeparatorStrideAndExtensionTestData(sep, stride)
	for i, r := range ret {
		addExtensionWithNestingSeparatorAndStrideExtensions(r.dstBinary.ProtoReflect(), sep, stride)
		if alreadyExist {
			addExtensionWithNestingSeparatorAndStrideExtensions(r.srcBinary.ProtoReflect(), sep, stride)
		}
		ret[i].protoToExtensionTestData = protoToExtData[i]
	}
	return ret
}

func TestAddInternalExtension(t *testing.T) {
	tests := []struct {
		name                   string
		bin                    []byte
		extensionsAlreadyInBin bool
		sep                    string
		stride                 uint32
	}{
		{
			name:                   "extension appended",
			bin:                    []byte("val"),
			extensionsAlreadyInBin: false,
			sep:                    "  ",
			stride:                 2,
		},
		{
			name:                   "already exists",
			bin:                    []byte("val"),
			extensionsAlreadyInBin: true,
			sep:                    "  ",
			stride:                 2,
		},
	}
	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			data := newAddInternalExtensionTestData(tc.bin, tc.sep, tc.stride, tc.extensionsAlreadyInBin)
			for _, d := range data {
				ext := d.dstExtension.ProtoReflect().New().Interface().(proto.Message)
				if err := ProtoToExtension(d.srcProto, ext); err != nil {
					t.Errorf("ProtoToExtension(%v, %T) got error: %v", d.srcProto, ext, err)
				}
				origSrcBin := proto.Clone(d.srcBinary)
				if err := AddInternalExtension(d.srcBinary, ext); err != nil {
					t.Errorf("AddInternalExtension(%v, %v) got error: %v", origSrcBin, ext, err)
				}
				if diff := cmp.Diff(d.dstBinary, d.srcBinary, protocmp.Transform()); diff != "" {
					t.Errorf("AddInternalExtension(%v, %v) returned unexpected diff: (-want, +got) %v", origSrcBin, ext, diff)
				}
			}
		})
	}
}

func TestDecimal(t *testing.T) {
	allVers := []fhirversion.Version{fhirversion.STU3, fhirversion.R4}
	tests := []struct {
		value string
		vers  []fhirversion.Version
	}{
		{"123.45", allVers},
		{"185", allVers},
		{"-40", allVers},
		{"0.0099", allVers},
		{"100", allVers},
		{"100.00", allVers},
		{"0", allVers},
		{"0.00", allVers},
		{"66.899999999999991", allVers},
		{"4e2", []fhirversion.Version{fhirversion.R4}},
		{"-2E8", []fhirversion.Version{fhirversion.R4}},
	}
	for _, test := range tests {
		t.Run(test.value, func(t *testing.T) {
			expects := map[fhirversion.Version]proto.Message{
				fhirversion.STU3:  &d3pb.Decimal{Value: test.value},
				fhirversion.R4:    &d4pb.Decimal{Value: test.value},
			}
			for _, ver := range test.vers {
				e := expects[ver]
				got := e.ProtoReflect().New().Interface().(proto.Message)
				if err := ParseDecimal(json.RawMessage(test.value), got); err != nil {
					t.Fatalf("ParseDecimal(%q, %T) got error: %v", test, got, err)
				}
				if !cmp.Equal(e, got, protocmp.Transform()) {
					t.Errorf("ParseDecimal(%q, %T): got %v, want: %v", test, got, got, e)
				}
			}
		})
	}
}

func TestDecimal_Invalid(t *testing.T) {
	allVers := []fhirversion.Version{fhirversion.STU3, fhirversion.R4}
	tests := []struct {
		name string
		json string
		vers []fhirversion.Version
	}{
		{
			"invalid characters",
			"123abc",
			allVers,
		},
		{
			"exponent",
			"1e3",
			[]fhirversion.Version{fhirversion.STU3},
		},
		{
			"leading 0",
			"000123",
			allVers,
		},
	}
	for _, test := range tests {
		msgs := map[fhirversion.Version]proto.Message{
			fhirversion.STU3:  &d3pb.Decimal{},
			fhirversion.R4:    &d4pb.Decimal{},
		}
		for _, ver := range test.vers {
			if err := ParseDecimal(json.RawMessage(test.json), msgs[ver]); err == nil {
				t.Errorf("ParseDecimal(%q, %T) succeeded, want error", test.name, msgs[ver])
			}
		}
	}
}

func TestBinary(t *testing.T) {
	tests := []struct {
		name          string
		json          string
		binVal        []byte
		hasExtensions bool
		sep           string
		stride        uint32
	}{
		{
			name:          "continuous case",
			json:          "Y29udGludW91cyBjYXNl",
			binVal:        []byte("continuous case"),
			hasExtensions: false,
		},
		{
			name:          "split case",
			json:          "c3Bsa   XQgY2   FzZQ=   =",
			binVal:        []byte("split case"),
			hasExtensions: true,
			sep:           "   ",
			stride:        5,
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			typesAndCreators := []struct {
				message proto.Message
				creator base64BinarySeparatorStrideCreator
			}{
				{
					&d3pb.Base64Binary{},
					func(sep string, stride uint32) proto.Message {
						return &e3pb.Base64BinarySeparatorStride{
							Separator: &d3pb.String{Value: sep},
							Stride:    &d3pb.PositiveInt{Value: stride},
						}
					},
				},
				{
					&d4pb.Base64Binary{},
					func(sep string, stride uint32) proto.Message {
						return &e4pb.Base64BinarySeparatorStride{
							Separator: &d4pb.String{Value: sep},
							Stride:    &d4pb.PositiveInt{Value: stride},
						}
					},
				},
			}
			for _, tnc := range typesAndCreators {
				// Test parsing
				parsed := tnc.message.ProtoReflect().New().Interface().(proto.Message)
				if err := ParseBinary(json.RawMessage(strconv.Quote(test.json)), parsed, tnc.creator); err != nil {
					t.Fatalf("ParseBinary(%q, %T, %v) got error: %v", test.json, parsed, tnc.creator, err)
				}
				binary := tnc.message.ProtoReflect().New()
				_ = accessor.SetValue(binary, test.binVal, "value")
				if test.hasExtensions {
					addExtensionWithNestingSeparatorAndStrideExtensions(binary, test.sep, test.stride)
				}
				if want := binary.Interface().(proto.Message); !cmp.Equal(want, parsed, protocmp.Transform()) {
					t.Errorf("ParseBinary(%q, %T, %v) got %v, want: %v", test.json, binary.Interface(), tnc.creator, parsed, want)
				}
				// Test serialization
				serialized, err := serializeBinary(binary.Interface().(proto.Message))
				if err != nil {
					t.Fatalf("SerializeBinary(%q, %v) got error: %v", binary.Interface().(proto.Message), tnc.creator, err)
				}
				if want := test.json; want != serialized {
					t.Errorf("SerializeBinary(%q, %v): got %q, want: %q",
						binary.Interface().(proto.Message), tnc.creator, serialized, want)
				}
			}
		})
	}
}

func TestParseBinary_Invalid(t *testing.T) {
	tests := []struct {
		name string
		json string
	}{
		{
			"invalid base64 character",
			"&",
		},
		{
			"invalid JSON character",
			"\x00",
		},
		{
			"bad padding",
			"c3BsaXQgY2FzZQ=",
		},
		{
			"bad spacing",
			"c3Bs  aXQg Y2FzZQ==",
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			typesAndCreators := []struct {
				message proto.Message
				creator base64BinarySeparatorStrideCreator
			}{
				{
					&d3pb.Base64Binary{},
					func(sep string, stride uint32) proto.Message {
						return &e3pb.Base64BinarySeparatorStride{
							Separator: &d3pb.String{Value: sep},
							Stride:    &d3pb.PositiveInt{Value: stride},
						}
					},
				},
				{
					&d4pb.Base64Binary{},
					func(sep string, stride uint32) proto.Message {
						return &e4pb.Base64BinarySeparatorStride{
							Separator: &d4pb.String{Value: sep},
							Stride:    &d4pb.PositiveInt{Value: stride},
						}
					},
				},
			}
			for _, tnc := range typesAndCreators {
				m := tnc.message.ProtoReflect().New().Interface().(proto.Message)
				if err := ParseBinary(json.RawMessage(strconv.Quote(test.json)), m, tnc.creator); err == nil {
					t.Errorf("ParseBinary(%q, %T, %v) succeeded, want error", test.json, m, tnc.creator)
				}
			}
		})
	}
}

type mvr struct {
	ver fhirversion.Version
	r   proto.Message
}

func TestMarshalPrimitiveType(t *testing.T) {
	tests := []struct {
		name   string
		inputs []mvr
		want   IsJSON
	}{
		{
			name: "Boolean",
			inputs: []mvr{
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
			},
			want: JSONRawValue("true"),
		},
		{
			name: "Integer",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
					r: &d3pb.Integer{
						Value: 1,
					},
				},
				{
					ver: fhirversion.R4,
					r: &d4pb.Integer{
						Value: 1,
					},
				},
			},
			want: JSONRawValue("1"),
		},
		{
			name: "Canonical",
			inputs: []mvr{
				{
					ver: fhirversion.R4,
					r: &d4pb.Canonical{
						Value: "c",
					},
				},
			},
			want: JSONString("c"),
		},
		{
			name: "Code",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
					r: &d3pb.Code{
						Value: "some code",
					},
				},
				{
					ver: fhirversion.R4,
					r: &d4pb.Code{
						Value: "some code",
					},
				},
			},
			want: JSONString("some code"),
		},
		{
			name: "Id",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
					r: &d3pb.Id{
						Value: "patient1234",
					},
				},
				{
					ver: fhirversion.R4,
					r: &d4pb.Id{
						Value: "patient1234",
					},
				},
			},
			want: JSONString("patient1234"),
		},
		{
			name: "String",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
					r: &d3pb.String{
						Value: "This is a string",
					},
				},
				{
					ver: fhirversion.R4,
					r: &d4pb.String{
						Value: "This is a string",
					},
				},
			},
			want: JSONString("This is a string"),
		},
		{
			name: "Markdown",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
					r: &d3pb.Markdown{
						Value: "md",
					},
				},
				{
					ver: fhirversion.R4,
					r: &d4pb.Markdown{
						Value: "md",
					},
				},
			},
			want: JSONString("md"),
		},
		{
			name: "Url",
			inputs: []mvr{
				{
					ver: fhirversion.R4,
					r: &d4pb.Url{
						Value: "u",
					},
				},
			},
			want: JSONString("u"),
		},
		{
			name: "Uuid",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
					r: &d3pb.Uuid{
						Value: "uuid",
					},
				},
				{
					ver: fhirversion.R4,
					r: &d4pb.Uuid{
						Value: "uuid",
					},
				},
			},
			want: JSONString("uuid"),
		},
		{
			name: "ResearchStudyStatusCode",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
					r: &c3pb.ResearchStudyStatusCode{
						Value: c3pb.ResearchStudyStatusCode_COMPLETED,
					},
				},
				{
					ver: fhirversion.R4,
					r: &rs4pb.ResearchStudy_StatusCode{
						Value: c4pb.ResearchStudyStatusCode_COMPLETED,
					},
				},
			},
			want: JSONString("completed"),
		},
		{
			name: "ResearchStudyStatusCode uninitialized",
			inputs: []mvr{
				{
					ver: fhirversion.STU3,
					r: &c3pb.ResearchStudyStatusCode{
						Value: c3pb.ResearchStudyStatusCode_INVALID_UNINITIALIZED,
					},
				},
				{
					ver: fhirversion.R4,
					r: &rs4pb.ResearchStudy_StatusCode{
						Value: c4pb.ResearchStudyStatusCode_INVALID_UNINITIALIZED,
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
					got, err := MarshalPrimitiveType(i.r.ProtoReflect())
					if err != nil {
						t.Fatalf("marshalPrimitiveType(%v): %v", test.name, err)
					}
					if diff := cmp.Diff(got, test.want); diff != "" {
						t.Errorf("found diff for marshalPrimitiveType(%v): got %s, want %s, diff: %s", test.name, got, test.want, diff)
					}
				})
			}
		})
	}
}
