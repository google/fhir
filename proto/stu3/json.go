/*
 * Copyright 2018 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Custom JSON marshalling for the FHIR primitive data types.

package stu3

import (
	"encoding/base64"
	"fmt"
	"regexp"
	"strconv"
	"strings"

	"github.com/golang/protobuf/descriptor"
	"github.com/golang/protobuf/jsonpb"
	"github.com/golang/protobuf/proto"
)

const (
	doubleQuote        = `"`
	escapedDoubleQuote = `\"`
)

// jsonString returns s as a JSON string in double quotes, escaping any existing
// quotes.
func jsonString(s string) []byte {
	return []byte(fmt.Sprintf(
		`"%s"`,
		strings.Replace(s, doubleQuote, escapedDoubleQuote, -1),
	))
}

// parseJSONString strips surrounding double quotes and unescapes any remaining
// quotes.
func parseJSONString(buf []byte) (string, error) {
	n := len(buf)
	if n < 2 || buf[0] != '"' || buf[n-1] != '"' {
		return "", fmt.Errorf("invalid JSON string %s", buf)
	}
	s := string(buf[1 : n-1])
	return strings.Replace(s, escapedDoubleQuote, doubleQuote, -1), nil
}

// reMatch checks if the JSON string matches the regular expression in the
// proto option "value_regex".
func reMatch(msg descriptor.Message, json []byte) error {
	_, md := descriptor.ForMessage(msg)
	if !proto.HasExtension(md.Options, E_ValueRegex) {
		return nil
	}
	ex, err := proto.GetExtension(md.Options, E_ValueRegex)
	if err != nil {
		return fmt.Errorf("proto.GetExtension(value_regex) of %T: %v", msg, err)
	}

	switch s := ex.(type) {
	case *string:
		if s == nil {
			return nil
		}
		// The FHIR definition states that "regexes should be qualified with start
		// of string and end of string anchors based on the regex implementation
		// used" - http://hl7.org/fhir/datatypes.html
		reStr := fmt.Sprintf("^%s$", *s)

		re, err := regexp.Compile(reStr)
		if err != nil {
			// This would indicate a bug in the proto conversion process, or
			// that the specification has a bad regex, rather than poorly formed
			// JSON input.
			return fmt.Errorf("compiling regex %s for %T: %v", reStr, msg, err)
		}
		if !re.Match(json) {
			return fmt.Errorf("does not match regex %s for %T", reStr, msg)
		}
		return nil
	default:
		return fmt.Errorf("value_regex for %T option of type %T", msg, s)
	}
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *Base64Binary) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	return jsonString(base64.StdEncoding.EncodeToString(p.Value)), nil
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *Base64Binary) UnmarshalJSONPB(_ *jsonpb.Unmarshaler, buf []byte) error {
	if err := reMatch(p, buf); err != nil {
		return err
	}
	s, err := parseJSONString(buf)
	if err != nil {
		return err
	}
	buf, err = base64.StdEncoding.DecodeString(s)
	if err != nil {
		return fmt.Errorf("decoding base64: %v", err)
	}
	p.Value = buf
	return nil
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *Boolean) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	if p.Value {
		return []byte(`true`), nil
	}
	return []byte(`false`), nil
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *Boolean) UnmarshalJSONPB(_ *jsonpb.Unmarshaler, buf []byte) error {
	if err := reMatch(p, buf); err != nil {
		return err
	}
	switch string(buf) {
	case `true`:
		p.Value = true
		return nil
	case `false`:
		p.Value = false
		return nil
	}
	return fmt.Errorf(`invalid JSON boolean %s`, buf)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *Code) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *Code) UnmarshalJSONPB(_ *jsonpb.Unmarshaler, buf []byte) error {
	if err := reMatch(p, buf); err != nil {
		return err
	}
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *Date) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *Date) UnmarshalJSONPB(_ *jsonpb.Unmarshaler, buf []byte) error {
	if err := reMatch(p, buf); err != nil {
		return err
	}
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *DateTime) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *DateTime) UnmarshalJSONPB(_ *jsonpb.Unmarshaler, buf []byte) error {
	if err := reMatch(p, buf); err != nil {
		return err
	}
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *Decimal) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *Decimal) UnmarshalJSONPB(_ *jsonpb.Unmarshaler, buf []byte) error {
	if err := reMatch(p, buf); err != nil {
		return err
	}
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *Id) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *Id) UnmarshalJSONPB(_ *jsonpb.Unmarshaler, buf []byte) error {
	if err := reMatch(p, buf); err != nil {
		return err
	}
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *Instant) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *Instant) UnmarshalJSONPB(_ *jsonpb.Unmarshaler, buf []byte) error {
	if err := reMatch(p, buf); err != nil {
		return err
	}
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *Integer) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	return []byte(fmt.Sprintf("%d", p.Value)), nil
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *Integer) UnmarshalJSONPB(_ *jsonpb.Unmarshaler, buf []byte) error {
	if err := reMatch(p, buf); err != nil {
		return err
	}
	i, err := strconv.ParseInt(string(buf), 10, 32)
	if err != nil {
		return fmt.Errorf("parse %s as 32-bit integer: %v", buf, err)
	}
	p.Value = int32(i)
	return nil
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *Markdown) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *Markdown) UnmarshalJSONPB(_ *jsonpb.Unmarshaler, buf []byte) error {
	if err := reMatch(p, buf); err != nil {
		return err
	}
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *Oid) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *Oid) UnmarshalJSONPB(_ *jsonpb.Unmarshaler, buf []byte) error {
	if err := reMatch(p, buf); err != nil {
		return err
	}
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *PositiveInt) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *PositiveInt) UnmarshalJSONPB(_ *jsonpb.Unmarshaler, buf []byte) error {
	if err := reMatch(p, buf); err != nil {
		return err
	}
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *String) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	return jsonString(p.Value), nil
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *String) UnmarshalJSONPB(_ *jsonpb.Unmarshaler, buf []byte) error {
	if err := reMatch(p, buf); err != nil {
		return err
	}
	s, err := parseJSONString(buf)
	if err != nil {
		return err
	}
	p.Value = s
	return nil
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *Time) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *Time) UnmarshalJSONPB(_ *jsonpb.Unmarshaler, buf []byte) error {
	if err := reMatch(p, buf); err != nil {
		return err
	}
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *UnsignedInt) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *UnsignedInt) UnmarshalJSONPB(_ *jsonpb.Unmarshaler, buf []byte) error {
	if err := reMatch(p, buf); err != nil {
		return err
	}
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *Uri) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *Uri) UnmarshalJSONPB(_ *jsonpb.Unmarshaler, buf []byte) error {
	if err := reMatch(p, buf); err != nil {
		return err
	}
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *Uuid) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *Uuid) UnmarshalJSONPB(_ *jsonpb.Unmarshaler, buf []byte) error {
	if err := reMatch(p, buf); err != nil {
		return err
	}
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *Xhtml) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *Xhtml) UnmarshalJSONPB(_ *jsonpb.Unmarshaler, buf []byte) error {
	if err := reMatch(p, buf); err != nil {
		return err
	}
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}
