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

package jsonformat

import (
	"encoding/base64"
	"encoding/json"
	"fmt"
	"sort"
	"strings"

	"github.com/google/fhir/go/jsonformat/internal/accessor"
	"github.com/google/fhir/go/jsonformat/internal/jsonpbhelper"
	"github.com/google/fhir/go/jsonformat/internal/protopath"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/reflect/protoreflect"
)

// protoToExtension converts a FHIR extension proto (e.g. Base64BinarySeparatorStride or
// PrimitiveHasNoValue) into a Google-defined FHIR extension describing the proto.
func protoToExtension(pb, ext proto.Message) error {
	rpb := pb.ProtoReflect()
	url, err := jsonpbhelper.InternalExtensionURL(rpb.Descriptor())
	if err != nil {
		return err
	}
	ret := ext.ProtoReflect()
	if err := accessor.SetValue(ret, url, "url", "value"); err != nil {
		return fmt.Errorf("setting url value in a new extension proto: %v, err: %w", ret.Interface(), err)
	}

	// Iterate over fields in deterministic order by sorting by number.
	type descAndVal struct {
		desc protoreflect.FieldDescriptor
		val  protoreflect.Value
	}
	var setFields []descAndVal
	rpb.Range(func(fd protoreflect.FieldDescriptor, val protoreflect.Value) bool {
		setFields = append(setFields, descAndVal{fd, val})
		return true
	})
	sort.Slice(setFields, func(i, j int) bool { return setFields[i].desc.Number() < setFields[j].desc.Number() })

	// Add an extension for each populated primitive type proto field.
	for i := range setFields {
		f, val := setFields[i].desc, setFields[i].val

		d := f.Message()
		if d == nil {
			return fmt.Errorf("unexpected field type %v, want Message type", f.Kind())
		}
		if !jsonpbhelper.IsPrimitiveType(d) {
			return fmt.Errorf("unexpected field %v of non-primitive type %v", f.Name(), d.Name())
		}
		if f.Cardinality() == protoreflect.Repeated {
			return fmt.Errorf("field %v is repeated", f.Name())
		}
		if len(setFields) == 1 {
			switch d.Name() {
			case "Boolean":
				if err := accessor.SetValue(ret, val.Message().Interface(), "value", "choice", "boolean"); err != nil {
					return fmt.Errorf("setting boolean to extension value in extension: %v, err: %w", ret.Interface(), err)
				}
			default:
				return fmt.Errorf("unsupported message type: %v", d.Name())
			}
			return nil
		}

		e := ret.New()
		if err := accessor.SetValue(e, string(f.Name()), "url", "value"); err != nil {
			return err
		}
		switch d.Name() {
		case "String":
			if err := accessor.SetValue(e, val.Message().Interface(), "value", "choice", "string_value"); err != nil {
				return fmt.Errorf("setting string to extension value in extension: %v, err: %w", e.Interface(), err)
			}
		case "PositiveInt":
			if err := accessor.SetValue(e, val.Message().Interface(), "value", "choice", "positive_int"); err != nil {
				return fmt.Errorf("setting poisitive integer to extension value in extension: %v, err: %w", e.Interface(), err)
			}
		default:
			return fmt.Errorf("unsupported message type: %v", f.Message().Name())
		}
		if err := accessor.AppendValue(ret, e.Interface().(proto.Message), "extension"); err != nil {
			return fmt.Errorf("appending extension to extension list in extension: %v, err: %w", ret.Interface(), err)
		}
	}
	return nil
}

// parseDecimal parses a FHIR decimal data object into a Decimal proto message, m.
func parseDecimal(decimal json.RawMessage, m proto.Message) error {
	mr := m.ProtoReflect()
	fn := mr.Descriptor().FullName()
	regex, has := jsonpbhelper.RegexValues[fn]
	if !has {
		return fmt.Errorf("regex not found for %v type", fn)
	}
	if !regex.MatchString(string(decimal)) {
		return fmt.Errorf("invalid decimal: %v", decimal)
	}
	if err := accessor.SetValue(mr, string(decimal), "value"); err != nil {
		return err
	}
	return nil
}

// Base64BinarySeparatorStrideCreator defines a type of functions that, given the separator string
// and stride value, returns a new Base64BinarySeparatorStride proto.
type base64BinarySeparatorStrideCreator func(sep string, stride uint32) proto.Message

type base64Data struct {
	data   []byte
	stride int
	sep    int
}

// filterBase64Spaces removes spaces according to the stride/separator encoding. An error is returned for inconsistent stride and separator lengths.
// The return values are: the final length of the string, the detected stride and separator lengths, and an error if one occurred.
func filterBase64Spaces(p []byte) (nn, stride, sep int, err error) {
	n := len(p)
	chunkStart := 0
	for i := 0; i < n; i++ {
		c := p[i]
		if c != ' ' {
			if i != nn {
				p[nn] = c
			}
			nn++
			continue
		}

		curStride := i - chunkStart
		if stride == 0 {
			stride = curStride
		}
		if curStride != stride {
			return 0, 0, 0, fmt.Errorf("found stride of length %d, previous stride was %d", curStride, stride)
		}

		chunkStart = i
		for ; i < n && p[i] == ' '; i++ {
		}

		curSep := i - chunkStart
		if sep == 0 {
			sep = curSep
		}
		if curSep != sep {
			return 0, 0, 0, fmt.Errorf("found separator of length %d, previous separator was %d", curSep, sep)
		}
		chunkStart = i
		i--
	}
	return nn, stride, sep, nil
}

func decodeBase64(data []byte) (base64Data, error) {
	n, stride, sep, err := filterBase64Spaces(data)
	if err != nil {
		return base64Data{}, err
	}
	data = data[:n]
	n, err = base64.StdEncoding.Decode(data, data)
	if err != nil {
		return base64Data{}, err
	}
	return base64Data{
		data:   data[:n],
		stride: stride,
		sep:    sep,
	}, nil
}

// parseBinary parses a FHIR Binary resource object into a Binary proto message, m.
func parseBinary(binary json.RawMessage, m proto.Message, createSepStride base64BinarySeparatorStrideCreator) error {
	if len(binary) < 2 || binary[0] != '"' || binary[len(binary)-1] != '"' {
		return fmt.Errorf("binary data is not a string")
	}
	val, err := decodeBase64(binary[1 : len(binary)-1])
	if err != nil {
		return err
	}
	if val.stride > 0 {
		mr := m.ProtoReflect()
		extList, err := accessor.GetList(mr, "extension")
		if err != nil {
			return err
		}
		ext := extList.NewElement().Message().Interface().(proto.Message)
		sepAndStride := createSepStride(strings.Repeat(" ", val.sep), uint32(val.stride))
		if err := protoToExtension(sepAndStride, ext); err != nil {
			return err
		}
		if err := protopath.Set(m, protopath.NewPath("extension.-1"), ext); err != nil {
			return err
		}
	}
	return protopath.Set(m, protopath.NewPath("value"), val.data)
}

// serializeBinary serializes proto representation of a FHIR Binary data object into a JSON message.
func serializeBinary(binary proto.Message) (string, error) {
	ext, err := jsonpbhelper.GetExtension(binary, jsonpbhelper.Base64BinarySeparatorStrideURL)
	if err != nil {
		return "", err
	}
	rBinary := binary.ProtoReflect()
	binVal, err := accessor.GetBytes(rBinary, "value")
	if err != nil {
		return "", err
	}
	encoded := base64.StdEncoding.EncodeToString(binVal)
	if ext == nil {
		return encoded, nil
	}
	extList, err := accessor.GetList(ext.ProtoReflect(), "extension")
	if err != nil {
		return "", err
	}
	sep := ""
	stride := 0
	for i := 0; i < extList.Len(); i++ {
		e := extList.Get(i)
		urlVal, err := accessor.GetString(e.Message(), "url", "value")
		if err != nil {
			return "", err
		}
		switch urlVal {
		case "separator":
			sep, err = accessor.GetString(e.Message(), "value", "choice", "string_value", "value")
			if err != nil {
				return "", err
			}
		case "stride":
			ustride, err := accessor.GetUint32(e.Message(), "value", "choice", "positive_int", "value")
			if err != nil {
				return "", err
			}
			stride = int(ustride)
		}
	}
	ret := ""
	pos := 0
	for ; (pos+1)*stride < len(encoded); pos++ {
		ret = ret + encoded[pos*stride:(pos+1)*stride] + sep
	}
	ret = ret + encoded[pos*stride:]
	return ret, nil
}
