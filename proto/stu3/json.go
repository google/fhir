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
	"bytes"
	"encoding/base64"
	"errors"
	"fmt"
	"regexp"
	"strconv"
	"strings"
	"time"

	"github.com/golang/protobuf/descriptor"
	"github.com/golang/protobuf/jsonpb"
	"github.com/golang/protobuf/proto"
)

const (
	doubleQuote        = `"`
	escapedDoubleQuote = `\"`
)

// jsonString returns s as a JSON string in double quotes, escaping any existing
// double quotes.
func jsonString(s string) []byte {
	return []byte(fmt.Sprintf(
		`"%s"`,
		strings.Replace(s, doubleQuote, escapedDoubleQuote, -1),
	))
}

// reMatchString strips surrounding double quotes and unescapes remaining double
// quotes, i.e. undoes jsonString(), before checking the resulting string with
// reMatch() and returning it.
func reMatchString(msg descriptor.Message, json []byte) (string, error) {
	n := len(json)
	if n < 2 || json[0] != '"' || json[n-1] != '"' {
		return "", fmt.Errorf("invalid JSON string %s", json)
	}
	json = bytes.Replace(
		json[1:n-1],
		[]byte(escapedDoubleQuote), []byte(doubleQuote), -1,
	)
	if err := reMatch(msg, json); err != nil {
		return "", err
	}
	return string(json), nil
}

// reMatch checks if the byte slices matches the regular expression in the proto
// option "value_regex". If json contains a JSON string, it must already parsed;
// see reMatchString().
func reMatch(msg descriptor.Message, json []byte) error {
	// TODO(arrans) memoise and benchmark extraction and compilation of regexes.
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
		// The FHIR definition states that "regexes should be qualified with
		// start of string and end of string anchors based on the regex
		// implementation used" - http://hl7.org/fhir/datatypes.html
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
		return fmt.Errorf("value_regex for %T option of type %T; expecting string", msg, s)
	}
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface, to properly handle this FHIR primitive type.
func (p *Base64Binary) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	return jsonString(base64.StdEncoding.EncodeToString(p.Value)), nil
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface, to properly handle this FHIR primitive type.
func (p *Base64Binary) UnmarshalJSONPB(_ *jsonpb.Unmarshaler, buf []byte) error {
	s, err := reMatchString(p, buf)
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
// interface, to properly handle this FHIR primitive type.
func (p *Boolean) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	if p.Value {
		return []byte(`true`), nil
	}
	return []byte(`false`), nil
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface, to properly handle this FHIR primitive type.
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
// interface, to properly handle this FHIR primitive type.
func (p *Code) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface, to properly handle this FHIR primitive type.
func (p *Code) UnmarshalJSONPB(_ *jsonpb.Unmarshaler, buf []byte) error {
	if err := reMatch(p, buf); err != nil {
		return err
	}
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

const (
	dateLayoutYear  = `2006`
	dateLayoutMonth = `2006-01`
	dateLayoutDay   = `2006-01-02`

	// Go layout format for convenience rather than checking the docs:
	// Mon Jan 2 15:04:05 -0700 MST 2006
)

const (
	unitToMicro = int64(time.Second / time.Microsecond)
	microToNano = int64(time.Microsecond / time.Nanosecond)
)

// utcFromMicroSec returns a time in UTC from unix microseconds.
func utcFromMicroSec(usec int64) time.Time {
	return time.Unix(usec/unitToMicro, (usec%microToNano)*microToNano).UTC()
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface, to properly handle this FHIR primitive type.
func (p *Date) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	if p.Timezone != "" {
		return nil, fmt.Errorf("FHIR date primitive SHALL not have timezone; has %q", p.Timezone)
	}
	var layout string
	switch p.Precision {
	case Date_YEAR:
		layout = dateLayoutYear
	case Date_MONTH:
		layout = dateLayoutMonth
	case Date_DAY:
		layout = dateLayoutDay
	default:
		return nil, fmt.Errorf("unknown precision[%d]", p.Precision)
	}
	return jsonString(p.Time().Format(layout)), nil
}

var datePrecisionLayout = map[Date_Precision]string{
	Date_YEAR:  dateLayoutYear,
	Date_MONTH: dateLayoutMonth,
	Date_DAY:   dateLayoutDay,
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface, to properly handle this FHIR primitive type.
func (p *Date) UnmarshalJSONPB(_ *jsonpb.Unmarshaler, buf []byte) error {
	s, err := reMatchString(p, buf)
	if err != nil {
		return err
	}
	for prec, layout := range datePrecisionLayout {
		t, err := time.ParseInLocation(layout, s, time.UTC)
		if err != nil {
			continue
		}
		p.ValueUs = t.UnixNano() / microToNano
		p.Precision = prec
		return nil
	}
	return fmt.Errorf("invalid date %s", buf)
}

// Time returns the Date object as a UTC Time, rounded based on Precision.
// Rounding is not performed with time.Time.Round() but rather by setting all
// finer-grain time scales to "zero";" i.e. YEAR-precision will always be
// January 1st at 00:00, MONTH-precision will always be 1st of the month at
// 00:00, and DAY will always be at 00:00 time.
func (p *Date) Time() time.Time {
	t := utcFromMicroSec(p.ValueUs)

	// 00:00 time
	sub := time.Duration(t.Hour())*time.Hour +
		time.Duration(t.Minute())*time.Minute +
		time.Duration(t.Second())*time.Second
	t = t.Add(-sub)

	var subMonths, subDays int
	switch p.Precision {
	case Date_YEAR:
		subMonths = int(t.Month() - time.January)
		fallthrough
	case Date_MONTH:
		subDays = t.Day() - 1
	}

	return t.AddDate(0, -subMonths, -subDays)
}

// TODO(arrans): implement AsDate()
// AsDate returns a Date at the specified time, after rounding according to the
// specified precision.
//func AsDate(t time.Time, p Date_Precision) *Date {
//}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface, to properly handle this FHIR primitive type.
func (p *DateTime) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface, to properly handle this FHIR primitive type.
func (p *DateTime) UnmarshalJSONPB(_ *jsonpb.Unmarshaler, buf []byte) error {
	if err := reMatch(p, buf); err != nil {
		return err
	}
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface, to properly handle this FHIR primitive type.
func (p *Decimal) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface, to properly handle this FHIR primitive type.
func (p *Decimal) UnmarshalJSONPB(_ *jsonpb.Unmarshaler, buf []byte) error {
	if err := reMatch(p, buf); err != nil {
		return err
	}
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface, to properly handle this FHIR primitive type.
func (p *Id) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface, to properly handle this FHIR primitive type.
func (p *Id) UnmarshalJSONPB(_ *jsonpb.Unmarshaler, buf []byte) error {
	if err := reMatch(p, buf); err != nil {
		return err
	}
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface, to properly handle this FHIR primitive type.
func (p *Instant) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface, to properly handle this FHIR primitive type.
func (p *Instant) UnmarshalJSONPB(_ *jsonpb.Unmarshaler, buf []byte) error {
	if err := reMatch(p, buf); err != nil {
		return err
	}
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface, to properly handle this FHIR primitive type.
func (p *Integer) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	return []byte(fmt.Sprintf("%d", p.Value)), nil
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface, to properly handle this FHIR primitive type.
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
// interface, to properly handle this FHIR primitive type.
func (p *Markdown) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface, to properly handle this FHIR primitive type.
func (p *Markdown) UnmarshalJSONPB(_ *jsonpb.Unmarshaler, buf []byte) error {
	if err := reMatch(p, buf); err != nil {
		return err
	}
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface, to properly handle this FHIR primitive type.
func (p *Oid) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface, to properly handle this FHIR primitive type.
func (p *Oid) UnmarshalJSONPB(_ *jsonpb.Unmarshaler, buf []byte) error {
	if err := reMatch(p, buf); err != nil {
		return err
	}
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface, to properly handle this FHIR primitive type.
func (p *PositiveInt) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	return []byte(fmt.Sprintf("%d", p.Value)), nil
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface, to properly handle this FHIR primitive type.
func (p *PositiveInt) UnmarshalJSONPB(_ *jsonpb.Unmarshaler, buf []byte) error {
	if err := reMatch(p, buf); err != nil {
		return err
	}
	i, err := strconv.ParseUint(string(buf), 10, 32)
	if err != nil {
		return fmt.Errorf("parse %s as 32-bit unsigned integer: %v", buf, err)
	}
	if i == 0 {
		return errors.New("FHIR positiveInt primitive must be >0")
	}
	p.Value = uint32(i)
	return nil
}

const noEmptyString = "FHIR string primitive can never be empty"

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface, to properly handle this FHIR primitive type.
func (p *String) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	if p.Value == "" {
		return nil, errors.New(noEmptyString)
	}
	return jsonString(p.Value), nil
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface, to properly handle this FHIR primitive type.
func (p *String) UnmarshalJSONPB(_ *jsonpb.Unmarshaler, buf []byte) error {
	s, err := reMatchString(p, buf)
	if err != nil {
		return err
	}
	if s == "" {
		return errors.New(noEmptyString)
	}
	p.Value = s
	return nil
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface, to properly handle this FHIR primitive type.
func (p *Time) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface, to properly handle this FHIR primitive type.
func (p *Time) UnmarshalJSONPB(_ *jsonpb.Unmarshaler, buf []byte) error {
	if err := reMatch(p, buf); err != nil {
		return err
	}
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface, to properly handle this FHIR primitive type.
func (p *UnsignedInt) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	return []byte(fmt.Sprintf("%d", p.Value)), nil
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface, to properly handle this FHIR primitive type.
func (p *UnsignedInt) UnmarshalJSONPB(_ *jsonpb.Unmarshaler, buf []byte) error {
	if err := reMatch(p, buf); err != nil {
		return err
	}
	i, err := strconv.ParseUint(string(buf), 10, 32)
	if err != nil {
		return fmt.Errorf("parse %s as 32-bit unsigned integer: %v", buf, err)
	}
	p.Value = uint32(i)
	return nil
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface, to properly handle this FHIR primitive type.
func (p *Uri) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface, to properly handle this FHIR primitive type.
func (p *Uri) UnmarshalJSONPB(_ *jsonpb.Unmarshaler, buf []byte) error {
	if err := reMatch(p, buf); err != nil {
		return err
	}
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface, to properly handle this FHIR primitive type.
func (p *Uuid) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface, to properly handle this FHIR primitive type.
func (p *Uuid) UnmarshalJSONPB(_ *jsonpb.Unmarshaler, buf []byte) error {
	if err := reMatch(p, buf); err != nil {
		return err
	}
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}

// MarshalJSONPB implements the single method of the jsonpb.JSONPBMarshaler
// interface, to properly handle this FHIR primitive type.
func (p *Xhtml) MarshalJSONPB(_ *jsonpb.Marshaler) ([]byte, error) {
	return nil, fmt.Errorf("MarshalJSONPB unimplemented for %T", p)
}

// UnmarshalJSONPB implements the single method of the jsonpb.JSONPBUnmarshaler
// interface, to properly handle this FHIR primitive type.
func (p *Xhtml) UnmarshalJSONPB(_ *jsonpb.Unmarshaler, buf []byte) error {
	if err := reMatch(p, buf); err != nil {
		return err
	}
	return fmt.Errorf("UnmarshalJSONPB unimplemented for %T", p)
}
