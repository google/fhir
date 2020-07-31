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
	"strings"
	"sync"
	"unicode"

	"google.golang.org/protobuf/reflect/protoreflect"
)

const (
	// ContainedField is the JSON field name of inline resources.
	ContainedField = "contained"
	// ResourceTypeField constant.
	ResourceTypeField = "resourceType"
	// OneofName field constant.
	OneofName = "oneof_resource"
	// Extension field constant.
	Extension = "extension"
)

// IsJSON defines JSON related interface.
type IsJSON interface {
	// IsJSON method.
	IsJSON()
}

// JSONObject defines JSON map.
type JSONObject map[string]IsJSON

// IsJSON implementation of JSON object.
func (JSONObject) IsJSON() {}

// JSONArray defines JSON array.
type JSONArray []IsJSON

// IsJSON implementation of JSON array.
func (JSONArray) IsJSON() {}

// JSONString defines JSON string.
type JSONString string

// IsJSON implementation of JSON string.
func (JSONString) IsJSON() {}

// JSONRawValue defines JSON bytes.
type JSONRawValue []byte

// IsJSON implementation of JSON bytes.
func (JSONRawValue) IsJSON() {}

// MarshalJSON marshals JSON to bytes.
func (v JSONRawValue) MarshalJSON() ([]byte, error) {
	return v, nil
}

var (
	nameMapCache sync.Map // type map[string]*map[string]string
)

// SnakeToCamel converts a snake_case string into a CamelCase string.
func SnakeToCamel(snakeCase string) string {
	camelCase := make([]rune, 0, len(snakeCase))
	upper := true
	for _, c := range snakeCase {
		if c == '_' {
			upper = true
		} else if upper {
			upper = false
			camelCase = append(camelCase, unicode.ToUpper(c))
		} else {
			camelCase = append(camelCase, unicode.ToLower(c))
		}
	}
	return string(camelCase)
}

// CamelToSnake converts a CamelCase string into snake_case by putting one '_' character before each uppercase char and lowercasing all characters.
func CamelToSnake(camelCase string) string {
	snakeCase := make([]rune, 0, len(camelCase))
	for _, c := range camelCase {
		if !unicode.IsLower(c) {
			if len(snakeCase) > 0 {
				snakeCase = append(snakeCase, '_')
			}
		}
		snakeCase = append(snakeCase, unicode.ToLower(c))
	}
	return string(snakeCase)
}

func buildNameMap(d protoreflect.MessageDescriptor) *map[string]string {
	m := &map[string]string{}
	fields := d.Fields()
	for i := 0; i < fields.Len(); i++ {
		f := fields.Get(i)
		if f.HasJSONName() {
			(*m)[CamelToSnake(f.JSONName())] = string(f.Name())
		}
	}
	return m
}

func getNameMap(d protoreflect.MessageDescriptor) *map[string]string {
	n := d.FullName()
	if v, ok := nameMapCache.Load(n); ok {
		return v.(*map[string]string)
	}
	v, _ := nameMapCache.LoadOrStore(n, buildNameMap(d))
	return v.(*map[string]string)
}

// GetField returns the field descriptor.
func GetField(d protoreflect.MessageDescriptor, f string) protoreflect.FieldDescriptor {
	s := CamelToSnake(f)

	if fd := d.Fields().ByName(protoreflect.Name(s)); fd != nil {
		return fd
	}
	// Try again, as there may be a field whose json_name matches fd.
	return d.Fields().ByName(protoreflect.Name((*getNameMap(d))[s]))
}

// SnakeToLowerCamel converts the snake case to lower camel case.
func SnakeToLowerCamel(s string) string {
	if len(s) == 0 {
		return s
	}
	camel := SnakeToCamel(s)
	return strings.ToLower(camel[:1]) + camel[1:]
}
