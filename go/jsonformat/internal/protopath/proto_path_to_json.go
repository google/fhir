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
	"strings"

	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/reflect/protoreflect"
)

// ToJSONPath converts the `path` corresponding to the `proto` in to the JSON
// path, providing a cleaner way to return paths to users for debugging. Therefore,
// errors are ignored and conversion is simply aborted early.
func ToJSONPath(p proto.Message, path Path) string {
	if !isValidPath(path) {
		return ""
	}
	return strings.Join(toJSON(p.ProtoReflect().Descriptor(), path.parts), ".")
}

func toJSON(md protoreflect.MessageDescriptor, path []protoreflect.Name) []string {
	var jsonParts []string
	if len(path) == 0 {
		return jsonParts
	}

	name := path[0]
	path = path[1:]

	// Find the valid FieldDescriptor by name.
	fd := md.Fields().ByName(name)
	if fd == nil {
		od := md.Oneofs().ByName(name)
		// If this is a oneOf, we move one down into the oneOf as a oneOf doesn't
		// have a MessageDescriptor to pass on the next call.
		if od != nil && len(path) >= 1 {
			fd = od.Fields().ByName(path[0])
			path = path[1:]
		}
		if fd == nil {
			return jsonParts
		}
	}

	// Extract the JSON representation of the FieldDescriptor.
	var jsonName string
	if fd.ContainingOneof() != nil {
		jsonName = "ofType(" + string(fd.Message().Name()) + ")"
	} else {
		jsonName = fd.JSONName()
	}

	if jsonName != "" && fd.Cardinality() == protoreflect.Repeated && len(path) >= 1 {
		jsonName += "[" + string(path[0]) + "]"
		path = path[1:]
	}

	if jsonName != "" {
		jsonParts = append(jsonParts, jsonName)
	}
	return append(jsonParts, toJSON(fd.Message(), path)...)
}
