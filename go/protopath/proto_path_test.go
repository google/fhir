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
package protopath

import (
	"testing"

	anypb "google.golang.org/protobuf/types/known/anypb"
	pptpb "github.com/google/fhir/go/protopath/protopathtest_go_proto"
	"github.com/google/go-cmp/cmp"
	"github.com/google/go-cmp/cmp/cmpopts"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/testing/protocmp"
)

func mustMarshalAny(t *testing.T, pb proto.Message) *anypb.Any {
	t.Helper()
	anyPB, err := anypb.New(pb)
	if err != nil {
		t.Fatalf("creating Any: %v", err)
	}
	return anyPB
}

func TestSet(t *testing.T) {
	tests := []struct {
		name  string
		path  Path
		value any
		msg   proto.Message
		want  proto.Message
	}{
		{
			"single field",
			NewPath("message_field"),
			&pptpb.Message_InnerMessage{InnerField: 1},
			&pptpb.Message{},
			&pptpb.Message{MessageField: &pptpb.Message_InnerMessage{InnerField: 1}},
		},
		{
			"nested field - parent exists",
			NewPath("message_field.inner_field"),
			int32(1),
			&pptpb.Message{MessageField: &pptpb.Message_InnerMessage{}},
			&pptpb.Message{MessageField: &pptpb.Message_InnerMessage{InnerField: 1}},
		},
		{
			"nested field - no parent",
			NewPath("message_field.inner_field"),
			int32(1),
			&pptpb.Message{},
			&pptpb.Message{MessageField: &pptpb.Message_InnerMessage{InnerField: 1}},
		},
		{
			"repeated field - index exists",
			NewPath("repeated_message_field.0.inner_field"),
			int32(2),
			&pptpb.Message{RepeatedMessageField: []*pptpb.Message_InnerMessage{{InnerField: 1}}},
			&pptpb.Message{RepeatedMessageField: []*pptpb.Message_InnerMessage{{InnerField: 2}}},
		},
		{
			"repeated field - end",
			NewPath("repeated_message_field.-1.inner_field"),
			int32(1),
			&pptpb.Message{RepeatedMessageField: []*pptpb.Message_InnerMessage{}},
			&pptpb.Message{RepeatedMessageField: []*pptpb.Message_InnerMessage{{InnerField: 1}}},
		},
		{
			"repeated field - no parent",
			NewPath("repeated_message_field.0.inner_field"),
			int32(1),
			&pptpb.Message{},
			&pptpb.Message{RepeatedMessageField: []*pptpb.Message_InnerMessage{{InnerField: 1}}},
		},
		{
			"repeated field - clear",
			NewPath("repeated_message_field"),
			Zero,
			&pptpb.Message{RepeatedMessageField: []*pptpb.Message_InnerMessage{{InnerField: 1}}},
			&pptpb.Message{RepeatedMessageField: []*pptpb.Message_InnerMessage{}},
		},
		{
			"repeated field - set",
			NewPath("repeated_message_field"),
			[]*pptpb.Message_InnerMessage{{InnerField: 1}, {InnerField: 2}},
			&pptpb.Message{RepeatedMessageField: []*pptpb.Message_InnerMessage{{InnerField: 3}}},
			&pptpb.Message{RepeatedMessageField: []*pptpb.Message_InnerMessage{{InnerField: 1}, {InnerField: 2}}},
		},
		{
			"repeated scalar field - set",
			NewPath("message_field.repeated_inner_field"),
			[]int32{1, 2},
			&pptpb.Message{MessageField: &pptpb.Message_InnerMessage{}},
			&pptpb.Message{MessageField: &pptpb.Message_InnerMessage{RepeatedInnerField: []int32{1, 2}}},
		},
		{
			"repeated scalar field - clear",
			NewPath("message_field.repeated_inner_field"),
			Zero,
			&pptpb.Message{MessageField: &pptpb.Message_InnerMessage{RepeatedInnerField: []int32{1, 2}}},
			&pptpb.Message{MessageField: &pptpb.Message_InnerMessage{}},
		},
		{
			"repeated scalar field - no parent",
			NewPath("message_field.repeated_inner_field"),
			[]int32{1, 2},
			&pptpb.Message{},
			&pptpb.Message{MessageField: &pptpb.Message_InnerMessage{RepeatedInnerField: []int32{1, 2}}},
		},
		{
			"repeated nested scalar field - no parent",
			NewPath("repeated_message_field.-1.repeated_inner_field"),
			[]int32{1, 2},
			&pptpb.Message{},
			&pptpb.Message{RepeatedMessageField: []*pptpb.Message_InnerMessage{{RepeatedInnerField: []int32{1, 2}}}},
		},
		{
			"repeated scalar field element",
			NewPath("message_field.repeated_inner_field.-1"),
			int32(1),
			&pptpb.Message{},
			&pptpb.Message{MessageField: &pptpb.Message_InnerMessage{RepeatedInnerField: []int32{1}}},
		},
		{
			"repeated field element",
			NewPath("repeated_message_field.-1"),
			&pptpb.Message_InnerMessage{InnerField: 1},
			&pptpb.Message{RepeatedMessageField: []*pptpb.Message_InnerMessage{}},
			&pptpb.Message{RepeatedMessageField: []*pptpb.Message_InnerMessage{{InnerField: 1}}},
		},
		{
			"missing field - zero value of pointer",
			NewPath("message_field.inner_field"),
			Zero,
			&pptpb.Message{MessageField: &pptpb.Message_InnerMessage{InnerField: 1}},
			&pptpb.Message{MessageField: &pptpb.Message_InnerMessage{}},
		},
		{
			"typed nil value",
			NewPath("message_field"),
			(*pptpb.Message_InnerMessage)(nil),
			&pptpb.Message{},
			&pptpb.Message{},
		},
		{
			"untyped nil value",
			NewPath("message_field"),
			nil,
			&pptpb.Message{},
			&pptpb.Message{},
		},
		{
			"clear primitive field",
			NewPath("message_field.inner_field"),
			Zero,
			&pptpb.Message{MessageField: &pptpb.Message_InnerMessage{InnerField: 1}},
			&pptpb.Message{MessageField: &pptpb.Message_InnerMessage{}},
		},
		{
			"oneof",
			NewPath("oneof_message_field.inner_field"),
			int32(1),
			&pptpb.Message{
				Oneof: &pptpb.Message_OneofMessageField{
					OneofMessageField: &pptpb.Message_InnerMessage{
						InnerField: 0}}},
			&pptpb.Message{
				Oneof: &pptpb.Message_OneofMessageField{
					OneofMessageField: &pptpb.Message_InnerMessage{
						InnerField: 1}}},
		},
		{
			"oneof and case",
			NewPath("oneof.oneof_message_field"),
			&pptpb.Message_InnerMessage{InnerField: 1},
			&pptpb.Message{},
			&pptpb.Message{
				Oneof: &pptpb.Message_OneofMessageField{
					OneofMessageField: &pptpb.Message_InnerMessage{
						InnerField: 1}}},
		},
		{
			"oneof - empty",
			NewPath("oneof_message_field.inner_field"),
			int32(1),
			&pptpb.Message{},
			&pptpb.Message{
				Oneof: &pptpb.Message_OneofMessageField{
					OneofMessageField: &pptpb.Message_InnerMessage{
						InnerField: 1}}},
		},
		{
			"oneof - wrap",
			NewPath("oneof_message_field"),
			&pptpb.Message_InnerMessage{InnerField: 1},
			&pptpb.Message{},
			&pptpb.Message{
				Oneof: &pptpb.Message_OneofMessageField{
					OneofMessageField: &pptpb.Message_InnerMessage{
						InnerField: 1}}},
		},
		{
			"oneof - primitive",
			NewPath("oneof"),
			true,
			&pptpb.Message{},
			&pptpb.Message{Oneof: &pptpb.Message_OneofPrimitiveField{OneofPrimitiveField: true}},
		},
		{
			"oneof - message",
			NewPath("oneof"),
			&pptpb.Message_InnerMessage{
				InnerField: 1,
			},
			&pptpb.Message{},
			&pptpb.Message{
				Oneof: &pptpb.Message_OneofMessageField{
					OneofMessageField: &pptpb.Message_InnerMessage{
						InnerField: 1,
					}}},
		},
		{
			"oneof - nil message",
			NewPath("oneof_message_field"),
			(*pptpb.Message_InnerMessage)(nil),
			&pptpb.Message{},
			&pptpb.Message{},
		},
		{
			"enum",
			NewPath("type"),
			pptpb.MessageType_TYPE_1,
			&pptpb.Message{},
			&pptpb.Message{Type: pptpb.MessageType_TYPE_1},
		},
		{
			"enum by name",
			NewPath("type"),
			"TYPE_1",
			&pptpb.Message{},
			&pptpb.Message{Type: pptpb.MessageType_TYPE_1},
		},
		{
			"bytes",
			NewPath("bytes_field"),
			[]byte{1, 2, 3},
			&pptpb.Message{},
			&pptpb.Message{BytesField: []byte{1, 2, 3}},
		},
		{
			"bytes - nil",
			NewPath("bytes_field"),
			nil,
			&pptpb.Message{BytesField: []byte{1, 2, 3}},
			&pptpb.Message{},
		},
		{
			"any",
			NewPath("any_field.inner_field"),
			int32(2),
			&pptpb.Message{
				AnyField: mustMarshalAny(t, &pptpb.Message_InnerMessage{
					InnerField: 1,
				}),
			},
			&pptpb.Message{
				AnyField: mustMarshalAny(t, &pptpb.Message_InnerMessage{
					InnerField: 2,
				}),
			},
		},
		{
			"nested - any",
			NewPath("any_field.any_field.inner_field"),
			int32(2),
			&pptpb.Message{
				AnyField: mustMarshalAny(t, &pptpb.Message{
					AnyField: mustMarshalAny(t, &pptpb.Message_InnerMessage{
						InnerField: 1,
					}),
				}),
			},
			&pptpb.Message{
				AnyField: mustMarshalAny(t, &pptpb.Message{
					AnyField: mustMarshalAny(t, &pptpb.Message_InnerMessage{
						InnerField: 2,
					}),
				}),
			},
		},
		{
			"set nil map",
			NewPath("map_string_string_field"),
			nil,
			&pptpb.Message{},
			&pptpb.Message{MapStringStringField: nil}},
		{
			"set zero map value",
			NewPath("map_string_message_field.`foo`.map_int32_enum_field.1"),
			Zero,
			&pptpb.Message{MapStringMessageField: map[string]*pptpb.Message_InnerMessage{}},
			&pptpb.Message{MapStringMessageField: map[string]*pptpb.Message_InnerMessage{"foo": &pptpb.Message_InnerMessage{MapInt32EnumField: map[int32]pptpb.MessageType{1: pptpb.MessageType_INVALID_UNINITIALIZED}}}},
		},
		{
			"map<string, string> field",
			NewPath("map_string_string_field"),
			map[string]string{"foo": "bar"},
			&pptpb.Message{},
			&pptpb.Message{MapStringStringField: map[string]string{"foo": "bar"}},
		},
		{
			"map<int32, bytes> field - update",
			NewPath("map_int32_bytes_field"),
			map[int32][]byte{1: []byte("bar"), 2: []byte("baz")},
			&pptpb.Message{MapInt32BytesField: map[int32][]byte{1: []byte("foo")}},
			&pptpb.Message{MapInt32BytesField: map[int32][]byte{1: []byte("bar"), 2: []byte("baz")}},
		},
		{
			"set value in map<bool, int32>",
			NewPath("map_bool_int32_field.true"),
			int32(1),
			&pptpb.Message{},
			&pptpb.Message{MapBoolInt32Field: map[bool]int32{true: 1}},
		},
		{
			"update value in map<string, InnerMessage>",
			NewPath("map_string_message_field.foo"),
			&pptpb.Message_InnerMessage{InnerField: 1},
			&pptpb.Message{MapStringMessageField: map[string]*pptpb.Message_InnerMessage{"foo": nil}},
			&pptpb.Message{MapStringMessageField: map[string]*pptpb.Message_InnerMessage{"foo": &pptpb.Message_InnerMessage{InnerField: 1}}},
		},
		{
			"clear value in map<string, string>",
			NewPath(`map_string_string_field`),
			Zero,
			&pptpb.Message{MapStringStringField: map[string]string{"foo": "bar"}},
			&pptpb.Message{MapStringStringField: nil},
		},
		{
			"add zero value map - no parent",
			NewPath(`repeated_message_field.-1.map_int32_enum_field.10`),
			Zero,
			&pptpb.Message{},
			&pptpb.Message{RepeatedMessageField: []*pptpb.Message_InnerMessage{{MapInt32EnumField: map[int32]pptpb.MessageType{10: pptpb.MessageType_INVALID_UNINITIALIZED}}}},
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			msg := proto.Clone(test.msg)
			if err := Set(msg, test.path, test.value); err != nil {
				t.Fatalf("Set failed with err: %v", err)
			}

			if diff := cmp.Diff(test.want, msg, protocmp.Transform()); diff != "" {
				t.Errorf("Set(_, %v, %v) => %v returned unexpected diff: (-want, +got) %v", test.value, test.path, test.msg, diff)
			}
		})
	}
}

func TestSet_Errors(t *testing.T) {
	tests := []struct {
		name  string
		path  Path
		value any
		msg   proto.Message
	}{
		{
			"empty path part",
			NewPath("foo."),
			Zero,
			&pptpb.Message{},
		},
		{
			"empty path",
			Path{},
			Zero,
			&pptpb.Message{},
		},
		{
			"typed nil message",
			NewPath("message_field"),
			&pptpb.Message_InnerMessage{},
			(*pptpb.Message)(nil),
		},
		{
			"untyped nil message",
			NewPath("message_field"),
			&pptpb.Message_InnerMessage{},
			nil,
		},
		{
			"invalid field",
			NewPath("meta2"),
			&pptpb.Message_InnerMessage{InnerField: 1},
			&pptpb.Message{},
		},
		{
			"invalid field - primitive",
			NewPath("message_field.inner_field.value"),
			Zero,
			&pptpb.Message{MessageField: &pptpb.Message_InnerMessage{InnerField: 1}},
		},
		{
			"invalid nested field - parent exists",
			NewPath("message_field.inner_field2"),
			1,
			&pptpb.Message{MessageField: &pptpb.Message_InnerMessage{}},
		},
		{
			"invalid nested field - no parent",
			NewPath("message_field.inner_field2"),
			1,
			&pptpb.Message{},
		},
		{
			"nested primitive field - incorrect type - primitive",
			NewPath("message_field.inner_field"),
			int64(1),
			&pptpb.Message{MessageField: &pptpb.Message_InnerMessage{}},
		},
		{
			"nested primitive field - incorrect type - message",
			NewPath("message_field.inner_field"),
			&pptpb.Message_InnerMessage{},
			&pptpb.Message{MessageField: &pptpb.Message_InnerMessage{}},
		},
		{
			"nested message field - incorrect type - primitive",
			NewPath("message_field.inner_message_field"),
			int64(1),
			&pptpb.Message{MessageField: &pptpb.Message_InnerMessage{}},
		},
		{
			"nested primitive field - incorrect type - message",
			NewPath("message_field.inner_message_field"),
			&pptpb.Message_InnerMessage2{},
			&pptpb.Message{MessageField: &pptpb.Message_InnerMessage{}},
		},
		{
			"repeated field - negative index",
			NewPath("repeated_message_field.-2.inner_field"),
			1,
			&pptpb.Message{RepeatedMessageField: []*pptpb.Message_InnerMessage{{InnerField: 1}}},
		},
		{
			"repeated field - invalid index",
			NewPath("repeated_message_field.foo.inner_field"),
			"code2",
			&pptpb.Message{RepeatedMessageField: []*pptpb.Message_InnerMessage{{InnerField: 1}}},
		},
		{
			"repeated field - index too high",
			NewPath("repeated_message_field.2.inner_field"),
			1,
			&pptpb.Message{RepeatedMessageField: []*pptpb.Message_InnerMessage{{InnerField: 1}}},
		},
		{
			"repeated field - invalid scalar type for message field",
			NewPath("message_field.repeated_inner_field"),
			[]int64{1},
			&pptpb.Message{MessageField: &pptpb.Message_InnerMessage{}},
		},
		{
			"repeated field - invalid message type",
			NewPath("message_field.repeated_inner_field"),
			[]*pptpb.Message{},
			&pptpb.Message{MessageField: &pptpb.Message_InnerMessage{}},
		},
		{
			"repeated field - invalid repeated message type",
			NewPath("repeated_message_field"),
			[]*pptpb.Message{},
			&pptpb.Message{},
		},
		{
			"repeated field - invalid string type for repeated message field",
			NewPath("repeated_message_field"),
			"foo",
			&pptpb.Message{},
		},
		{
			"repeated field - invalid scalar type for repeated scalar field",
			NewPath("message_field.repeated_inner_field"),
			int32(1),
			&pptpb.Message{MessageField: &pptpb.Message_InnerMessage{}},
		},
		{
			"oneof",
			NewPath("foo.inner_field"),
			Zero,
			&pptpb.Message{},
		},
		{
			"oneof - set oneof - missing proto",
			NewPath("oneof"),
			&pptpb.Missing{},
			&pptpb.Message{},
		},
		{
			"oneof - set oneof - missing primitive",
			NewPath("oneof"),
			float32(1.0),
			&pptpb.Message{},
		},
		{
			"oneof - set oneof - duplicate primitive type",
			NewPath("oneof"),
			int32(1),
			&pptpb.Message{},
		},
		{
			"oneof - set oneof - duplicate message type",
			NewPath("oneof"),
			&pptpb.Message_InnerMessage2{},
			&pptpb.Message{},
		},
		{
			"oneof - set case",
			NewPath("oneof.foo"),
			&pptpb.Message_InnerMessage{},
			&pptpb.Message{},
		},
		{
			"complex value",
			NewPath("message_field"),
			struct{}{},
			&pptpb.Message{},
		},
		{
			"enum",
			NewPath("type"),
			"INVALID",
			&pptpb.Message{},
		},
		{
			"missing any",
			NewPath("any_field.inner_field"),
			int32(1),
			&pptpb.Message{},
		},
		{
			"untyped any",
			NewPath("any_field.inner_field"),
			int32(1),
			&pptpb.Message{AnyField: &anypb.Any{}},
		},
		{
			"map - set list value",
			NewPath("map_int32_bytes_field"),
			[][]byte{[]byte("foo")},
			&pptpb.Message{},
		},
		{
			"map - set message value",
			NewPath("map_string_message_field"),
			&pptpb.Message_InnerMessage{},
			&pptpb.Message{},
		},
		{
			"map - set scalar value",
			NewPath("map_string_string_field"),
			"foo",
			&pptpb.Message{},
		},
		{
			"map - invalid key type",
			NewPath("map_int32_bytes_field"),
			map[int64][]byte{1: []byte("foo")},
			&pptpb.Message{},
		},
		{
			"map - invalid value type",
			NewPath("map_string_string_field"),
			map[string]bool{"foo": true},
			&pptpb.Message{},
		},
		{
			"map - invalid key and value types",
			NewPath("map_string_message_field"),
			map[pptpb.MessageType]string{pptpb.MessageType_TYPE_1: "foo"},
			&pptpb.Message{},
		},
		{
			"map key - empty",
			NewPath(`map_string_string_field.`),
			"foo",
			&pptpb.Message{},
		},
		{
			"map key - unparseable type",
			NewPath(`map_int32_bytes_field.$$$`),
			[]byte("foo"),
			&pptpb.Message{},
		},
		{
			"map value - wrong scalar type",
			NewPath(`map_string_string_field."foo"`),
			int32(1),
			&pptpb.Message{MapStringStringField: map[string]string{"foo": "bar"}},
		},
		{
			"map value - scalar for message type",
			NewPath(`map_string_message_field.foo`),
			int32(1),
			&pptpb.Message{},
		},
		{
			"map value - message for scalar type",
			NewPath(`map_string_string_field.foo`),
			&pptpb.Message_InnerMessage{},
			&pptpb.Message{},
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			msg := proto.Clone(test.msg)
			if err := Set(msg, test.path, test.value); err == nil {
				t.Fatalf("Set(%v, %v, %v) got error <nil>, expected error", test.msg, test.path, test.value)
			}
		})
	}
}

func TestGet(t *testing.T) {
	innerMsg := &pptpb.Message_InnerMessage{InnerField: 1}
	tests := []struct {
		name string
		msg  proto.Message
		path Path
		fn   func(proto.Message, Path) (any, error)
		want any
	}{
		{
			name: "concrete type",
			msg: &pptpb.Message{
				Int32: 1,
			},
			path: NewPath("int32"),
			fn: func(m proto.Message, path Path) (any, error) {
				return Get[int32](m, path)
			},
			want: int32(1),
		},
		{
			name: "interface",
			msg: &pptpb.Message{
				MessageField: innerMsg,
			},
			path: NewPath("message_field"),
			fn: func(m proto.Message, path Path) (any, error) {
				return Get[proto.Message](m, path)
			},
			want: innerMsg,
		},
		{
			name: "interface - missing value",
			msg:  &pptpb.Message{},
			path: NewPath("message_field"),
			fn: func(m proto.Message, path Path) (any, error) {
				return Get[proto.Message](m, path)
			},
			want: (*pptpb.Message_InnerMessage)(nil),
		},
		{
			name: "nil message",
			msg:  nil,
			path: NewPath("int32"),
			fn: func(m proto.Message, path Path) (any, error) {
				return Get[int32](m, path)
			},
			want: int32(0),
		},
		{
			name: "nil message - interface",
			msg:  nil,
			path: NewPath("message_field"),
			fn: func(m proto.Message, path Path) (any, error) {
				return Get[proto.Message](m, path)
			},
			want: (proto.Message)(nil),
		},
		{
			name: "proto slice",
			msg:  &pptpb.Message{RepeatedMessageField: []*pptpb.Message_InnerMessage{innerMsg}},
			path: NewPath("repeated_message_field"),
			fn: func(m proto.Message, path Path) (any, error) {
				return Get[[]proto.Message](m, path)
			},
			want: []proto.Message{proto.Message(innerMsg)},
		},
		{
			name: "scalar slice",
			msg:  &pptpb.Message{MessageField: &pptpb.Message_InnerMessage{RepeatedInnerField: []int32{1, 2}}},
			path: NewPath("message_field.repeated_inner_field"),
			fn: func(m proto.Message, path Path) (any, error) {
				return Get[[]int32](m, path)
			},
			want: []int32{1, 2},
		},
		{
			name: "map",
			msg:  &pptpb.Message{MapStringStringField: map[string]string{"foo": "bar"}},
			path: NewPath("map_string_string_field"),
			fn: func(m proto.Message, path Path) (any, error) {
				return Get[map[string]string](m, path)
			},
			want: map[string]string{"foo": "bar"},
		},
		{
			name: "map value - scalar",
			msg:  &pptpb.Message{MapBoolInt32Field: map[bool]int32{false: 7}},
			path: NewPath("map_bool_int32_field.false"),
			fn: func(m proto.Message, path Path) (any, error) {
				return Get[int32](m, path)
			},
			want: int32(7),
		},
		{
			name: "map value - message",
			msg:  &pptpb.Message{MapStringMessageField: map[string]*pptpb.Message_InnerMessage{"foo.bar": innerMsg}},
			path: NewPath(`map_string_message_field."foo.bar"`),
			fn: func(m proto.Message, path Path) (any, error) {
				return Get[proto.Message](m, path)
			},
			want: innerMsg,
		},
		{
			name: "map value - missing key",
			msg:  &pptpb.Message{MapStringStringField: map[string]string{"foo": "bar"}},
			path: NewPath(`map_string_string_field."missing"`),
			fn: func(m proto.Message, path Path) (any, error) {
				return Get[string](m, path)
			},
			want: "",
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			got, err := test.fn(test.msg, test.path)
			if err != nil {
				t.Errorf("Get(%v, %v) returned an unexpected error: %v", test.msg, test.path, err)
			}
			if diff := cmp.Diff(test.want, got, protocmp.Transform()); diff != "" {
				t.Errorf("Get(%v, %v) got unexpected diff(-want +got): %s", test.msg, test.path, diff)
			}
		})
	}
}

func TestGet_Errors(t *testing.T) {
	m := &pptpb.Message{Int32: 1}
	path := NewPath("int32")
	got, err := Get[int64](m, path)
	if err == nil {
		t.Errorf("Get(%v, %v) returned %v, want error", m, path, got)
	}
}

func TestGetWithDefault(t *testing.T) {
	tests := []struct {
		name   string
		path   Path
		defVal any
		msg    proto.Message
		want   any
	}{
		{
			"single field",
			NewPath("message_field"),
			(*pptpb.Message_InnerMessage)(nil),
			&pptpb.Message{MessageField: &pptpb.Message_InnerMessage{InnerField: 1}},
			&pptpb.Message_InnerMessage{InnerField: 1},
		},
		{
			"nested field",
			NewPath("message_field.inner_field"),
			int32(2),
			&pptpb.Message{MessageField: &pptpb.Message_InnerMessage{InnerField: 1}},
			int32(1),
		},
		{
			"repeated field - positive index",
			NewPath("repeated_message_field.0.inner_field"),
			Zero,
			&pptpb.Message{RepeatedMessageField: []*pptpb.Message_InnerMessage{{InnerField: 1}}},
			int32(1),
		},
		{
			"repeated field - end",
			NewPath("repeated_message_field.-1.inner_field"),
			Zero,
			&pptpb.Message{RepeatedMessageField: []*pptpb.Message_InnerMessage{{InnerField: 1}}},
			int32(1),
		},
		{
			"repeated field element",
			NewPath("repeated_message_field.-1"),
			Zero,
			&pptpb.Message{RepeatedMessageField: []*pptpb.Message_InnerMessage{{InnerField: 1}}},
			&pptpb.Message_InnerMessage{InnerField: 1},
		},
		{
			"missing field",
			NewPath("message_field.inner_field"),
			int32(1),
			&pptpb.Message{},
			int32(1),
		},
		{
			"missing field - nil",
			NewPath("message_field.repeated_inner_message_field"),
			(*pptpb.Message_InnerMessage2)(nil),
			&pptpb.Message{},
			(*pptpb.Message_InnerMessage2)(nil),
		},
		{
			"missing field - repeated parent",
			NewPath("message_field.repeated_inner_message_field.-1.inner_field2"),
			Zero,
			&pptpb.Message{},
			int32(0),
		},
		{
			"missing repeated field - parent exists",
			NewPath("repeated_message_field.-1.inner_field"),
			Zero,
			&pptpb.Message{},
			int32(0),
		},
		{
			"missing field - untyped nil",
			NewPath("message_field"),
			nil,
			&pptpb.Message{},
			(*pptpb.Message_InnerMessage)(nil),
		},
		{
			"missing field - nested untyped nil",
			NewPath("message_field"),
			nil,
			&pptpb.Message{},
			(*pptpb.Message_InnerMessage)(nil),
		},
		{
			"missing field - repeated",
			NewPath("repeated_message_field"),
			Zero,
			&pptpb.Message{},
			[]*pptpb.Message_InnerMessage{},
		},
		{
			"missing field - repeated scalar",
			NewPath("message_field.repeated_inner_field"),
			Zero,
			&pptpb.Message{},
			[]int32{},
		},
		{
			"repeated field - index out of bounds",
			NewPath("repeated_message_field.2.repeated_inner_field.0"),
			Zero,
			&pptpb.Message{RepeatedMessageField: []*pptpb.Message_InnerMessage{{}}},
			int32(0),
		},
		{
			"repeated field - not specified - index out of bounds",
			NewPath("repeated_message_field.0.repeated_inner_field.0"),
			Zero,
			&pptpb.Message{},
			int32(0),
		},
		{
			"missing field - zero",
			NewPath("message_field"),
			Zero,
			&pptpb.Message{},
			(*pptpb.Message_InnerMessage)(nil),
		},
		{
			"typed nil message",
			NewPath("message_field.inner_field"),
			int32(1),
			(*pptpb.Message)(nil),
			int32(1),
		},
		{
			"untyped nil message",
			NewPath("message_field.inner_field"),
			int32(1),
			nil,
			int32(1),
		},
		{
			"oneof",
			NewPath("oneof_message_field.inner_field"),
			Zero,
			&pptpb.Message{
				Oneof: &pptpb.Message_OneofMessageField{
					OneofMessageField: &pptpb.Message_InnerMessage{
						InnerField: 1}}},
			int32(1),
		},
		{
			"oneof - zero",
			NewPath("oneof_message_field.inner_field"),
			Zero,
			&pptpb.Message{},
			int32(0),
		},
		{
			"oneof - message",
			NewPath("oneof"),
			Zero,
			&pptpb.Message{
				Oneof: &pptpb.Message_OneofMessageField{
					OneofMessageField: &pptpb.Message_InnerMessage{InnerField: 1},
				},
			},
			&pptpb.Message_InnerMessage{InnerField: 1},
		},
		{
			"oneof and case",
			NewPath("oneof.oneof_message_field"),
			Zero,
			&pptpb.Message{
				Oneof: &pptpb.Message_OneofMessageField{
					OneofMessageField: &pptpb.Message_InnerMessage{InnerField: 1},
				},
			},
			&pptpb.Message_InnerMessage{InnerField: 1},
		},
		{
			"oneof - primitive",
			NewPath("oneof"),
			Zero,
			&pptpb.Message{
				Oneof: &pptpb.Message_OneofPrimitiveField{
					OneofPrimitiveField: true,
				},
			},
			true,
		},
		{
			"oneof - multiple interfaces - ends on concrete type",
			NewPath("oneof_message_field.inner_field"),
			Zero,
			&pptpb.Message{
				Oneof: &pptpb.Message_OneofMessageField{}},
			int32(0),
		},
		{
			"whole repeated field - not empty",
			NewPath("repeated_message_field"),
			Zero,
			&pptpb.Message{RepeatedMessageField: []*pptpb.Message_InnerMessage{{InnerField: 1}}},
			[]*pptpb.Message_InnerMessage{{InnerField: 1}},
		},
		{
			"whole repeated field - empty",
			NewPath("repeated_message_field"),
			Zero,
			&pptpb.Message{RepeatedMessageField: []*pptpb.Message_InnerMessage{}},
			[]*pptpb.Message_InnerMessage{},
		},
		{
			"enum",
			NewPath("type"),
			Zero,
			&pptpb.Message{Type: pptpb.MessageType_TYPE_1},
			pptpb.MessageType_TYPE_1,
		},
		{
			"enum - zero",
			NewPath("type"),
			Zero,
			&pptpb.Message{},
			pptpb.MessageType_INVALID_UNINITIALIZED,
		},
		{
			"enum - default",
			NewPath("type"),
			pptpb.MessageType_TYPE_1,
			&pptpb.Message{},
			pptpb.MessageType_TYPE_1,
		},
		{
			"any",
			NewPath("any_field.inner_field"),
			Zero,
			&pptpb.Message{
				AnyField: mustMarshalAny(t, &pptpb.Message_InnerMessage{
					InnerField: 1,
				}),
			},
			int32(1),
		},
		{
			"nested - any",
			NewPath("any_field.any_field.inner_field"),
			Zero,
			&pptpb.Message{
				AnyField: mustMarshalAny(t, &pptpb.Message{
					AnyField: mustMarshalAny(t, &pptpb.Message_InnerMessage{
						InnerField: 1,
					}),
				}),
			},
			int32(1),
		},
		{
			"map",
			NewPath("map_string_string_field"),
			Zero,
			&pptpb.Message{MapStringStringField: map[string]string{"foo": "bar"}},
			map[string]string{"foo": "bar"},
		},
		{
			"map - proto value",
			NewPath("map_string_message_field"),
			Zero,
			&pptpb.Message{MapStringMessageField: map[string]*pptpb.Message_InnerMessage{}},
			map[string]*pptpb.Message_InnerMessage{},
		},
		{
			"scalar map value",
			NewPath("map_bool_int32_field.true"),
			Zero,
			&pptpb.Message{MapBoolInt32Field: map[bool]int32{true: 3}},
			int32(3),
		},
		{
			"scalar map value - empty",
			NewPath("map_bool_int32_field.true"),
			Zero,
			&pptpb.Message{},
			int32(0),
		},
		{
			"nested map value",
			NewPath("map_string_message_field.foo.map_int32_enum_field.1"),
			Zero,
			&pptpb.Message{MapStringMessageField: map[string]*pptpb.Message_InnerMessage{"foo": &pptpb.Message_InnerMessage{MapInt32EnumField: map[int32]pptpb.MessageType{1: pptpb.MessageType_TYPE_1}}}},
			pptpb.MessageType_TYPE_1,
		},
		{
			"map value - enum",
			NewPath("map_int32_enum_field.1"),
			Zero,
			&pptpb.Message_InnerMessage{MapInt32EnumField: map[int32]pptpb.MessageType{1: pptpb.MessageType_TYPE_1}},
			pptpb.MessageType_TYPE_1,
		},
		{
			"proto map value",
			NewPath("map_string_message_field.`bar`"),
			Zero,
			&pptpb.Message{MapStringMessageField: map[string]*pptpb.Message_InnerMessage{"bar": &pptpb.Message_InnerMessage{InnerField: 1}}},
			&pptpb.Message_InnerMessage{InnerField: 1},
		},
		{
			"missing map value - parent exists",
			NewPath("map_string_message_field.foo.inner_field"),
			Zero,
			&pptpb.Message{},
			int32(0),
		},
		{
			"missing enum map value - parent exists",
			NewPath("message_field.map_int32_enum_field.0"),
			Zero,
			&pptpb.Message{},
			pptpb.MessageType_INVALID_UNINITIALIZED,
		},
		{
			"missing enum map - parent exists",
			NewPath("message_field.map_int32_enum_field"),
			Zero,
			&pptpb.Message{},
			map[int32]pptpb.MessageType{},
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			val, err := GetWithDefault(test.msg, test.path, test.defVal)
			if err != nil {
				t.Fatalf("Get(%v, %v, %v) got err %v, expected <nil>", test.msg, test.path, test.defVal, err)
			}

			if diff := cmp.Diff(val, test.want, protocmp.Transform(), cmpopts.EquateEmpty()); diff != "" {
				t.Errorf("Get(%v, %v, _) => %v, want %v, diff:\n%s", test.msg, test.path, val, test.want, diff)
			}
		})
	}
}

func TestGetWithDefault_Errors(t *testing.T) {
	tests := []struct {
		name   string
		path   Path
		defVal any
		msg    proto.Message
	}{
		{
			"empty path part",
			NewPath("foo."),
			Zero,
			&pptpb.Message{},
		},
		{
			"empty path",
			Path{},
			Zero,
			&pptpb.Message{},
		},
		{
			"invalid field",
			NewPath("message_field2"),
			nil,
			&pptpb.Message{},
		},
		{
			"invalid field - parent exists",
			NewPath("message_field.inner_field.value"),
			Zero,
			&pptpb.Message{MessageField: &pptpb.Message_InnerMessage{InnerField: 1}},
		},
		{
			"invalid nested field - parent exists",
			NewPath("message_field.inner_field2"),
			nil,
			&pptpb.Message{MessageField: &pptpb.Message_InnerMessage{}},
		},
		{
			"field by type - incorrect default value type - primitive",
			NewPath("message_field.inner_field"),
			int64(1),
			&pptpb.Message{},
		},
		{
			"field by type - incorrect default value type - message",
			NewPath("message_field.repeated_inner_message_field"),
			&pptpb.Message_InnerMessage{},
			&pptpb.Message{},
		},
		{
			"field by type - trailing path",
			NewPath("message_field.inner_field.value"),
			Zero,
			&pptpb.Message{},
		},
		{
			"field by type - invalid path",
			NewPath("message_field.inner_field2"),
			Zero,
			&pptpb.Message{},
		},
		{
			"field by type - oneof",
			NewPath("message_field.inner_oneof"),
			Zero,
			&pptpb.Message{},
		},
		{
			"repeated field - invalid index",
			NewPath("repeated_message_field.foo.inner_field"),
			Zero,
			&pptpb.Message{RepeatedMessageField: []*pptpb.Message_InnerMessage{{InnerField: 1}}},
		},
		{
			"unknown oneof",
			NewPath("foo.inner_field"),
			Zero,
			&pptpb.Message{},
		},
		{
			"missing any",
			NewPath("any_field.inner_field"),
			Zero,
			&pptpb.Message{},
		},
		{
			"untyped any",
			NewPath("any_field.inner_field"),
			Zero,
			&pptpb.Message{AnyField: &anypb.Any{}},
		},
		{
			"map key - invalid string",
			NewPath("map_string_string_field.```"),
			Zero,
			&pptpb.Message{MapStringStringField: map[string]string{"1": "2"}},
		},
		{
			"map key - unparseable type",
			NewPath("map_int32_bytes_field.___"),
			Zero,
			&pptpb.Message{MapInt32BytesField: map[int32][]byte{}},
		},
		{
			"map key - wrong type",
			NewPath(`map_string_message_field.foo.map_int32_enum_field.true`),
			Zero,
			&pptpb.Message{MapStringMessageField: map[string]*pptpb.Message_InnerMessage{"foo": &pptpb.Message_InnerMessage{MapInt32EnumField: map[int32]pptpb.MessageType{}}}},
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			if _, err := GetWithDefault(test.msg, test.path, test.defVal); err == nil {
				t.Fatalf("Get(%v, %v, %v) got error <nil>, expected error", test.msg, test.path, test.defVal)
			}
		})
	}
}

func TestString(t *testing.T) {
	tests := []string{
		"normal",
		"multiple.level",
		"with.number.0.1.2.-1",
		"",
	}
	for _, tc := range tests {
		p := NewPath(tc)
		if p.String() != tc {
			t.Errorf("%v.String() != %s", p, tc)
		}
	}
}
