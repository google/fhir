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

// Package protopath defines methods for getting and setting nested proto
// fields using selectors.
//
// A path is defined as a period-delimited list of Go proto field names, i.e.
// "Msg.Field.SubField". Repeated fields can be indexed using a numeric field
// name, i.e. "Msg.RepeatedField.0.SubField". To refer to the last repeat
// element use the special field name "-1". Setting index "-1" will extend an
// array, not replace the last element.
package protopath

import (
	"errors"
	"fmt"
	"reflect"
	"strconv"
	"strings"

	anypb "google.golang.org/protobuf/types/known/anypb"
	protov1 "github.com/golang/protobuf/proto"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/reflect/protoreflect"
	"google.golang.org/protobuf/reflect/protoregistry"
)

// Zero represents the zero value of any type, it can be used in Set as the
// value argument to set the value at the path to its corresponding zero value,
// or Get as the default value, to return the zero value of the field being
// retrieved if the field is valid but not populated.
const Zero zeroType = 0

// zeroType is the type of Zero and solely used for Zero.
type zeroType int

// Path is a selector for a proto field.
type Path struct {
	parts []protoreflect.Name
	err   error
}

// String implements the Stringer interface, returns a string representation of
// the path.
func (p Path) String() string {
	var strParts []string
	for _, part := range p.parts {
		strParts = append(strParts, string(part))
	}
	return strings.Join(strParts, ".")
}

// NewPath creates a Path from a string definition using proto field names.
func NewPath(p string) Path {
	path := Path{}

	i := 0
	for i < len(p) {
		if i > 0 {
			if p[i] != '.' {
				return Path{err: fmt.Errorf("invalid path %q", p)}
			}
			i++
			if i == len(p) {
				return Path{err: fmt.Errorf("invalid path %q", p)}
			}
		}

		prefix, err := strconv.QuotedPrefix(p[i:])
		if p[i] != '\'' && err == nil { // single-quoted map string keys are not allowed
			prefix, _ = strconv.Unquote(prefix)
			path.parts = append(path.parts, protoreflect.Name(prefix))
			i += len(prefix) + 2
		} else {
			n := strings.IndexRune(p[i:], '.')
			if n == -1 {
				path.parts = append(path.parts, protoreflect.Name(p[i:]))
				break
			} else {
				path.parts = append(path.parts, protoreflect.Name(p[i:n+i]))
				i = n + i
			}
		}
	}
	return path
}

// Append a name to this Path, returns a new Path.
func (p Path) Append(name protoreflect.Name) Path {
	parts := make([]protoreflect.Name, len(p.parts))
	copy(parts, p.parts)
	parts = append(parts, name)
	return Path{parts: parts}
}

// Parent of the current path, returns a new Path.
func (p Path) Parent() Path {
	n := len(p.parts) - 1
	parts := make([]protoreflect.Name, n)
	copy(parts, p.parts[:n])
	return Path{parts: parts}
}

// IsValid reports whether s is a syntactically valid proto path.
// An empty path, or one with blank parts, is invalid.
func (p Path) IsValid() bool {
	return isValidPath(p)
}

func isValidPath(p Path) bool {
	if p.err != nil {
		return false
	}
	if len(p.parts) == 0 {
		p.err = fmt.Errorf("path cannot be empty")
		return false
	}
	for _, part := range p.parts {
		if part == "" {
			p.err = fmt.Errorf("found empty component in path")
			return false
		}
	}
	return true
}

func getMessageField(rpb protoreflect.Message, fieldName protoreflect.Name) (protoreflect.Descriptor, error) {
	desc := rpb.Descriptor()
	f := rpb.Descriptor().Fields().ByName(fieldName)
	if f != nil {
		return f, nil
	}
	oneof := desc.Oneofs().ByName(fieldName)
	if oneof != nil {
		return oneof, nil
	}
	return nil, fmt.Errorf("no field %s in %v", fieldName, desc.FullName())
}

func getSliceElement(m protoreflect.Message, fd protoreflect.FieldDescriptor, idx int, allowExtend bool) (protoreflect.Value, error) {
	slice := m.Get(fd).List()
	if idx == -1 {
		if allowExtend {
			idx = slice.Len()
		} else {
			idx = slice.Len() - 1
		}
	}

	if idx == slice.Len() && allowExtend {
		if !m.Has(fd) {
			slice = m.NewField(fd).List()
		}
		v := slice.NewElement()
		slice.Append(v)
		m.Set(fd, protoreflect.ValueOfList(slice))
		return v, nil
	}
	if idx < 0 || idx >= slice.Len() {
		return protoreflect.Value{}, fmt.Errorf("%v exceeds the bounds of slice %v", idx, slice)
	}
	return slice.Get(idx), nil
}

func getMapKey(fd protoreflect.FieldDescriptor, key string) (protoreflect.MapKey, error) {
	var ret any
	var err error
	switch keyKind := fd.MapKey().Kind(); keyKind {
	case protoreflect.StringKind:
		ret = key
	case protoreflect.BoolKind:
		ret, err = strconv.ParseBool(key)
	case protoreflect.Int32Kind, protoreflect.Sint32Kind, protoreflect.Sfixed32Kind:
		var k int64
		k, err = strconv.ParseInt(key, 0, 32)
		ret = int32(k)
	case protoreflect.Int64Kind, protoreflect.Sint64Kind, protoreflect.Sfixed64Kind:
		ret, err = strconv.ParseInt(key, 0, 64)
	case protoreflect.Uint32Kind, protoreflect.Fixed32Kind:
		var k uint64
		k, err = strconv.ParseUint(key, 0, 32)
		ret = uint32(k)
	case protoreflect.Uint64Kind, protoreflect.Fixed64Kind:
		ret, err = strconv.ParseUint(key, 0, 64)
	default:
		return protoreflect.MapKey{}, fmt.Errorf("map field %v has unsupported key kind %v", fd.FullName(), keyKind)
	}
	if err != nil {
		return protoreflect.MapKey{}, fmt.Errorf("invalid key %q: %v", key, err)
	}
	return protoreflect.ValueOf(ret).MapKey(), nil
}

func getMapElement(m protoreflect.Message, fd protoreflect.FieldDescriptor, k protoreflect.Name, allowAdd bool) (protoreflect.Value, error) {
	key, err := getMapKey(fd, string(k))
	if err != nil {
		return protoreflect.Value{}, err
	}
	mp := m.Get(fd).Map()
	if !m.Has(fd) {
		mp = m.NewField(fd).Map()
	}
	if !mp.Has(key) {
		if allowAdd {
			v := mp.NewValue()
			mp.Set(key, v)
			m.Set(fd, protoreflect.ValueOfMap(mp))
			return v, nil
		}
		return fd.MapValue().Default(), nil
	}
	return mp.Get(key), nil
}

// fillField will populate the struct or slice at the next level of the path if
// the path refers to a field that is currently nil. The new value and path
// will be returned. A new path is returned when selecting into a slice because
// the field returned is an element of the slice.
func fillField(m protoreflect.Message, field protoreflect.FieldDescriptor, path []protoreflect.Name) (protoreflect.Value, []protoreflect.Name, error) {
	if field.IsList() && len(path) > 1 {
		idx, err := strconv.Atoi(string(path[1]))
		if err != nil {
			return protoreflect.Value{}, nil, err
		}
		f, err := getSliceElement(m, field, idx, true)
		if err != nil {
			return f, nil, err
		}
		return f, path[1:], nil
	}
	if field.IsMap() && len(path) > 1 {
		f, err := getMapElement(m, field, path[1], true)
		if err != nil {
			return f, nil, err
		}
		return f, path[1:], nil
	}

	if field.Kind() != protoreflect.MessageKind {
		return m.Get(field), path, nil
	}

	if !m.Has(field) {
		v := m.NewField(field)
		m.Set(field, v)
		return v, path, nil
	}
	return m.Mutable(field), path, nil
}

func getOneOfField(pb protoreflect.Message, oneofField protoreflect.OneofDescriptor, name protoreflect.Name, fill bool) (protoreflect.FieldDescriptor, error) {
	caseField := oneofField.Fields().ByName(name)
	if caseField == nil {
		return nil, fmt.Errorf("could not find field %v in %v", name, oneofField.Name())
	}
	if caseField.Kind() != protoreflect.MessageKind {
		return nil, fmt.Errorf("unexpected oneof field kind: %v", caseField.Kind())
	}

	if pb.Has(caseField) {
		return caseField, nil
	}

	if fill {
		innerMsg := pb.NewField(caseField).Message()
		pb.Set(caseField, protoreflect.ValueOf(innerMsg))
	}
	return caseField, nil
}

func getAnyOneOfField(pb protoreflect.Message, oneofField protoreflect.OneofDescriptor) (any, error) {
	caseField := pb.WhichOneof(oneofField)
	if caseField == nil {
		return nil, fmt.Errorf("%s is empty", oneofField.Name())
	}

	caseValue := pb.Get(caseField)
	if caseField.Kind() == protoreflect.MessageKind {
		return caseValue.Message().Interface(), nil
	}
	return caseValue.Interface(), nil
}

func oneOfFieldByMessageType(m protoreflect.Message, oneOfDesc protoreflect.OneofDescriptor, valPB protoreflect.Message) (protoreflect.FieldDescriptor, error) {
	var messageField protoreflect.FieldDescriptor
	valType := valPB.Descriptor()
	fields := oneOfDesc.Fields()
	for i := 0; i < fields.Len(); i++ {
		f := fields.Get(i)
		if f.Kind() != protoreflect.MessageKind {
			continue
		}
		if canAssignValueToField(m, f, valPB.Interface()) {
			if messageField != nil {
				return nil, fmt.Errorf("multiple fields of %s have type %s", oneOfDesc.FullName(), valType.FullName())
			}
			messageField = f
		}
	}
	if messageField == nil {
		return nil, fmt.Errorf("%s is not an option for %s", valType.FullName(), oneOfDesc.FullName())
	}
	return messageField, nil
}

func getEnumValueByName(ed protoreflect.EnumDescriptor, val string) (protoreflect.EnumNumber, bool) {
	enumVals := ed.Values()
	for i := 0; i < enumVals.Len(); i++ {
		enumVal := enumVals.Get(i)
		if string(enumVal.Name()) == val {
			return enumVal.Number(), true
		}
	}
	return 0, false
}

func canAssignValueToField(m protoreflect.Message, fd protoreflect.FieldDescriptor, val any) bool {
	fdKind := fd.Kind()
	if val == nil {
		return fdKind == protoreflect.MessageKind || fdKind == protoreflect.BytesKind || fd.IsList() || fd.IsMap()
	}

	valType := reflect.TypeOf(val)
	if fdKind == protoreflect.BytesKind {
		return valType.AssignableTo(reflect.TypeOf([]byte{}))
	}

	if valType.Kind() == reflect.Slice {
		if !fd.IsList() {
			return false
		}
		valType = valType.Elem()
	}
	if valType.Kind() == reflect.Map {
		if !fd.IsMap() {
			return false
		}
		k := reflect.ValueOf(fd.MapKey().Default().Interface())
		v := reflect.ValueOf(fd.MapValue().Default().Interface())
		if !k.IsValid() || !v.IsValid() {
			return false
		}
		return valType.Key().AssignableTo(k.Type()) && valType.Elem().AssignableTo(v.Type())
	}

	if valType.Kind() == reflect.Ptr {
		rpb, ok := valAsReflectMessage(reflect.Zero(valType).Interface())
		if !ok {
			return false
		}
		if fd.IsMap() {
			return fd.MapValue().Message() == rpb.Descriptor()
		}
		return fd.Message() == rpb.Descriptor()
	} else if fdKind == protoreflect.EnumKind {
		switch val := val.(type) {
		case protoreflect.Enum:
			return fd.Enum() == val.Descriptor()
		case string:
			_, ok := getEnumValueByName(fd.Enum(), val)
			return ok
		default:
			return false
		}
	}

	var def reflect.Value
	if fd.IsList() {
		def = reflect.ValueOf(m.Get(fd).List().NewElement().Interface())
	} else if fd.IsMap() {
		def = reflect.ValueOf(fd.MapValue().Default().Interface())
	} else {
		def = reflect.ValueOf(fd.Default().Interface())
	}
	if !def.IsValid() {
		return false
	}
	return valType.AssignableTo(def.Type())
}

func valAsReflectMessage(val any) (protoreflect.Message, bool) {
	switch msgVal := val.(type) {
	case proto.Message:
		return msgVal.ProtoReflect(), true
	case protov1.Message:
		return protov1.MessageReflect(msgVal), true
	default:
		return nil, false
	}
}

func oneOfFieldByPrimitiveType(m protoreflect.Message, oneOfDesc protoreflect.OneofDescriptor, val any) (protoreflect.FieldDescriptor, error) {
	var typeField protoreflect.FieldDescriptor
	valType := reflect.TypeOf(val)
	fields := oneOfDesc.Fields()
	for i := 0; i < fields.Len(); i++ {
		f := fields.Get(i)
		if canAssignValueToField(m, f, val) {
			if typeField != nil {
				return nil, fmt.Errorf("multiple fields of %s have type %s", oneOfDesc.FullName(), valType.Name())
			}
			typeField = f
		}
	}
	if typeField == nil {
		return nil, fmt.Errorf("%s is not an option for %s", valType.Name(), oneOfDesc.FullName())
	}
	return typeField, nil
}

func setOneOfFieldByType(m protoreflect.Message, oneOfDesc protoreflect.OneofDescriptor, val any) error {
	var innerField protoreflect.FieldDescriptor
	var err error
	if rpb, ok := valAsReflectMessage(val); ok {
		val = rpb
		innerField, err = oneOfFieldByMessageType(m, oneOfDesc, rpb)
	} else {
		innerField, err = oneOfFieldByPrimitiveType(m, oneOfDesc, val)
	}
	if err != nil {
		return err
	}
	rval := protoreflect.ValueOf(val)
	m.Set(innerField, rval)
	return nil
}

func setAny(anyMsg *anypb.Any, value any, path []protoreflect.Name) error {
	if anyMsg.GetTypeUrl() == "" {
		return fmt.Errorf("cannot return a value on an untyped Any")
	}
	inner, err := anyMsg.UnmarshalNew()
	if err != nil {
		return err
	}
	if err := set(inner.ProtoReflect(), value, path); err != nil {
		return err
	}
	modifiedAny, err := anypb.New(inner)
	if err != nil {
		return err
	}
	anyMsg.Value = modifiedAny.Value
	return nil
}

func set(m protoreflect.Message, value any, path []protoreflect.Name) error {
	fieldDesc, err := getMessageField(m, path[0])
	if err != nil {
		if anyMsg, ok := m.Interface().(*anypb.Any); ok {
			return setAny(anyMsg, value, path)
		}
		return err
	}
	var oneOfDesc protoreflect.OneofDescriptor
	// Special case for when getMessageField results in a oneof using a proto
	// path.
	if d, ok := fieldDesc.(protoreflect.OneofDescriptor); ok {
		oneOfDesc = d
	}

	var fd protoreflect.FieldDescriptor
	var v protoreflect.Value
	if oneOfDesc != nil {
		if len(path) == 1 {
			return setOneOfFieldByType(m, oneOfDesc, value)
		}
		fd, err = getOneOfField(m, oneOfDesc, path[1], true)
		if err != nil {
			return err
		}
		v = m.Mutable(fd)
		path = path[1:]
	} else {
		fd = fieldDesc.(protoreflect.FieldDescriptor)
		if v, path, err = fillField(m, fd, path); err != nil {
			return err
		}
	}
	if len(path) > 1 {
		m, ok := v.Interface().(protoreflect.Message)
		if !ok {
			return fmt.Errorf("found trailing path for scalar at %s", path[0])
		}
		return set(m, value, path[1:])
	}
	return assignValue(m, fd, path, value)
}

func assignValue(m protoreflect.Message, fd protoreflect.FieldDescriptor, path []protoreflect.Name, value any) error {
	// Allow Zero to enables us to set proto fields to zero regardless of the
	// underlying type.
	if value == Zero {
		if !fd.IsMap() || fd.TextName() == string(path[0]) {
			m.Clear(fd)
		}
		return nil
	}

	if !canAssignValueToField(m, fd, value) {
		defVal, err := goValueFromProtoValue(fd, fd.Default())
		if err != nil {
			return err
		}
		return fmt.Errorf("cannot assign %T to %T", value, defVal)
	}
	v := protoValueFromGoValue(m, fd, value)
	_, valIsMap := v.Interface().(protoreflect.Map)
	_, valIsList := v.Interface().(protoreflect.List)
	if (!fd.IsList() && !fd.IsMap()) || valIsList || valIsMap || value == nil {
		if v.IsValid() {
			m.Set(fd, v)
		} else {
			m.Clear(fd)
		}
		return nil
	}

	if fd.IsList() {
		i, err := strconv.Atoi(string(path[0]))
		if err != nil {
			// Last element of path is not a valid index.
			defVal, err := goValueFromProtoValue(fd, fd.Default())
			if err != nil {
				return err
			}
			return fmt.Errorf("cannot assign %T to %T", value, defVal)
		}
		slice := m.Get(fd).List()
		// index has already been validated by `getSliceElement`, and the slice was
		// extended, but we have to convert it again here.
		if i == -1 {
			i = slice.Len() - 1
		}
		slice.Set(i, v)
	} else {
		if fd.TextName() == string(path[0]) {
			return fmt.Errorf("cannot assign %T to map", value)
		}
		key, err := getMapKey(fd, string(path[0]))
		if err != nil {
			return err
		}
		mp := m.Get(fd).Map()
		mp.Set(key, v)
	}
	return nil
}

func protoValueFromGoValue(m protoreflect.Message, fd protoreflect.FieldDescriptor, i any) protoreflect.Value {
	switch v := i.(type) {
	case string:
		if fd.Kind() == protoreflect.EnumKind {
			if enumVal, ok := getEnumValueByName(fd.Enum(), v); ok {
				return protoreflect.ValueOfEnum(enumVal)
			}
		}
		return protoreflect.ValueOf(v)
	case nil, bool, int32, int64, uint32, uint64, float32, float64,
		[]byte, protoreflect.EnumNumber, protoreflect.Message, protoreflect.List,
		protoreflect.Map:
		return protoreflect.ValueOf(v)
	case proto.Message:
		rpb := v.ProtoReflect()
		if !rpb.IsValid() {
			return protoreflect.Value{}
		}
		return protoreflect.ValueOfMessage(rpb)
	case protov1.Message:
		rpb := protov1.MessageReflect(v)
		if !rpb.IsValid() {
			return protoreflect.Value{}
		}
		return protoreflect.ValueOfMessage(rpb)
	case protoreflect.Enum:
		return protoreflect.ValueOfEnum(v.Number())
	}
	rv := reflect.ValueOf(i)
	if fd.IsList() {
		slice := m.NewField(fd).List()
		for i := 0; i < rv.Len(); i++ {
			e := rv.Index(i)
			eVal := protoValueFromGoValue(m, fd, e.Interface())
			slice.Append(eVal)
		}
		return protoreflect.ValueOfList(slice)
	}

	mp := m.NewField(fd).Map()
	iter := rv.MapRange()
	for iter.Next() {
		kVal := protoValueFromGoValue(m, fd, iter.Key().Interface())
		vVal := protoValueFromGoValue(m, fd, iter.Value().Interface())
		mp.Set(kVal.MapKey(), vVal)
	}
	return protoreflect.ValueOfMap(mp)
}

func goValueFromProtoValue(fd protoreflect.FieldDescriptor, v protoreflect.Value) (any, error) {
	switch v := v.Interface().(type) {
	case protoreflect.Message:
		return v.Interface(), nil
	case protoreflect.List:
		e, err := goValueFromProtoValue(fd, v.NewElement())
		if err != nil {
			return nil, err
		}
		slice := reflect.MakeSlice(reflect.SliceOf(reflect.TypeOf(e)), v.Len(), v.Len())
		for i := 0; i < v.Len(); i++ {
			v, err := goValueFromProtoValue(fd, v.Get(i))
			if err != nil {
				return nil, err
			}
			slice.Index(i).Set(reflect.ValueOf(v))
		}
		return slice.Interface(), nil
	case protoreflect.Map:
		key, err := goValueFromProtoValue(fd, protoreflect.ValueOf(fd.MapKey().Default().Interface()))
		if err != nil {
			return nil, err
		}
		var val any
		if _, ok := v.NewValue().Interface().(protoreflect.Message); ok {
			val = v.NewValue().Message().Interface()
		} else {
			val, err = goValueFromProtoValue(fd, fd.MapValue().Default())
			if err != nil {
				return nil, err
			}
		}
		mp := reflect.MakeMap(reflect.MapOf(reflect.TypeOf(key), reflect.TypeOf(val)))
		var mErr error
		v.Range(func(mKey protoreflect.MapKey, mVal protoreflect.Value) bool {
			var k, e any
			k, mErr = goValueFromProtoValue(fd, mKey.Value())
			if mErr != nil {
				return false
			}
			e, mErr = goValueFromProtoValue(fd, protoreflect.Value(mVal))
			if mErr != nil {
				return false
			}
			mp.SetMapIndex(reflect.ValueOf(k), reflect.ValueOf(e))
			return true
		})
		if mErr != nil {
			return nil, mErr
		}
		return mp.Interface(), nil
	case protoreflect.EnumNumber:
		var n protoreflect.FullName
		if fd.IsMap() {
			n = fd.MapValue().Enum().FullName()
		} else {
			n = fd.Enum().FullName()
		}
		enum, err := protoregistry.GlobalTypes.FindEnumByName(n)
		if err != nil {
			return nil, err
		}
		return enum.New(v), nil
	default:
		return v, nil
	}
}

// Set sets the value of a proto at `path`. An error will occur if the path is
// invalid, or the specified value's type is incompatible with the type of the
// field at `path`.
//
// If the last value of `path` is a oneof then the field of the oneof that is
// assignable to value will be set. If multiple fields have the same type an
// error is returned.
//
// If the input message is nil, an error will be returned.
func Set(m proto.Message, path Path, value any) error {
	if !isValidPath(path) {
		return fmt.Errorf("invalid path %v", path)
	}
	if m == nil {
		return errors.New("cannot call Set() on nil message")
	}
	r := m.ProtoReflect()
	if !r.IsValid() {
		return errors.New("cannot call Set() on nil message")
	}
	return set(r, value, path.parts)
}

func getDefaultValueAtPath(m protoreflect.Message, fd protoreflect.FieldDescriptor, path []protoreflect.Name) (protoreflect.Message, protoreflect.FieldDescriptor, error) {
	if len(path) == 0 {
		return m, fd, nil
	}

	t := fd.Message()
	v := m.NewField(fd)
	if slice, ok := v.Interface().(protoreflect.List); ok {
		m = slice.NewElement().Message()
	} else if mp, ok := v.Interface().(protoreflect.Map); ok {
		m = mp.NewValue().Message()
		t = m.Descriptor()
	} else {
		m = v.Message()
	}
	if _, ok := m.Interface().(*anypb.Any); ok {
		return nil, nil, fmt.Errorf("cannot return a default value for an untyped Any")
	}

	var ft protoreflect.FieldDescriptor
	oneOfDesc := t.Oneofs().ByName(path[0])
	if oneOfDesc == nil {
		ft = t.Fields().ByName(path[0])
	} else {
		return nil, nil, fmt.Errorf("cannot return default value for oneof %s in %s", path[0], t.FullName())
	}
	if ft == nil {
		return nil, nil, fmt.Errorf("invalid field %s in %s", path[0], t.FullName())
	}
	if len(path) == 1 {
		return m, ft, nil
	}
	if ft.Kind() != protoreflect.MessageKind && !ft.IsList() && !ft.IsMap() {
		return nil, nil, fmt.Errorf("found trailing path for scalar at %s", path[0])
	}
	if ft.IsList() {
		path = path[1:]
		_, err := strconv.Atoi(string(path[0]))
		if err != nil {
			return nil, nil, err
		}
	} else if ft.IsMap() {
		path = path[1:]
		if err := validateMapKeyInPath(ft, string(path[0])); err != nil {
			return nil, nil, err
		}
		val := m.NewField(ft).Map().NewValue()
		if _, ok := val.Interface().(protoreflect.Message); !ok {
			return m, ft, nil
		}
		ft = ft.MapValue()
		m = val.Message()
	}
	return getDefaultValueAtPath(m, ft, path[1:])
}

func validateMapKeyInPath(fd protoreflect.FieldDescriptor, kStr string) error {
	key, err := getMapKey(fd, kStr)
	if err != nil {
		return err
	}
	k := reflect.ValueOf(fd.MapKey().Default().Interface())
	if !k.IsValid() {
		return fmt.Errorf("found invalid map key type at %s", kStr)
	}
	if !reflect.TypeOf(key.Interface()).AssignableTo(k.Type()) {
		return fmt.Errorf("found invalid map key type at %s", kStr)
	}
	return nil
}

func checkDefaultValue(m protoreflect.Message, fd protoreflect.FieldDescriptor, path []protoreflect.Name, defVal any) (any, error) {
	p := path
	if fd.IsMap() || fd.IsList() {
		p = p[2:]
	}
	m, ft, err := getDefaultValueAtPath(m, fd, p)
	if err != nil {
		return nil, err
	}
	if (ft.IsMap() || ft.IsList()) && ft.TextName() != string(path[len(path)-1]) {
		if defVal == Zero {
			rv, _ := goValueFromProtoValue(ft, m.NewField(ft))
			return reflect.Zero(reflect.TypeOf(rv).Elem()).Interface(), nil
		}
		if !canAssignValueToField(m, ft, defVal) {
			return nil, fmt.Errorf("invalid type %T for default value, expected %v", defVal, ft.Name())
		}
		return defVal, nil
	}

	// Allow untyped nil pointers which enables us to set proto fields regardless
	// of the underlying type.
	if defVal == Zero || defVal == nil && ft.Kind() == protoreflect.MessageKind {
		rv, _ := goValueFromProtoValue(ft, m.NewField(ft))
		// NewField creates a new struct instead of nil.
		if ft.Kind() == protoreflect.MessageKind {
			return reflect.Zero(reflect.TypeOf(rv)).Interface(), nil
		}
		return rv, nil
	}
	if !canAssignValueToField(m, ft, defVal) {
		return nil, fmt.Errorf("invalid type %T for default value, expected %v", defVal, ft.Name())
	}
	return defVal, nil
}

func getAny(anyMsg *anypb.Any, defVal any, path []protoreflect.Name) (any, error) {
	if anyMsg.GetTypeUrl() == "" {
		return nil, fmt.Errorf("cannot return a value from an untyped Any")
	}
	inner, err := anyMsg.UnmarshalNew()
	if err != nil {
		return nil, err
	}
	return get(inner.ProtoReflect(), defVal, path)
}

func get(m protoreflect.Message, defVal any, path []protoreflect.Name) (any, error) {
	field, err := getMessageField(m, path[0])
	if err != nil {
		if anyMsg, ok := m.Interface().(*anypb.Any); ok {
			return getAny(anyMsg, defVal, path)
		}
		return nil, err
	}

	// Special case for when getMessageField results in a oneof using a proto
	// path.
	var oneOfDesc protoreflect.OneofDescriptor
	if d, ok := field.(protoreflect.OneofDescriptor); ok {
		oneOfDesc = d
	}

	var v protoreflect.Value
	var fd protoreflect.FieldDescriptor
	if oneOfDesc != nil {
		if len(path) == 1 {
			return getAnyOneOfField(m, oneOfDesc)
		}
		fd, err = getOneOfField(m, oneOfDesc, path[1], false)
		if err != nil {
			return nil, err
		}
		if m.Has(fd) {
			v = m.Get(fd)
		}
		path = path[1:]
	} else {
		fd = field.(protoreflect.FieldDescriptor)
		if m.Has(fd) || (fd.IsList() || fd.IsMap()) && len(path) == 1 {
			if fd.IsList() && len(path) > 1 {
				idx, err := strconv.Atoi(string(path[1]))
				if err != nil {
					return nil, err
				}
				if idx < -1 || idx >= m.Get(fd).List().Len() {
					return checkDefaultValue(m, fd, path, defVal)
				}
				v, err = getSliceElement(m, fd, idx, false)
				if err != nil {
					return nil, err
				}
				path = path[1:]
			} else if fd.IsMap() && len(path) > 1 {
				v, err = getMapElement(m, fd, path[1], false)
				if err != nil {
					return nil, err
				}
				if _, ok := v.Interface().(protoreflect.Message); ok {
					m = v.Message()
					fd = fd.MapValue()
				}
				path = path[1:]
			} else {
				v = m.Get(fd)
			}
		} else if fd.IsList() {
			_, err := strconv.Atoi(string(path[1]))
			if err != nil {
				return nil, err
			}
			return checkDefaultValue(m, fd, path, defVal)
		} else if fd.IsMap() {
			if err := validateMapKeyInPath(fd, string(path[1])); err != nil {
				return nil, err
			}
			return checkDefaultValue(m, fd, path, defVal)
		}
	}

	if !v.IsValid() {
		return checkDefaultValue(m, fd, path[1:], defVal)
	}
	if len(path) == 1 {
		return goValueFromProtoValue(fd, v)
	}
	m, ok := v.Interface().(protoreflect.Message)
	if !ok {
		return nil, fmt.Errorf("found trailing path for scalar at %s", path[0])
	}
	return get(m, defVal, path[1:])
}

// Get retrieves a value from a proto at `path`, or returns the zero value of
// that field if any of the parent fields are missing. An error will occur if the path is invalid.
//
// If the last value of `path` is a oneof then the populated field of the oneof
// will be returned. An error will be returned if the oneof is not populated.
//
// Any T such that the value at path is assignable to T is valid. If T is a
// slice then the elements of path must be assignable to T. Reflection is used
// to cast each of the elements, so the performance implications of this should
// be taken into account. If this is not desired simply use `any` for the type
// of T.
func Get[T any](m proto.Message, path Path) (T, error) {
	return getUntyped[T](m, path, Zero)
}

// GetWithDefault retrieves a value from a proto at `path`, or returns a default value. An
// error will occur if the path is invalid, or the default value's type is
// incompatible with the type of the field at `path`.
//
// If the last value of `path` is a oneof then the populated field of the oneof
// will be returned. `defVal` is ignored in this case, an error will be returned
// if the oneof is not populated.
//
// If the input message is nil, the default value will be returned.
//
// Any T such that the value at path is assignable to T is valid. If T is a
// slice then the elements of path must be assignable to T. Reflection is used
// to cast each of the elements, so the performance implications of this should
// be taken into account. If this is not desired simply use `any` for the type
// of T.
func GetWithDefault[T any](m proto.Message, path Path, defVal T) (T, error) {
	return getUntyped[T](m, path, defVal)
}

func getUntyped[T any](m proto.Message, path Path, defVal any) (T, error) {
	var zero T
	if !isValidPath(path) {
		return zero, fmt.Errorf("invalid path %v", path)
	}
	if m == nil {
		if defVal == Zero {
			return zero, nil
		}
		defValT, ok := defVal.(T)
		if !ok {
			return zero, fmt.Errorf("invalid type %T for default value", defVal)
		}
		return defValT, nil
	}
	val, err := get(m.ProtoReflect(), defVal, path.parts)
	if err != nil {
		return zero, err
	}
	// Take a reference + Elem so this works with interfaces too
	retType := reflect.TypeOf(&zero).Elem()
	valType := reflect.TypeOf(val)
	if valType.AssignableTo(retType) {
		return val.(T), nil
	}
	if retType.Kind() == reflect.Slice && valType.Kind() == reflect.Slice && valType.Elem().AssignableTo(retType.Elem()) {
		retElemType := retType.Elem()
		valRef := reflect.ValueOf(val)
		ret := reflect.MakeSlice(retType, 0, valRef.Len())
		for i := 0; i < valRef.Len(); i++ {
			ret = reflect.Append(ret, valRef.Index(i).Convert(retElemType))
		}
		return ret.Interface().(T), nil
	}
	if retType.Kind() == reflect.Map && valType.Kind() == reflect.Map && valType.Elem().AssignableTo(retType.Elem()) {
		valRef := reflect.ValueOf(val)
		ret := reflect.MakeMap(retType)
		iter := valRef.MapRange()
		for iter.Next() {
			ret.SetMapIndex(iter.Key(), iter.Value())
		}
		return ret.Interface().(T), nil
	}
	return zero, fmt.Errorf("value at path is a %T, want %T", val, zero)
}
