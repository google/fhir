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

package accessor

import (
	"bytes"
	"reflect"
	"strings"
	"testing"

	"github.com/google/go-cmp/cmp"
	"github.com/golang/protobuf/proto"
	"google.golang.org/protobuf/testing/protocmp"

	atpb "google/fhir/jsonformat/internal/accessor/accessor_test_go_proto"
	d3pb "google/fhir/proto/stu3/datatypes_go_proto"
	r3pb "google/fhir/proto/stu3/resources_go_proto"
)

func TestGetMessage(t *testing.T) {
	tests := []struct {
		name   string
		pb     proto.Message
		path   []string
		expect proto.Message
	}{
		{
			name:   "get one level nesting message",
			pb:     &r3pb.Patient{Id: &d3pb.Id{Value: "patient_id"}},
			path:   []string{"id"},
			expect: &d3pb.Id{Value: "patient_id"},
		},
		{
			name:   "get multiple level nesting message",
			pb:     &r3pb.Patient{Meta: &d3pb.Meta{Id: &d3pb.String{Value: "meta_id"}}},
			path:   []string{"meta", "id"},
			expect: &d3pb.String{Value: "meta_id"},
		},
		{
			name:   "get oneof field with oneof field name",
			pb:     &d3pb.Reference{Reference: &d3pb.Reference_Uri{Uri: &d3pb.String{Value: "reference_uri"}}},
			path:   []string{"reference", "uri"},
			expect: &d3pb.String{Value: "reference_uri"},
		},
		{
			name:   "get oneof field withouth oneof field name",
			pb:     &d3pb.Reference{Reference: &d3pb.Reference_Uri{Uri: &d3pb.String{Value: "reference_uri"}}},
			path:   []string{"uri"},
			expect: &d3pb.String{Value: "reference_uri"},
		},
	}
	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			rpb := proto.MessageReflect(tc.pb)
			rgot, err := GetMessage(rpb, tc.path...)
			if err != nil {
				t.Errorf("GetMessage(%v, %v) returned error: %v", tc.pb, tc.path, err)
			}
			got := rgot.Interface().(proto.Message)
			if diff := cmp.Diff(tc.expect, got, protocmp.Transform()); diff != "" {
				t.Errorf("GetMessage(%v, %v) returned unexpected diff: (-want, +got) %v", tc.pb, tc.path, diff)
			}
		})
	}
}

func TestGetMessage_Invalid(t *testing.T) {
	tests := []struct {
		name          string
		pb            proto.Message
		path          []string
		expectInError string
	}{
		{
			name:          "get unpopulated field",
			pb:            &r3pb.Patient{},
			path:          []string{"id"},
			expectInError: "field: id in google.fhir.stu3.proto.Patient is not populated",
		},
		{
			name:          "get through unpopulated field",
			pb:            &r3pb.Patient{},
			path:          []string{"meta", "id"},
			expectInError: "field: meta in google.fhir.stu3.proto.Patient is not populated",
		},
		{
			name: "get repeated field",
			pb: &d3pb.Reference{Extension: []*d3pb.Extension{
				{Id: &d3pb.String{Value: "extension_1"}},
			}},
			path:          []string{"extension"},
			expectInError: "field: extension in google.fhir.stu3.proto.Reference is repeated",
		},
		{
			name: "get through repeated field",
			pb: &d3pb.Reference{Extension: []*d3pb.Extension{
				{Id: &d3pb.String{Value: "extension_1"}},
			}},
			path:          []string{"extension", "id"},
			expectInError: "field: extension in google.fhir.stu3.proto.Reference is repeated",
		},
		{
			name:          "field is not a message type",
			pb:            &r3pb.Patient{Id: &d3pb.Id{Value: "patient_id"}},
			path:          []string{"id", "value"},
			expectInError: "field: value in google.fhir.stu3.proto.Id is not message type",
		},
		{
			name:          "not exist field",
			pb:            &r3pb.Patient{Id: &d3pb.Id{Value: "patient_id"}},
			path:          []string{"not_exist_field"},
			expectInError: "field: not_exist_field not found in google.fhir.stu3.proto.Patient",
		},
		{
			name:          "no field given",
			pb:            &r3pb.Patient{Id: &d3pb.Id{Value: "patient_id"}},
			path:          []string{},
			expectInError: "none given field, at least one field is required",
		},
		{
			name:          "nil but typed proto",
			pb:            (*r3pb.Patient)(nil),
			path:          []string{"id"},
			expectInError: "field: id in google.fhir.stu3.proto.Patient is not populated",
		},
		{
			name:          "incomplete path for oneof fields",
			pb:            &d3pb.Reference{Reference: &d3pb.Reference_Uri{Uri: &d3pb.String{Value: "uri"}}},
			path:          []string{"reference"},
			expectInError: "not enough fields given to get field through 'oneof': reference in google.fhir.stu3.proto.Reference",
		},
	}
	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			rpb := proto.MessageReflect(tc.pb)
			_, err := GetMessage(rpb, tc.path...)
			if err == nil {
				t.Errorf("GetMessage(%v, %v) should return error, but did not", tc.pb, tc.path)
			}
			if !strings.Contains(err.Error(), tc.expectInError) {
				t.Errorf("GetMessage(%v, %v) returned unexpected error, expect \"%s\" in error, but not found, returned error: \"%v\"",
					tc.pb, tc.path, tc.expectInError, err)
			}
		})
	}
}

func TestGetList(t *testing.T) {
	tests := []struct {
		name            string
		pb              proto.Message
		path            []string
		isListOfMessage bool
		expect          []interface{}
	}{
		{
			name: "get message list",
			pb: &d3pb.HumanName{Given: []*d3pb.String{
				{Value: "given_1"},
				{Value: "given_2"},
				{Value: "given_3"},
			}},
			path:            []string{"given"},
			isListOfMessage: true,
			expect: append([]interface{}{},
				&d3pb.String{Value: "given_1"},
				&d3pb.String{Value: "given_2"},
				&d3pb.String{Value: "given_3"},
			),
		},
		{
			name: "get message through multi-level path",
			pb: &d3pb.HumanName{Text: &d3pb.String{
				Extension: []*d3pb.Extension{
					{Url: &d3pb.Uri{Value: "uri_1"}},
					{Url: &d3pb.Uri{Value: "uri_2"}},
					{Url: &d3pb.Uri{Value: "uri_3"}},
				},
			}},
			path:            []string{"text", "extension"},
			isListOfMessage: true,
			expect: append([]interface{}{},
				&d3pb.Extension{Url: &d3pb.Uri{Value: "uri_1"}},
				&d3pb.Extension{Url: &d3pb.Uri{Value: "uri_2"}},
				&d3pb.Extension{Url: &d3pb.Uri{Value: "uri_3"}},
			),
		},
		{
			name:            "get list of primitive type",
			pb:              &atpb.StringList{Strings: []string{"a", "b", "c"}},
			path:            []string{"strings"},
			isListOfMessage: false,
			expect:          append([]interface{}{}, "a", "b", "c"),
		},
		{
			name:            "empty list",
			pb:              &d3pb.HumanName{Given: []*d3pb.String{}},
			path:            []string{"given"},
			isListOfMessage: true,
			expect:          []interface{}{},
		},
	}
	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			rpb := proto.MessageReflect(tc.pb)
			rgot, err := GetList(rpb, tc.path...)
			if err != nil {
				t.Errorf("GetList(%v, %v) returned error: %v", tc.pb, tc.path, err)
			}
			if rgot.Len() != len(tc.expect) {
				t.Errorf("GetList(%v, %v) returned list of unexpected length, expect: %v, actual: %v",
					tc.pb, tc.path, len(tc.expect), rgot.Len())
			}
			for i := 0; i < rgot.Len(); i++ {
				rgoti := rgot.Get(i)
				if tc.isListOfMessage {
					expi := tc.expect[i].(proto.Message)
					goti := rgoti.Message().Interface().(proto.Message)
					if diff := cmp.Diff(expi, goti, protocmp.Transform()); diff != "" {
						t.Errorf("GetList(%v, %v) returned unexpected diff: (-want, +got) %v", tc.pb, tc.path, diff)
					}
					continue
				}
				expi := tc.expect[i]
				goti := rgoti.Interface()
				if !reflect.DeepEqual(expi, goti) {
					t.Errorf("GetList(%v, %v) returned unexpected list, expect element at %v: %v, actual: %v",
						tc.pb, tc.path, i, expi, goti)
				}
			}
		})
	}
}

func TestGetList_Invalid(t *testing.T) {
	tests := []struct {
		name          string
		pb            proto.Message
		path          []string
		expectInError string
	}{
		{
			name: "no field given",
			pb: &d3pb.HumanName{Given: []*d3pb.String{
				{Value: "given_1"},
			}},
			path:          []string{},
			expectInError: "none given field, at least one field is required",
		},
		{
			name: "invalid field",
			pb: &d3pb.HumanName{Given: []*d3pb.String{
				{Value: "given_1"},
			}},
			path:          []string{"not_exist"},
			expectInError: "field: not_exist not found in google.fhir.stu3.proto.HumanName",
		},
		{
			name:          "get not repeated field",
			pb:            &d3pb.HumanName{Text: &d3pb.String{Value: "text"}},
			path:          []string{"text"},
			expectInError: "field: text in google.fhir.stu3.proto.HumanName is not repeated",
		},
		{
			name: "get through repeated field",
			pb: &d3pb.HumanName{Given: []*d3pb.String{
				{Extension: []*d3pb.Extension{{Url: &d3pb.Uri{Value: "a"}}}},
				{Extension: []*d3pb.Extension{{Url: &d3pb.Uri{Value: "b"}}}},
				{Extension: []*d3pb.Extension{{Url: &d3pb.Uri{Value: "c"}}}},
			}},
			path:          []string{"given", "extension"},
			expectInError: "field: given in google.fhir.stu3.proto.HumanName is repeated",
		},
	}
	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			rpb := proto.MessageReflect(tc.pb)
			_, err := GetList(rpb, tc.path...)
			if err == nil {
				t.Errorf("GetList(%v, %v) should return error, but did not", tc.pb, tc.path)
			}
			if !strings.Contains(err.Error(), tc.expectInError) {
				t.Errorf("GetList(%v, %v) returned unexpected error, expect \"%s\" in error, but not found, returned error: \"%v\"",
					tc.pb, tc.path, tc.expectInError, err)
			}
		})
	}
}

func TestGetString(t *testing.T) {
	tests := []struct {
		name   string
		pb     proto.Message
		path   []string
		expect string
	}{
		{
			name:   "get nesting string",
			pb:     &d3pb.String{Value: "content"},
			path:   []string{"value"},
			expect: "content",
		},
		{
			name:   "get multi-level nesting string",
			pb:     &d3pb.HumanName{Text: &d3pb.String{Value: "name"}},
			path:   []string{"text", "value"},
			expect: "name",
		},
		{
			name:   "get through one-of field",
			pb:     &d3pb.Reference{Reference: &d3pb.Reference_Uri{Uri: &d3pb.String{Value: "uri"}}},
			path:   []string{"reference", "uri", "value"},
			expect: "uri",
		},
		{
			name:   "get unpopulated string field",
			pb:     &d3pb.Reference{Reference: &d3pb.Reference_Uri{Uri: &d3pb.String{}}},
			path:   []string{"reference", "uri", "value"},
			expect: "",
		},
	}
	for _, tc := range tests {
		rpb := proto.MessageReflect(tc.pb)
		str, err := GetString(rpb, tc.path...)
		if err != nil {
			t.Errorf("GetString(%v, %v) returned error: %v", tc.pb, tc.path, err)
		}
		if str != tc.expect {
			t.Errorf("GetString(%v, %v) returned unexpected string, expect: %v, actual: %v",
				tc.pb, tc.path, tc.expect, str)
		}
	}
}

func TestGetString_Invalid(t *testing.T) {
	tests := []struct {
		name          string
		pb            proto.Message
		path          []string
		expectInError string
	}{
		{
			name: "no field given",
			pb: &d3pb.HumanName{Given: []*d3pb.String{
				{Value: "given_1"},
			}},
			path:          []string{},
			expectInError: "none given field, at least one field is required",
		},
		{
			name: "invalid field",
			pb: &d3pb.HumanName{Given: []*d3pb.String{
				{Value: "given_1"},
			}},
			path:          []string{"not_exist"},
			expectInError: "field: not_exist not found in google.fhir.stu3.proto.HumanName",
		},
		{
			name:          "not string field",
			pb:            &d3pb.HumanName{Text: &d3pb.String{Value: "text"}},
			path:          []string{"text"},
			expectInError: "field: text in google.fhir.stu3.proto.HumanName is of kind: message, valid kinds are: [string]",
		},
		{
			name:          "get repeated string field",
			pb:            &atpb.StringList{Strings: []string{"a", "b", "c"}},
			path:          []string{"strings"},
			expectInError: "field: strings in fhir.go.jsonformat.internal.accessor.StringList is repeated",
		},
		{
			name: "get through repeated field",
			pb: &d3pb.HumanName{Given: []*d3pb.String{
				{Extension: []*d3pb.Extension{{Url: &d3pb.Uri{Value: "a"}}}},
				{Extension: []*d3pb.Extension{{Url: &d3pb.Uri{Value: "b"}}}},
				{Extension: []*d3pb.Extension{{Url: &d3pb.Uri{Value: "c"}}}},
			}},
			path:          []string{"given", "extension", "url", "value"},
			expectInError: "field: given in google.fhir.stu3.proto.HumanName is repeated",
		},
	}
	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			rpb := proto.MessageReflect(tc.pb)
			_, err := GetString(rpb, tc.path...)
			if err == nil {
				t.Errorf("GetString(%v, %v) should return error, but did not", tc.pb, tc.path)
			}
			if !strings.Contains(err.Error(), tc.expectInError) {
				t.Errorf("GetString(%v, %v) returned unexpected error, expect \"%s\" in error, but not found, returned error: \"%v\"",
					tc.pb, tc.path, tc.expectInError, err)
			}
		})
	}
}

func TestGetInt64(t *testing.T) {
	tests := []struct {
		name   string
		pb     proto.Message
		path   []string
		expect int64
	}{
		{
			name:   "get int64",
			pb:     &d3pb.Date{ValueUs: 12345},
			path:   []string{"value_us"},
			expect: 12345,
		},
		{
			name:   "get int64 through multi-level fields",
			pb:     &r3pb.Patient{BirthDate: &d3pb.Date{ValueUs: 12345}},
			path:   []string{"birth_date", "value_us"},
			expect: 12345,
		},
		{
			name:   "get unpopulated int64 field",
			pb:     &r3pb.Patient{BirthDate: &d3pb.Date{}},
			path:   []string{"birth_date", "value_us"},
			expect: 0,
		},
	}
	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			rpb := proto.MessageReflect(tc.pb)
			got, err := GetInt64(rpb, tc.path...)
			if err != nil {
				t.Errorf("GetInt64(%v, %v) returned error: %v", tc.pb, tc.path, err)
			}
			if got != tc.expect {
				t.Errorf("GetInt64(%v, %v) returned unexpected int64, expect: %v, actual: %v",
					tc.pb, tc.path, tc.expect, got)
			}
		})
	}
}

func TestGetInt64_Invalid(t *testing.T) {
	tests := []struct {
		name          string
		pb            proto.Message
		path          []string
		expectInError string
	}{
		{
			name:          "no field",
			pb:            &d3pb.Date{ValueUs: 12345},
			path:          []string{},
			expectInError: "none given field, at least one field is required",
		},
		{
			name:          "invalid field",
			pb:            &d3pb.Date{ValueUs: 12345},
			path:          []string{"not_exist"},
			expectInError: "field: not_exist not found in google.fhir.stu3.proto.Date",
		},
		{
			name:          "not int64 field",
			pb:            &r3pb.Patient{BirthDate: &d3pb.Date{ValueUs: 12345}},
			path:          []string{"birth_date"},
			expectInError: "field: birth_date in google.fhir.stu3.proto.Patient is of kind: message, valid kinds are: [int64 sint64 sfixed64]",
		},
	}
	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			rpb := proto.MessageReflect(tc.pb)
			_, err := GetInt64(rpb, tc.path...)
			if err == nil {
				t.Errorf("GetInt64(%v, %v) should return error, but did not", tc.pb, tc.path)
			}
			if !strings.Contains(err.Error(), tc.expectInError) {
				t.Errorf("GetInt64(%v, %v) returned unexpected error, expect \"%s\" in error, but not found, returned error: \"%v\"",
					tc.pb, tc.path, tc.expectInError, err)
			}
		})
	}
}

func TestGetUint32(t *testing.T) {
	tests := []struct {
		name   string
		pb     proto.Message
		path   []string
		expect uint32
	}{
		{
			name:   "get uint32",
			pb:     &d3pb.PositiveInt{Value: 123},
			path:   []string{"value"},
			expect: 123,
		},
		{
			name: "get uint32 through multi-level fields",
			pb: &d3pb.Extension{Value: &d3pb.Extension_ValueX{
				Choice: &d3pb.Extension_ValueX_PositiveInt{PositiveInt: &d3pb.PositiveInt{Value: 123}},
			}},
			path:   []string{"value", "choice", "positive_int", "value"},
			expect: 123,
		},
		{
			name: "get unpopulated uint32 field",
			pb: &d3pb.Extension{Value: &d3pb.Extension_ValueX{
				Choice: &d3pb.Extension_ValueX_PositiveInt{PositiveInt: &d3pb.PositiveInt{}},
			}},
			path:   []string{"value", "choice", "positive_int", "value"},
			expect: 0,
		},
	}
	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			rpb := proto.MessageReflect(tc.pb)
			got, err := GetUint32(rpb, tc.path...)
			if err != nil {
				t.Errorf("GetUint32(%v, %v) returned error: %v", tc.pb, tc.path, err)
			}
			if got != tc.expect {
				t.Errorf("GetUint32(%v, %v) returned unexpected int64, expect: %v, actual: %v",
					tc.pb, tc.path, tc.expect, got)
			}
		})
	}
}

func TestGetUint32_Invalid(t *testing.T) {
	tests := []struct {
		name          string
		pb            proto.Message
		path          []string
		expectInError string
	}{
		{
			name: "no field",
			pb: &d3pb.Extension{Value: &d3pb.Extension_ValueX{
				Choice: &d3pb.Extension_ValueX_PositiveInt{PositiveInt: &d3pb.PositiveInt{Value: 123}},
			}},
			path:          []string{},
			expectInError: "none given field, at least one field is required",
		},
		{
			name: "invalid field",
			pb: &d3pb.Extension{Value: &d3pb.Extension_ValueX{
				Choice: &d3pb.Extension_ValueX_PositiveInt{PositiveInt: &d3pb.PositiveInt{Value: 123}},
			}},
			path:          []string{"not_exist"},
			expectInError: "field: not_exist not found in google.fhir.stu3.proto.Extension",
		},
		{
			name:          "not uint32 field",
			pb:            &r3pb.Patient{BirthDate: &d3pb.Date{ValueUs: 12345}},
			path:          []string{"birth_date"},
			expectInError: "field: birth_date in google.fhir.stu3.proto.Patient is of kind: message, valid kinds are: [uint32 fixed32]",
		},
	}
	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			rpb := proto.MessageReflect(tc.pb)
			_, err := GetUint32(rpb, tc.path...)
			if err == nil {
				t.Errorf("GetUint32(%v, %v) should return error, but did not", tc.pb, tc.path)
			}
			if !strings.Contains(err.Error(), tc.expectInError) {
				t.Errorf("GetUint32(%v, %v) returned unexpected error, expect \"%s\" in error, but not found, returned error: \"%v\"",
					tc.pb, tc.path, tc.expectInError, err)
			}
		})
	}
}

func TestGetBytes(t *testing.T) {
	tests := []struct {
		name   string
		pb     proto.Message
		path   []string
		expect []byte
	}{
		{
			name:   "simple",
			pb:     &d3pb.Base64Binary{Value: []byte("abc")},
			path:   []string{"value"},
			expect: []byte("abc"),
		},
		{
			name:   "multi-level",
			pb:     &d3pb.Attachment{Data: &d3pb.Base64Binary{Value: []byte("abc")}},
			path:   []string{"data", "value"},
			expect: []byte("abc"),
		},
		{
			name:   "not populated",
			pb:     &d3pb.Attachment{Data: &d3pb.Base64Binary{}},
			path:   []string{"data", "value"},
			expect: []byte{},
		},
	}
	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			rpb := proto.MessageReflect(tc.pb)
			got, err := GetBytes(rpb, tc.path...)
			if err != nil {
				t.Errorf("GetBytes(%v, %v) returned error: %v", tc.pb, tc.path, err)
			}
			if !bytes.Equal(tc.expect, got) {
				t.Errorf("GetBytes(%v, %v) returned unexpected bytes, expect: %v, actual: %v",
					tc.pb, tc.path, string(tc.expect), string(got))
			}
		})
	}
}

func TestGetBytes_Invalid(t *testing.T) {
	tests := []struct {
		name          string
		pb            proto.Message
		path          []string
		expectInError string
	}{
		{
			name:          "no field",
			pb:            &d3pb.Base64Binary{Value: []byte("abc")},
			path:          []string{},
			expectInError: "none given field, at least one field is required",
		},
		{
			name:          "invalid field",
			pb:            &d3pb.Base64Binary{Value: []byte("abc")},
			path:          []string{"not_exist"},
			expectInError: "field: not_exist not found in google.fhir.stu3.proto.Base64Binary",
		},
		{
			name:          "not []byte field",
			pb:            &d3pb.Attachment{Data: &d3pb.Base64Binary{}},
			path:          []string{"data"},
			expectInError: "field: data in google.fhir.stu3.proto.Attachment is of kind: message, valid kinds are: [bytes]",
		},
	}
	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			rpb := proto.MessageReflect(tc.pb)
			_, err := GetBytes(rpb, tc.path...)
			if err == nil {
				t.Errorf("GetBytes(%v, %v) should return error, but did not", tc.pb, tc.path)
			}
			if !strings.Contains(err.Error(), tc.expectInError) {
				t.Errorf("GetBytes(%v, %v) returned unexpected error, expect \"%s\" in error, but not found, returned error: \"%v\"",
					tc.pb, tc.path, tc.expectInError, err)
			}
		})
	}
}

func TestGetEnumNumber(t *testing.T) {
	tests := []struct {
		name   string
		pb     proto.Message
		path   []string
		expect int32
	}{
		{
			name:   "simple",
			pb:     &d3pb.Date{Precision: d3pb.Date_YEAR},
			path:   []string{"precision"},
			expect: int32(d3pb.Date_YEAR),
		},
		{
			name:   "multi-level",
			pb:     &r3pb.Patient{BirthDate: &d3pb.Date{Precision: d3pb.Date_DAY}},
			path:   []string{"birth_date", "precision"},
			expect: int32(d3pb.Date_DAY),
		},
		{
			name:   "not populated",
			pb:     &r3pb.Patient{BirthDate: &d3pb.Date{}},
			path:   []string{"birth_date", "precision"},
			expect: int32(d3pb.Date_PRECISION_UNSPECIFIED),
		},
	}
	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			rpb := proto.MessageReflect(tc.pb)
			got, err := GetEnumNumber(rpb, tc.path...)
			if err != nil {
				t.Errorf("GetEnumNumber(%v, %v) returned error: %v", tc.pb, tc.path, err)
			}
			if int32(got) != tc.expect {
				t.Errorf("GetEnumNumber(%v, %v) returned unexpected value, expect: %v, actual: %v",
					tc.pb, tc.path, tc.expect, int32(got))
			}
		})
	}
}

func TestGetEnumNumber_Invalid(t *testing.T) {
	tests := []struct {
		name          string
		pb            proto.Message
		path          []string
		expectInError string
	}{
		{
			name:          "no field",
			pb:            &r3pb.Patient{BirthDate: &d3pb.Date{Precision: d3pb.Date_DAY}},
			path:          []string{},
			expectInError: "none given field, at least one field is required",
		},
		{
			name:          "invalid field",
			pb:            &r3pb.Patient{BirthDate: &d3pb.Date{Precision: d3pb.Date_DAY}},
			path:          []string{"not_exist"},
			expectInError: "field: not_exist not found in google.fhir.stu3.proto.Patient",
		},
		{
			name:          "not enum field",
			pb:            &r3pb.Patient{BirthDate: &d3pb.Date{Precision: d3pb.Date_DAY}},
			path:          []string{"birth_date", "value_us"},
			expectInError: "field: value_us in google.fhir.stu3.proto.Date is of kind: int64, valid kinds are: [enum]",
		},
	}
	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			rpb := proto.MessageReflect(tc.pb)
			_, err := GetEnumNumber(rpb, tc.path...)
			if err == nil {
				t.Errorf("GetEnumNumber(%v, %v) should return error, but did not", tc.pb, tc.path)
			}
			if !strings.Contains(err.Error(), tc.expectInError) {
				t.Errorf("GetEnumNumber(%v, %v) returned unexpected error, expect \"%s\" in error, but not found, returned error: \"%v\"",
					tc.pb, tc.path, tc.expectInError, err)
			}
		})
	}
}

func TestGetEnumDescriptor(t *testing.T) {
	tests := []struct {
		name     string
		pb       proto.Message
		path     []string
		fullname string
	}{
		{
			name:     "simple",
			pb:       &d3pb.Date{Precision: d3pb.Date_YEAR},
			path:     []string{"precision"},
			fullname: "google.fhir.stu3.proto.Date.Precision",
		},
		{
			name:     "multi-level",
			pb:       &r3pb.Patient{BirthDate: &d3pb.Date{Precision: d3pb.Date_DAY}},
			path:     []string{"birth_date", "precision"},
			fullname: "google.fhir.stu3.proto.Date.Precision",
		},
		{
			name:     "not populated",
			pb:       &r3pb.Patient{BirthDate: &d3pb.Date{}},
			path:     []string{"birth_date", "precision"},
			fullname: "google.fhir.stu3.proto.Date.Precision",
		},
	}
	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			rpb := proto.MessageReflect(tc.pb)
			got, err := GetEnumDescriptor(rpb.Descriptor(), tc.path...)
			if err != nil {
				t.Errorf("GetEnumDescriptor(%v, %v) returned error: %v", tc.pb, tc.path, err)
			}
			if string(got.FullName()) != tc.fullname {
				t.Errorf("GetEnumDescriptor(%v, %v) returned unexpected descriptor name, expect: %v, actual: %v",
					tc.pb, tc.path, tc.fullname, string(got.FullName()))
			}
		})
	}
}

func TestGetEnumDescriptor_Invalid(t *testing.T) {
	tests := []struct {
		name          string
		pb            proto.Message
		path          []string
		expectInError string
	}{
		{
			name:          "no field",
			pb:            &r3pb.Patient{BirthDate: &d3pb.Date{Precision: d3pb.Date_DAY}},
			path:          []string{},
			expectInError: "none given field, at least one field is required",
		},
		{
			name:          "invalid field",
			pb:            &r3pb.Patient{BirthDate: &d3pb.Date{Precision: d3pb.Date_DAY}},
			path:          []string{"not_exist"},
			expectInError: "field: not_exist not found in google.fhir.stu3.proto.Patient",
		},
		{
			name:          "not enum field",
			pb:            &r3pb.Patient{BirthDate: &d3pb.Date{Precision: d3pb.Date_DAY}},
			path:          []string{"birth_date", "value_us"},
			expectInError: "field: value_us in google.fhir.stu3.proto.Date is not of enum kind",
		},
	}
	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			rpb := proto.MessageReflect(tc.pb)
			_, err := GetEnumDescriptor(rpb.Descriptor(), tc.path...)
			if err == nil {
				t.Errorf("GetEnumNumber(%v, %v) should return error, but did not", tc.pb, tc.path)
			}
			if !strings.Contains(err.Error(), tc.expectInError) {
				t.Errorf("GetEnumNumber(%v, %v) returned unexpected error, expect \"%s\" in error, but not found, returned error: \"%v\"",
					tc.pb, tc.path, tc.expectInError, err)
			}
		})
	}
}

func TestSetValue(t *testing.T) {
	tests := []struct {
		name   string
		pb     proto.Message
		path   []string
		value  interface{}
		expect proto.Message
	}{
		{
			name:   "bool",
			pb:     &d3pb.Boolean{Value: false},
			path:   []string{"value"},
			value:  true,
			expect: &d3pb.Boolean{Value: true},
		},
		{
			name:   "enum with enum type",
			pb:     &d3pb.Date{Precision: d3pb.Date_YEAR},
			path:   []string{"precision"},
			value:  d3pb.Date_MONTH,
			expect: &d3pb.Date{Precision: d3pb.Date_MONTH},
		},
		{
			name:   "enum with int32 type",
			pb:     &d3pb.Date{Precision: d3pb.Date_YEAR},
			path:   []string{"precision"},
			value:  int32(d3pb.Date_MONTH),
			expect: &d3pb.Date{Precision: d3pb.Date_MONTH},
		},
		{
			name:   "int64",
			pb:     &d3pb.Date{ValueUs: 0},
			path:   []string{"value_us"},
			value:  int64(12345),
			expect: &d3pb.Date{ValueUs: 12345},
		},
		{
			name:   "uint32",
			pb:     &d3pb.PositiveInt{Value: 0},
			path:   []string{"value"},
			value:  uint32(123),
			expect: &d3pb.PositiveInt{Value: 123},
		},
		{
			name:   "string",
			pb:     &d3pb.String{Value: "svn"},
			path:   []string{"value"},
			value:  "git",
			expect: &d3pb.String{Value: "git"},
		},
		{
			name:   "[]byte",
			pb:     &d3pb.Base64Binary{Value: []byte("John Snow")},
			path:   []string{"value"},
			value:  []byte("Aegon Targaryen"),
			expect: &d3pb.Base64Binary{Value: []byte("Aegon Targaryen")},
		},
		{
			name:   "message",
			pb:     &r3pb.Patient{Active: &d3pb.Boolean{Value: false}},
			path:   []string{"active"},
			value:  &d3pb.Boolean{Value: true},
			expect: &r3pb.Patient{Active: &d3pb.Boolean{Value: true}},
		},
		{
			name:   "multi-level primitive",
			pb:     &r3pb.Patient{Active: &d3pb.Boolean{Value: false}},
			path:   []string{"active", "value"},
			value:  true,
			expect: &r3pb.Patient{Active: &d3pb.Boolean{Value: true}},
		},
		{
			name:   "multi-level message",
			pb:     &d3pb.HumanName{Period: &d3pb.Period{Id: &d3pb.String{Value: "c++"}}},
			path:   []string{"period", "id"},
			value:  &d3pb.String{Value: "go"},
			expect: &d3pb.HumanName{Period: &d3pb.Period{Id: &d3pb.String{Value: "go"}}},
		},
		{
			name:   "multi-level unpopulated",
			pb:     &r3pb.Patient{},
			path:   []string{"active", "value"},
			value:  true,
			expect: &r3pb.Patient{Active: &d3pb.Boolean{Value: true}},
		},
		{
			name:   "through one-of message but not the immediate sub-field",
			pb:     &d3pb.Reference{Reference: &d3pb.Reference_Uri{Uri: &d3pb.String{Value: "uri"}}},
			path:   []string{"reference", "fragment", "value"},
			value:  "fragment",
			expect: &d3pb.Reference{Reference: &d3pb.Reference_Fragment{Fragment: &d3pb.String{Value: "fragment"}}},
		},
		{
			name:   "immediate sub-field of an Oneof field",
			pb:     &d3pb.Reference{Reference: &d3pb.Reference_Uri{Uri: &d3pb.String{Value: "uri"}}},
			path:   []string{"reference", "fragment"},
			value:  &d3pb.String{Value: "fragment"},
			expect: &d3pb.Reference{Reference: &d3pb.Reference_Fragment{Fragment: &d3pb.String{Value: "fragment"}}},
		},
	}
	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			rpb := proto.MessageReflect(tc.pb)
			if err := SetValue(rpb, tc.value, tc.path...); err != nil {
				t.Errorf("SeteValue(%v, %v, %v) returned error: %v", tc.pb, tc.value, tc.path, err)
			}
			got := rpb.Interface().(proto.Message)
			if diff := cmp.Diff(tc.expect, got, protocmp.Transform()); diff != "" {
				t.Errorf("SetValue(%v, %v, %v) returned unexpected diff: (-want, +got) %v", tc.pb, tc.value, tc.path, diff)
			}
		})
	}
}

func TestSetValue_Invalid(t *testing.T) {
	tests := []struct {
		name          string
		pb            proto.Message
		path          []string
		value         interface{}
		expectInError string
	}{
		{
			name:          "no field",
			pb:            &d3pb.String{Value: "str"},
			path:          []string{},
			value:         "rts",
			expectInError: "none given field, at least one field is required",
		},
		{
			name:          "invalid field",
			pb:            &d3pb.String{Value: "str"},
			path:          []string{"not_exist"},
			value:         "rts",
			expectInError: "field: not_exist not found in google.fhir.stu3.proto.String",
		},
		{
			name:          "repeated field",
			pb:            &d3pb.String{Extension: []*d3pb.Extension{{Id: &d3pb.String{Value: "ext1"}}}},
			path:          []string{"extension"},
			value:         &d3pb.Extension{Id: &d3pb.String{Value: "ext2"}},
			expectInError: "field: extension in google.fhir.stu3.proto.String is repeated",
		},
		{
			name:          "primitive go kind mismatch",
			pb:            &d3pb.String{Value: "str"},
			path:          []string{"value"},
			value:         int32(123),
			expectInError: "go kind mismatch, field: value in google.fhir.stu3.proto.String is of kind: string, given value is of kind: int32",
		},
		{
			name:          "[]byte type mismatch",
			pb:            &d3pb.Base64Binary{Value: []byte("abc")},
			path:          []string{"value"},
			value:         "123",
			expectInError: "type mismatch, given value is not a byte slice ([]byte type)",
		},
		{
			name:          "not a proto message",
			pb:            &r3pb.Patient{},
			path:          []string{"birth_date"},
			value:         "&d3pb.Date{ValueUs: 12345}",
			expectInError: "type mismatch, given value is not a proto message",
		},
		{
			name:          "message type mismatch",
			pb:            &r3pb.Patient{},
			path:          []string{"birth_date"},
			value:         &d3pb.DateTime{ValueUs: 12345},
			expectInError: "type mismatch, field birth_date in google.fhir.stu3.proto.Patient is a message of type: google.fhir.stu3.proto.Date, given value is a message of type: google.fhir.stu3.proto.DateTime",
		},
	}
	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			rpb := proto.MessageReflect(tc.pb)
			err := SetValue(rpb, tc.value, tc.path...)
			if err == nil {
				t.Errorf("SetValue(%v, %v, %v) should return error, but did not", tc.pb, tc.value, tc.path)
			}
			if !strings.Contains(err.Error(), tc.expectInError) {
				t.Errorf("SetValue(%v, %v, %v) returned unexpected error, expect \"%v\" in error, but not found, returned error: \"%v\"",
					tc.pb, tc.value, tc.path, tc.expectInError, err.Error())
			}
		})
	}
}

func TestAppendValue(t *testing.T) {
	tests := []struct {
		name   string
		pb     proto.Message
		path   []string
		value  interface{}
		expect proto.Message
	}{
		{
			name:   "pritimive string",
			pb:     &atpb.StringList{Strings: []string{"a", "b", "c"}},
			path:   []string{"strings"},
			value:  "d",
			expect: &atpb.StringList{Strings: []string{"a", "b", "c", "d"}},
		},
		{
			name:   "message",
			pb:     &d3pb.Base64Binary{Extension: []*d3pb.Extension{{Id: &d3pb.String{Value: "id1"}}}},
			path:   []string{"extension"},
			value:  &d3pb.Extension{Id: &d3pb.String{Value: "id2"}},
			expect: &d3pb.Base64Binary{Extension: []*d3pb.Extension{{Id: &d3pb.String{Value: "id1"}}, {Id: &d3pb.String{Value: "id2"}}}},
		},
		{
			name:   "not populated",
			pb:     &r3pb.Patient{},
			path:   []string{"id", "extension"},
			value:  &d3pb.Extension{Id: &d3pb.String{Value: "id"}},
			expect: &r3pb.Patient{Id: &d3pb.Id{Extension: []*d3pb.Extension{{Id: &d3pb.String{Value: "id"}}}}},
		},
	}
	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			rpb := proto.MessageReflect(tc.pb)
			err := AppendValue(rpb, tc.value, tc.path...)
			if err != nil {
				t.Errorf("AppendValue(%v, %v, %v) returned error: %v", tc.pb, tc.value, tc.path, err)
			}
			got := rpb.Interface().(proto.Message)
			if diff := cmp.Diff(tc.expect, got, protocmp.Transform()); diff != "" {
				t.Errorf("AppendValue(%v, %v, %v) returned unexpected diff: (-want, +got) %v", tc.pb, tc.value, tc.path, diff)
			}
		})
	}
}

func TestAppendValue_Invalid(t *testing.T) {
	tests := []struct {
		name          string
		pb            proto.Message
		path          []string
		value         interface{}
		expectInError string
	}{
		{
			name:          "no field",
			pb:            &atpb.StringList{Strings: []string{"a", "b", "c"}},
			path:          []string{},
			value:         "d",
			expectInError: "none given field, at least one field is required",
		},
		{
			name:          "invalid field",
			pb:            &d3pb.Extension{Extension: []*d3pb.Extension{}},
			path:          []string{"not_exist"},
			value:         &d3pb.Extension{Id: &d3pb.String{Value: "id"}},
			expectInError: "field: not_exist not found in google.fhir.stu3.proto.Extension",
		},
		{
			name:          "not repeated field",
			pb:            &d3pb.Extension{Extension: []*d3pb.Extension{}},
			path:          []string{"id"},
			value:         &d3pb.Extension{Id: &d3pb.String{Value: "id"}},
			expectInError: "field: id in google.fhir.stu3.proto.Extension is not repeated",
		},
		{
			name:          "primitive go kind mismatch",
			pb:            &atpb.StringList{Strings: []string{"a", "b", "c"}},
			path:          []string{"strings"},
			value:         int32(123),
			expectInError: "go kind mismatch, field: strings in fhir.go.jsonformat.internal.accessor.StringList is repeated of kind: string, given value is of kind: int32",
		},
		{
			name:          "not a proto message",
			pb:            &d3pb.Extension{Extension: []*d3pb.Extension{}},
			path:          []string{"extension"},
			value:         "id",
			expectInError: "type mismatch, given value is not a proto message",
		},
		{
			name:          "message type mismatch",
			pb:            &d3pb.Extension{Extension: []*d3pb.Extension{}},
			path:          []string{"extension"},
			value:         &d3pb.String{Value: "id"},
			expectInError: "type mismatch, field: extension in google.fhir.stu3.proto.Extension is a list of messages of type: google.fhir.stu3.proto.Extension, given value is a message of type: google.fhir.stu3.proto.String",
		},
	}
	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			rpb := proto.MessageReflect(tc.pb)
			err := AppendValue(rpb, tc.value, tc.path...)
			if err == nil {
				t.Errorf("AppendValue(%v, %v, %v) should return error, but did not", tc.pb, tc.value, tc.path)
			}
			if !strings.Contains(err.Error(), tc.expectInError) {
				t.Errorf("AppendValue(%v, %v, %v) returned unexpected error, expect \"%v\" in error, but not found, returned error: \"%v\"",
					tc.pb, tc.value, tc.path, tc.expectInError, err.Error())
			}
		})
	}
}
