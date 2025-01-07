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

// Package accessor contains the getter/setter functions to access reflected FHIR proto messages.
package accessor

import (
	"fmt"
	"reflect"
	"strings"

	"github.com/golang/protobuf/proto"
	"google.golang.org/protobuf/reflect/protoreflect"
)

func firstFieldDesc(ty protoreflect.MessageDescriptor, fields ...string) (protoreflect.FieldDescriptor, []string, error) {
	if len(fields) == 0 {
		return nil, nil, fmt.Errorf("none given field, at least one field is required")
	}
	f := fields[0]
	fd := ty.Fields().ByName(protoreflect.Name(f))
	fields = fields[1:]
	if fd == nil {
		// try with oneof fields
		oneof := ty.Oneofs().ByName(protoreflect.Name(f))
		if oneof == nil {
			return nil, nil, fmt.Errorf("field: %v not found in %v", f, ty.FullName())
		}
		if len(fields) < 1 {
			return nil, nil, fmt.Errorf("not enough fields given to get field through 'oneof': %v in %v", f, ty.FullName())
		}
		f2 := fields[0]
		fields = fields[1:]
		fd = oneof.Fields().ByName(protoreflect.Name(f2))
		if fd == nil {
			return nil, nil, fmt.Errorf("field: %v not found in 'oneof' field %v of %v", f2, f, ty.FullName())
		}
	}
	return fd, fields, nil
}

func getNonRepeatedFieldValue(rpb protoreflect.Message, fields []string, validKinds ...protoreflect.Kind) (protoreflect.Value, error) {
	var err error
	if len(fields) == 0 {
		return protoreflect.Value{}, fmt.Errorf("none given field, at least one field is required")
	}
	if len(fields) > 1 {
		rpb, err = getMessage(rpb, false, fields[0:len(fields)-1]...)
		if err != nil {
			return protoreflect.Value{}, fmt.Errorf("getting parent message: %w", err)
		}
	}
	f := fields[len(fields)-1]
	fd := rpb.Descriptor().Fields().ByName(protoreflect.Name(f))
	if fd == nil {
		return protoreflect.Value{}, fmt.Errorf("field: %v not found in %v", f, rpb.Descriptor().FullName())
	}
	isValidKind := false
	for _, k := range validKinds {
		if fd.Kind() == k {
			isValidKind = true
			break
		}
	}
	if !isValidKind {
		return protoreflect.Value{}, fmt.Errorf("field: %v in %v is of kind: %v, valid kinds are: %v", fd.Name(), rpb.Descriptor().FullName(), fd.Kind(), validKinds)
	}
	if fd.IsList() {
		return protoreflect.Value{}, fmt.Errorf("field: %v in %v is repeated", fd.Name(), rpb.Descriptor().FullName())
	}
	if fd.IsMap() {
		return protoreflect.Value{}, fmt.Errorf("field: %v in %v is a map", fd.Name(), rpb.Descriptor().FullName())
	}
	return rpb.Get(fd), nil
}

// getMessage returns the reflected message at the field pointed by the path specified by the given
// fields in the given reflected message. Returns error if the pointed field or any intermediate
// fields are either repeated, or not a proto message. If populateIfMissing is set to true, empty
// message will be created and set to the given reflected message, if any intermediate fields or the
// pointed field is not populated. If populateIfMissing is set to false, in cases of reaching
// an unpopulated field, error will be returned.
func getMessage(rpb protoreflect.Message, populateIfMissing bool, fields ...string) (protoreflect.Message, error) {
	fd, fields, err := firstFieldDesc(rpb.Descriptor(), fields...)
	if err != nil {
		return nil, fmt.Errorf("getting field descriptor: %w", err)
	}
	if fd.Kind() != protoreflect.MessageKind {
		return nil, fmt.Errorf("field: %v in %v is not message type", fd.Name(), rpb.Descriptor().FullName())
	}
	if fd.IsList() {
		return nil, fmt.Errorf("field: %v in %v is repeated", fd.Name(), rpb.Descriptor().FullName())
	}
	if fd.IsMap() {
		return nil, fmt.Errorf("field: %v in %v is a map", fd.Name(), rpb.Descriptor().FullName())
	}
	if !rpb.Has(fd) {
		if !populateIfMissing {
			return nil, fmt.Errorf("field: %v in %v is not populated", fd.Name(), rpb.Descriptor().FullName())
		}
		emptyMsg := reflect.New(proto.MessageType(string(fd.Message().FullName())).Elem()).Interface().(proto.Message)
		rpb.Set(fd, protoreflect.ValueOf(proto.MessageReflect(emptyMsg)))
	}
	ret := rpb.Get(fd).Message()
	if len(fields) > 0 {
		ret, err = getMessage(ret, populateIfMissing, fields...)
		if err != nil {
			return nil, fmt.Errorf("field %v: %w", fd.Name(), err)
		}
	}
	return ret, nil
}

// GetMessage takes a reflected proto message and a list of field names as path, returns the
// nesting message pointed by the path, in the form of reflected proto. The field names are the
// names defined in the original proto message, not the ones defined in the generated Go code.
// GetMessage can handle OneOf fields, given either the inner concrete field name or the oneof
// field name AND the inner concrete field name.
// GetMessage returns error in following cases:
//  1. No field is given.
//  2. The nested field pointed by the path is not of message type (i.e. the field is of primitive
//     types like string, []byte, int, etc)
//  3. The nested field pointed by the path is a repeated field or is a map.
//  4. The path points to/through a field that does not exist.
//  5. The path points to/through a field that is not populated.
//
// Example, assume we have a fully populated Foo message: foo, and its reflection: rfoo:
//
//	message Foo {
//	  message Bar {
//	    message Xyz {
//	      string field_a;
//	    }
//	    Xyz field_b;
//	  }
//	  Bar field_c;
//	  repeated Bar field_d;
//	  message Bar2 {
//	    string bar2_content
//	  }
//	  oneof field_one_of {
//	    Bar field_one_of_bar;
//	    Bar2 field_one_of_bar2;
//	  }
//	}
//
// GetMessage(rfoo, "field_c") returns:
//
//	reflected: foo.FieldC
//
// GetMessage(rfoo, "field_c", "field_b") returns:
//
//	reflected: foo.FieldC.FieldB
//
// GetMessage(rfoo, "field_c", "field_b", "field_a") returns:
//
//	error as the path points to a non-message type field
//
// GetMessage(rfoo, "not_exist") returns:
//
//	error as not_exist is not a valid field
//
// GetMessage(rfoo, "field_d") returns:
//
//	error as the path points to a repeated field
//
// GetMessage(rfoo, "field_one_of", "field_one_of_bar") returns:
//
//	foo.GetFieldOneOfBar() if field_one_of_bar is populated, otherwise error
//
// GetMessage(rfoo, "field_one_of_bar") also returns:
//
//	foo.GetFieldOneOfBar() if field_one_of_bar is populated, otherwise error
func GetMessage(rpb protoreflect.Message, fields ...string) (protoreflect.Message, error) {
	ret, err := getMessage(rpb, false, fields...)
	if err != nil {
		return nil, fmt.Errorf("getting message: %w", err)
	}
	return ret, nil
}

// GetList takes a reflected proto message and a list of field names as path, returns the nesting
// repeated field pointed by the path, in the form of reflected list. The field names are the
// names defined in the original proto message, not the ones defined in the generated Go code. The
// path, except the last field, must not go through any repeated or map fields, and all the
// intermediate fields must be of message type (i.e. cannot be of primitive types). However, the
// final target field can be a repeated field of either primitive types or message types.
// GetList returns error in following cases:
// 1. No field is given (empty path)
// 2. If len(fields) > 1 and failed to get the parent message of the target list(see GetMessage)
// 3. Fields contain invalid fields
// 4. Field pointed by the path is not of repeated
//
// Example: assume we have a fully populated Foo message: foo, and its reflection: rfoo
//
//	message Foo {
//		message Bar {
//			repeated string field_a;
//		}
//		repeated Bar field_b;
//	 Bar field_c;
//	}
//
// GetList(rfoo, "field_b") returns:
//
//	foo.FieldB
//
// GetList(rfoo, "field_b", "field_a") returns:
//
//	error, as field_b is a repeated field and intermediate fields must not be repeated
//
// GetList(rfoo, "field_c", "field_a") returns:
//
//	foo.FieldC.FieldA
func GetList(rpb protoreflect.Message, fields ...string) (protoreflect.List, error) {
	var err error
	if len(fields) == 0 {
		return nil, fmt.Errorf("none given field, at least one field is required")
	}
	if len(fields) > 1 {
		rpb, err = getMessage(rpb, false, fields[0:len(fields)-1]...)
		if err != nil {
			return nil, fmt.Errorf("getting parent message: %w", err)
		}
	}
	f := fields[len(fields)-1]
	fd := rpb.Descriptor().Fields().ByName(protoreflect.Name(f))
	if fd == nil {
		return nil, fmt.Errorf("field: %v not found in %v", f, rpb.Descriptor().FullName())
	}
	if fd.Cardinality() != protoreflect.Repeated {
		return nil, fmt.Errorf("field: %v in %v is not repeated", f, rpb.Descriptor().FullName())
	}
	if !rpb.Has(fd) {
		return rpb.Mutable(fd).List(), nil
	}
	return rpb.Get(fd).List(), nil
}

// GetString takes a reflected proto message and a list of fields as path, returns the string
// pointed by the path. The path must not go through any repeated or map fields, and all the
// intermediate fields must be of message type (i.e. cannot be of primitive types). The last field
// MUST be a field of Go string type. If the string field is not populated, empty string will be
// returned.
// GetString returns error in following cases:
// 1. No field is given (empty path)
// 2. If len(fields) > 1 and failed to get the parent message of the target string (see GetMessage)
// 3. Fields contain invalid fields
// 4. Target field is not of Go string type.
// 5. Target field is repeated.
//
// Example: assume we have a fully populated Foo message: foo, and its reflection: rfoo
//
//	message Foo {
//	 string field_a;
//		message Bar {
//			string field_b;
//		}
//	 Bar field_c;
//	 repeated string field_d;
//	}
//
// GetString(rfoo, "field_a") returns:
//
//	foo.FieldA's content
//
// GetString(rfoo, "field_c", "field_b") returns:
//
//	foo.FieldC.FieldB's content
//
// GetString(rfoo, "field_c") returns:
//
//	error, as field_c is not a field of string type
//
// GetString(rfoo, "field_d") returns:
//
//	error, as field_d is a repeated field.
func GetString(rpb protoreflect.Message, fields ...string) (string, error) {
	fval, err := getNonRepeatedFieldValue(rpb, fields, protoreflect.StringKind)
	if err != nil {
		return "", fmt.Errorf("getting field value: %w", err)
	}
	return fval.String(), nil
}

// GetInt64 takes a reflected proto message and a list of fields as path, returns the int64 pointed
// by the path. The path must not go through any repeated or map fields, and all the intermediate
// fields must be of message type (i.e. cannot be of primitive types). And the last field MUST point
// to a field with one of the 64-bit int kinds. If the field is not populated, int64(0) will be
// returned.
// GetInt64 returns error in following cases:
// 1. No field is given (empty path)
// 2. If len(fields) > 1 and failed to get the parent message of the target int64 (see GetMessage)
// 3. Fields contain invalid fields
// 4. Target field is not of 64-bit int kind.
// 5. Target field is repeated.
//
// Example: assume we have a fully populated Foo message: foo, and its reflection: rfoo
//
//	message Foo {
//	 int64 field_a;
//		message Bar {
//			int64 field_b;
//		}
//	 Bar field_c;
//	 repeated int64 field_d;
//	}
//
// GetInt64(rfoo, "field_a") returns:
//
//	foo.FieldA
//
// GetInt64(rfoo, "field_c", "field_b") returns:
//
//	foo.FieldC.FieldB
//
// GetInt64(rfoo, "field_c") returns:
//
//	error, as field_c is not a field of 64-bit int kind
//
// GetInt64(rfoo, "field_d") returns:
//
//	error, as field_d is a repeated field.
func GetInt64(rpb protoreflect.Message, fields ...string) (int64, error) {
	fval, err := getNonRepeatedFieldValue(rpb, fields, protoreflect.Int64Kind, protoreflect.Sint64Kind, protoreflect.Sfixed64Kind)
	if err != nil {
		return 0, fmt.Errorf("getting field value: %w", err)
	}
	return fval.Int(), nil
}

// GetUint32 takes a reflected proto message and a list of fields as path, returns the uint32
// pointed by the path. The path must not go through any repeated or map fields, and all the
// intermediate fields must be of message type (i.e. cannot be of primitive types). And the last
// field MUST point to a field with one of the 32-bit uint kinds. If the field is not populated,
// uint32(0) will be returned.
// GetUint32 returns error in following cases:
// 1. No field is given (empty path)
// 2. If len(fields) > 1 and failed to get the parent message of the target uint32 (see GetMessage)
// 3. Fields contain invalid fields
// 4. Target field is not of 32-bit uint kind.
// 5. Target field is repeated.
//
// Example: assume we have a fully populated Foo message: foo, and its reflection: rfoo
//
//	message Foo {
//	 uint32 field_a;
//		message Bar {
//			uint32 field_b;
//		}
//	 Bar field_c;
//	 repeated uint32 field_d;
//	}
//
// GetUint32(rfoo, "field_a") returns:
//
//	foo.FieldA
//
// GetUint32(rfoo, "field_c", "field_b") returns:
//
//	foo.FieldC.FieldB
//
// GetUint32(rfoo, "field_c") returns:
//
//	error, as field_c is not a field of 32-bit uint kind
//
// GetUint32(rfoo, "field_d") returns:
//
//	error, as field_d is a repeated field.
func GetUint32(rpb protoreflect.Message, fields ...string) (uint32, error) {
	fval, err := getNonRepeatedFieldValue(rpb, fields, protoreflect.Uint32Kind, protoreflect.Fixed32Kind)
	if err != nil {
		return 0, fmt.Errorf("getting field value: %w", err)
	}
	return uint32(fval.Uint()), nil
}

// GetBytes takes a reflected proto message and a list of fields as path, returns the []byte pointed
// by the path. The path must not go through any repeated or map fields, and all the intermediate
// fields must be of message type (i.e. cannot be of primitive types). And the last field MUST field
// of []byte type. If the field is not populated, empty byte slice will be returned.
// GetBytes returns error in following cases:
// 1. No field is given (empty path)
// 2. If len(fields) > 1 and failed to get the parent message of the target []byte (see GetMessage)
// 3. Fields contain invalid fields
// 4. Target field is not of bytes kind.
// 5. Target field is repeated.
//
// Example: assume we have a fully populated Foo message: foo, and its reflection: rfoo
//
//	message Foo {
//	 bytes field_a;
//		message Bar {
//			bytes field_b;
//		}
//	 Bar field_c;
//	 repeated bytes field_d;
//	}
//
// GetBytes(rfoo, "field_a") returns:
//
//	foo.FieldA
//
// GetBytes(rfoo, "field_c", "field_b") returns:
//
//	foo.FieldC.FieldB
//
// GetBytes(rfoo, "field_c") returns:
//
//	error, as field_c is not a field of bytes kind, but message kind
//
// GetBytes(rfoo, "field_d") returns:
//
//	error, as field_d is a repeated field.
func GetBytes(rpb protoreflect.Message, fields ...string) ([]byte, error) {
	fval, err := getNonRepeatedFieldValue(rpb, fields, protoreflect.BytesKind)
	if err != nil {
		return nil, fmt.Errorf("getting field value: %w", err)
	}
	return fval.Bytes(), nil
}

// GetEnumNumber takes a reflected proto message and a list of fields as path, returns the enum
// number pointed by the path. The path must not go through any repeated or map fields, and all the
// intermediate fields must be of message type (i.e. cannot be of primitive types). And the last
// field MUST be a field of enum type. If the field is not populated, 0 will be returned.
// GetEnumNumber returns error in following cases:
// 1. No field is given (empty path)
// 2. If len(fields) > 1 and failed to get the parent message of the target enum (see GetMessage)
// 3. Fields contain invalid fields
// 4. Target field is not of enum kind.
// 5. Target field is repeated.
//
// Example: assume we have a fully populated Foo message: foo, and its reflection: rfoo
//
//	message Foo {
//	 string field_a;
//		Enum Fruit {
//			APPLE = 0;
//	   ORANGE = 1;
//		}
//	 Fruit field_b;
//	 Message Bar {
//	   Fruit field_c
//	 }
//	 Bar field_d;
//	 repeated Fruit field_e;
//	}
//
// GetEnumNumber(rfoo, "field_a") returns:
//
//	error, as field_a is not a field of enum type.
//
// GetEnumNumber(rfoo, "field_b") returns:
//
//	the number value of foo.FieldB
//
// GetEnumNumber(rfoo, "field_d", "field_c") returns:
//
//	the number value of foo.FieldD.FieldC
//
// GetEnumNumber(rfoo, "field_e") returns:
//
//	error, as field_e is a repeated field.
func GetEnumNumber(rpb protoreflect.Message, fields ...string) (protoreflect.EnumNumber, error) {
	fval, err := getNonRepeatedFieldValue(rpb, fields, protoreflect.EnumKind)
	if err != nil {
		return 0, fmt.Errorf("getting field value: %w", err)
	}
	return fval.Enum(), nil
}

// GetEnumDescriptor takes a proto message descriptor and a list of fields as path, returns the enum
// descriptor of the field pointed by the path. The path must not go through any repeated or map
// fields, and all the intermediate fields must be of message type (i.e. cannot be of primitive
// types). And the last field MUST be a field of enum type.
// GetEnumDescriptor returns error in following cases:
// 1. No field is given (empty path)
// 2. If len(fields) > 1 and failed to get the parent message descriptor of the target enum (see GetMessage)
// 3. Fields contain invalid fields
// 4. Target field is not of enum kind.
// 5. Target field is repeated.
//
// Example: assume we have a Foo message
//
//	message Foo {
//	 string field_a;
//		Enum Fruit {
//			APPLE = 0;
//	   ORANGE = 1;
//		}
//	 Fruit field_b;
//	 Message Bar {
//	   Fruit field_c
//	 }
//	 Bar field_d;
//	 repeated Fruit field_e;
//	}
//
// GetEnumDescriptor(Foo, "field_a") returns:
//
//	error, as field_a is not a field of enum type.
//
// GetEnumDescriptor(Foo, "field_b") returns:
//
//	the enum descriptor of Foo_Fruit
//
// GetEnumDescriptor(Foo, "field_d", "field_c") returns:
//
//	the enum descriptor of Foo_Fruit
//
// GetEnumDescriptor(Foo, "field_e") returns:
//
//	error, as field_e is a repeated field.
func GetEnumDescriptor(ty protoreflect.MessageDescriptor, fields ...string) (protoreflect.EnumDescriptor, error) {
	var err error
	fd, fields, err := firstFieldDesc(ty, fields...)
	if err != nil {
		return nil, fmt.Errorf("getting field descriptor for getting enum descriptor: %w", err)
	}
	if fd.IsList() {
		return nil, fmt.Errorf("field: %v in %v is repeated", fd.Name(), ty.FullName())
	}
	if fd.IsMap() {
		return nil, fmt.Errorf("field: %v in %v is a map", fd.Name(), ty.FullName())
	}
	if len(fields) > 0 {
		if fd.Kind() != protoreflect.MessageKind {
			return nil, fmt.Errorf("field: %v in %v is not message type", fd.Name(), ty.FullName())
		}
		ed, err := GetEnumDescriptor(fd.Message(), fields...)
		if err != nil {
			return nil, fmt.Errorf("field %q: %w", fd.Name(), err)
		}
		return ed, nil
	}
	if fd.Kind() != protoreflect.EnumKind {
		return nil, fmt.Errorf("field: %v in %v is not of enum kind", fd.Name(), ty.FullName())
	}
	return fd.Enum(), nil
}

func getParentMessage(rpb protoreflect.Message, populateIfMissing bool, fields ...string) (protoreflect.Message, error) {
	if len(fields) == 0 {
		return nil, fmt.Errorf("none given field, at least one field is required")
	}
	if len(fields) == 1 {
		return rpb, nil
	}
	strip1, err := getMessage(rpb, populateIfMissing, fields[0:len(fields)-1]...)
	if err == nil {
		return strip1, nil
	}
	if !strings.Contains(err.Error(), "not enough fields given to get field through 'oneof'") {
		return nil, err
	}
	if len(fields) <= 2 {
		return rpb, nil
	}
	return getMessage(rpb, true, fields[0:len(fields)-2]...)
}

// SetValue sets a value to a nesting field specified by the given fields in the given reflected
// proto message. The path specified by the fields must not go through any repeated or map fields.
// The target field can be primitive type field (including enum fields) or message type field. When
// setting primitive type field values, the given value must be convertible to the field concrete
// Go type. When setting message type values, the given value must be exactly the same type as the
// field. The target field must NOT be a repeated or map field.
//
// Example: assume we have a Foo message: foo, and its reflection: rfoo
//
//	message Foo {
//	 int64 field_a;
//		message Bar {
//			int64 field_b;
//		}
//	 Bar field_c;
//	 repeated int64 field_d;
//	}
//
// SetValue(rfoo, 123, "field_a") sets the foo.FieldA to 123
// SetValue(rfoo, &Foo_Bar{FieldB: 123}, "field_c"} sets the foo.FieldC to &Foo_Bar{FieldB: 123}
// SetValue(rfoo, 123, "field_c", "field_b") sets the foo.FieldC.FieldB to 123. And if FieldC is
//
//	not populated ahead, a Foo_Bar message will be created and set to foo.FieldC.
//
// SetValue(rfoo, "str", "field_a") returns error as value/field type mismatch
// SetValue(rfoo, "str", "not_exist") returns error as field not_exist is not valid
// SetValue(rfoo, 123, "field_d") returns error as field_d is repeated
func SetValue(rpb protoreflect.Message, value interface{}, fields ...string) error {
	var err error
	rpb, err = getParentMessage(rpb, true, fields...)
	if err != nil {
		return fmt.Errorf("setting value: %w", err)
	}
	f := fields[len(fields)-1]
	fd, _, err := firstFieldDesc(rpb.Descriptor(), f)
	if err != nil {
		return fmt.Errorf("setting value: %w", err)
	}
	if fd.IsList() {
		return fmt.Errorf("field: %v in %v is repeated", fd.Name(), rpb.Descriptor().FullName())
	}
	if fd.IsMap() {
		return fmt.Errorf("field: %v in %v is a map", fd.Name(), rpb.Descriptor().FullName())
	}
	valueGoType := reflect.TypeOf(value)
	valueGoKind := valueGoType.Kind()
	switch fd.Kind() {
	// primitive and enum, add more primitive kinds if necessary
	case protoreflect.BoolKind,
		protoreflect.StringKind,
		protoreflect.EnumKind,
		protoreflect.Sint32Kind,
		protoreflect.Uint32Kind,
		protoreflect.Int64Kind,
		protoreflect.Sint64Kind:
		// this works because the field is guaranteed to be not repeated
		defaultValue := fd.Default().Interface()
		defaultGoKind := reflect.TypeOf(defaultValue).Kind()
		if defaultGoKind != valueGoKind {
			return fmt.Errorf("go kind mismatch, field: %v in %v is of kind: %v, given value is of kind: %v",
				fd.Name(), rpb.Descriptor().FullName(), defaultGoKind, valueGoKind)
		}
		converted := reflect.ValueOf(value).Convert(reflect.TypeOf(defaultValue)).Interface()
		rpb.Set(fd, protoreflect.ValueOf(converted))
		return nil

	// bytes
	case protoreflect.BytesKind:
		if valueGoType != reflect.TypeOf([]byte{}) {
			return fmt.Errorf("type mismatch, given value is not a byte slice ([]byte type)")
		}
		rpb.Set(fd, protoreflect.ValueOf(value))
		return nil

	// message
	case protoreflect.MessageKind:
		pv, ok := value.(proto.Message)
		if !ok {
			return fmt.Errorf("type mismatch, given value is not a proto message: %T", value)
		}
		fieldProtoName := fd.Message().FullName()
		valueProtoName := proto.MessageReflect(pv).Descriptor().FullName()
		if fieldProtoName != valueProtoName {
			return fmt.Errorf("type mismatch, field %v in %v is a message of type: %v, given value is a message of type: %v",
				fd.Name(), rpb.Descriptor().FullName(), fieldProtoName, valueProtoName)
		}
		rpb.Set(fd, protoreflect.ValueOf(proto.MessageReflect(pv)))
		return nil

	// not supported
	default:
		return fmt.Errorf("proto kind: %v is not supported", fd.Kind())
	}
}

// AppendValue appends the given value to the field pointed by the path specified through the given
// fields in the given reflected proto message. The path specified by the fields must not go through
// any repeated or map fields. The target field must be a repeated field (see GetList). The list can
// be of either primitive types (including enum) or message types. If the field is primitive type,
// the given value must be convertible to the field type. If the field is message type, the given
// value must be exactly the same type as the field. The target field must not be a map.
//
// Example: assume we have a Foo message: foo, and its reflection: rfoo
//
//	message Foo {
//	 repeated int64 field_a;
//		message Bar {
//			repeated int64 field_b;
//		}
//	 repeated Bar field_c;
//	 Bar field_d;
//	}
//
// AppendValue(rfoo, 123, "field_a") append int64(123) to the foo.FieldA slice
// AppendValue(rfoo, &Foo_Bar{FieldB: []int64{123}}, "field_c"} append the new Foo_Bar to the foo.FieldC slice
// AppendValue(rfoo, "str", "field_a") returns error as value/field type mismatch
// AppendValue(rfoo, "str", "not_exist") returns error as field not_exist is not valid
// AppendValue(rfoo, 123, "field_c", "field_b") returns error as field_c is repeated as an intermediate field
// AppendValue(rfoo, 123, "field_d", "field_b") sets int64(123) to the foo.FieldD.FieldB slice.
func AppendValue(rpb protoreflect.Message, value interface{}, fields ...string) error {
	var err error
	rpb, err = getParentMessage(rpb, true, fields...)
	if err != nil {
		return fmt.Errorf("appending value: %w", err)
	}
	f := fields[len(fields)-1]
	list, err := GetList(rpb, f)
	if err != nil {
		return fmt.Errorf("appending value: %w", err)
	}
	fd := rpb.Descriptor().Fields().ByName(protoreflect.Name(f))
	// GetList has already checked fd is not nil
	valueGoType := reflect.TypeOf(value)
	valueGoKind := valueGoType.Kind()
	switch fd.Kind() {
	// primitive and enum, add more primitive kinds if necessary
	case protoreflect.BoolKind,
		protoreflect.StringKind,
		protoreflect.EnumKind,
		protoreflect.Uint32Kind,
		protoreflect.Int64Kind:
		goType, err := getGoType(fd.Kind())
		if err != nil {
			return fmt.Errorf("appending value: %w", err)
		}
		goKind := goType.Kind()
		if goKind != valueGoKind {
			return fmt.Errorf("go kind mismatch, field: %v in %v is repeated of kind: %v, given value is of kind: %v",
				fd.Name(), rpb.Descriptor().FullName(), goKind, valueGoKind)
		}
		converted := reflect.ValueOf(value).Convert(goType).Interface()
		list.Append(protoreflect.ValueOf(converted))
		return nil

	// message
	case protoreflect.MessageKind:
		pv, ok := value.(proto.Message)
		if !ok {
			return fmt.Errorf("type mismatch, given value is not a proto message: %T", value)
		}
		fieldProtoName := fd.Message().FullName()
		valueProtoName := proto.MessageReflect(pv).Descriptor().FullName()
		if fieldProtoName != valueProtoName {
			return fmt.Errorf("type mismatch, field: %v in %v is a list of messages of type: %v, given value is a message of type: %v",
				fd.Name(), rpb.Descriptor().FullName(), fieldProtoName, valueProtoName)
		}
		list.Append(protoreflect.ValueOf(proto.MessageReflect(pv)))
		return nil
	default:
		return fmt.Errorf("proto kind: %v is not supported", fd.Kind())
	}
}

func getGoType(kind protoreflect.Kind) (reflect.Type, error) {
	switch kind {
	case protoreflect.BoolKind:
		return reflect.TypeOf(false), nil
	case protoreflect.StringKind:
		return reflect.TypeOf(""), nil
	case protoreflect.EnumKind:
		return reflect.TypeOf(int32(0)), nil
	case protoreflect.Uint32Kind:
		return reflect.TypeOf(uint32(0)), nil
	case protoreflect.Int64Kind:
		return reflect.TypeOf(int64(0)), nil
	default:
		return nil, fmt.Errorf("proto kind: %v is not supported", kind)
	}
}
