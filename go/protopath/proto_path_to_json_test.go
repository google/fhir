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

package protopath

import (
	"testing"

	pptpb "github.com/google/fhir/go/protopath/protopathtest_go_proto"
	"github.com/google/go-cmp/cmp"
)

func TestToJSONPath(t *testing.T) {
	tests := []struct {
		name string
		path Path
		want string
	}{
		{
			"single path",
			NewPath("string_field"),
			"stringField",
		},
		{
			"nested path",
			NewPath("message_field.inner_field"),
			"messageField.innerField",
		},
		{
			"repeated inner path",
			NewPath("repeated_message_field.0.inner_field"),
			"repeatedMessageField[0].innerField",
		},
		{
			"repeated leaf path",
			NewPath("repeated_message_field.0"),
			"repeatedMessageField[0]",
		},
		{
			"oneof prefix path",
			NewPath("oneof.oneof_message_field.inner_field"),
			"ofType(InnerMessage).innerField",
		},
		{
			"oneof path",
			NewPath("oneof_message_field.inner_field"),
			"ofType(InnerMessage).innerField",
		},
		{
			"nonexistent path",
			NewPath("message_field.invalid"),
			"messageField",
		},
		{
			"invalid path",
			NewPath("foo."),
			"",
		},
		{
			"json annotated",
			NewPath("json_annotated_string_field"),
			"jsonAnnotated",
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			json := ToJSONPath(&pptpb.Message{}, test.path)
			if diff := cmp.Diff(test.want, json); diff != "" {
				t.Errorf("ToJSONPath(_, %v) => %v, want %v, diff:\n%s", test.path, json, test.want, diff)
			}
		})
	}
}
