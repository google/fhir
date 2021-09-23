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
package protopath

import (
	"testing"

	"github.com/google/go-cmp/cmp"
	"github.com/google/go-cmp/cmp/cmpopts"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/testing/protocmp"

	pptpb "github.com/google/fhir/go/jsonformat/internal/protopath/protopathtest_go_proto"
)

func TestSet(t *testing.T) {
	tests := []struct {
		name  string
		path  Path
		value interface{}
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
		value interface{}
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
	tests := []struct {
		name   string
		path   Path
		defVal interface{}
		msg    proto.Message
		want   interface{}
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
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			val, err := Get(test.msg, test.path, test.defVal)
			if err != nil {
				t.Fatalf("Get(%v, %v, %v) got err %v, expected <nil>", test.msg, test.path, test.defVal, err)
			}

			if diff := cmp.Diff(val, test.want, protocmp.Transform(), cmpopts.EquateEmpty()); diff != "" {
				t.Errorf("Get(%v, %v, _) => %v, want %v, diff:\n%s", test.msg, test.path, val, test.want, diff)
			}
		})
	}
}

func TestGet_Errors(t *testing.T) {
	tests := []struct {
		name   string
		path   Path
		defVal interface{}
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
			"repeated field - negative index",
			NewPath("repeated_message_field.-2.inner_field"),
			int32(1),
			&pptpb.Message{RepeatedMessageField: []*pptpb.Message_InnerMessage{{InnerField: 1}}},
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
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			if _, err := Get(test.msg, test.path, test.defVal); err == nil {
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
