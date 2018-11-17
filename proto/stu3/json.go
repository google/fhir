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
	"strings"

	"github.com/golang/protobuf/jsonpb"
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

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *Base64Binary) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	return jsonString(base64.StdEncoding.EncodeToString(p.Value)), nil
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *Base64Binary) UnmarshalJSONPB(m *jsonpb.Unmarshaler, buf []byte) error {
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
func (p *Boolean) MarshalJSONPB(m *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *Boolean) UnmarshalJSONPB(m *jsonpb.Unmarshaler, buf []byte) error {
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *Code) MarshalJSONPB(m *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *Code) UnmarshalJSONPB(m *jsonpb.Unmarshaler, buf []byte) error {
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *Date) MarshalJSONPB(m *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *Date) UnmarshalJSONPB(m *jsonpb.Unmarshaler, buf []byte) error {
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *DateTime) MarshalJSONPB(m *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *DateTime) UnmarshalJSONPB(m *jsonpb.Unmarshaler, buf []byte) error {
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *Decimal) MarshalJSONPB(m *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *Decimal) UnmarshalJSONPB(m *jsonpb.Unmarshaler, buf []byte) error {
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *Id) MarshalJSONPB(m *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *Id) UnmarshalJSONPB(m *jsonpb.Unmarshaler, buf []byte) error {
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *Instant) MarshalJSONPB(m *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *Instant) UnmarshalJSONPB(m *jsonpb.Unmarshaler, buf []byte) error {
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *Integer) MarshalJSONPB(m *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *Integer) UnmarshalJSONPB(m *jsonpb.Unmarshaler, buf []byte) error {
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *Markdown) MarshalJSONPB(m *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *Markdown) UnmarshalJSONPB(m *jsonpb.Unmarshaler, buf []byte) error {
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *Oid) MarshalJSONPB(m *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *Oid) UnmarshalJSONPB(m *jsonpb.Unmarshaler, buf []byte) error {
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *PositiveInt) MarshalJSONPB(m *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *PositiveInt) UnmarshalJSONPB(m *jsonpb.Unmarshaler, buf []byte) error {
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *String) MarshalJSONPB(m *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *String) UnmarshalJSONPB(m *jsonpb.Unmarshaler, buf []byte) error {
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *Time) MarshalJSONPB(m *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *Time) UnmarshalJSONPB(m *jsonpb.Unmarshaler, buf []byte) error {
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *UnsignedInt) MarshalJSONPB(m *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *UnsignedInt) UnmarshalJSONPB(m *jsonpb.Unmarshaler, buf []byte) error {
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *Uri) MarshalJSONPB(m *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *Uri) UnmarshalJSONPB(m *jsonpb.Unmarshaler, buf []byte) error {
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *Uuid) MarshalJSONPB(m *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *Uuid) UnmarshalJSONPB(m *jsonpb.Unmarshaler, buf []byte) error {
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface.
func (p *Xhtml) MarshalJSONPB(m *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface.
func (p *Xhtml) UnmarshalJSONPB(m *jsonpb.Unmarshaler, buf []byte) error {
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}
