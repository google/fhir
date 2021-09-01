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

// Package jsonformat provides utility functions for FHIR proto conversion.
package jsonformat

import (
	"encoding/json"
	"fmt"
	"time"

	"github.com/google/fhir/go/jsonformat/internal/accessor"
	"github.com/google/fhir/go/jsonformat/internal/jsonpbhelper"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/reflect/protoreflect"
)

func checkEnumValueNames(enum protoreflect.EnumDescriptor, valueNames ...string) error {
	ev := enum.Values()
	for _, vn := range valueNames {
		if ev.ByName(protoreflect.Name(vn)) == nil {
			return fmt.Errorf("enum: %v does not have a value with name: %v", enum.FullName(), vn)
		}
	}
	return nil
}

// parseDateFromStr parses a FHIR date string into a Date proto message, m.
func parseDateFromStr(date string, l *time.Location, m proto.Message) error {
	mr := m.ProtoReflect()
	// Date regular expression definition from https://www.hl7.org/fhir/datatypes.html
	matched := jsonpbhelper.DateCompiledRegex.MatchString(date)
	timeZone := l.String()
	precEnum, err := accessor.GetEnumDescriptor(mr.Descriptor(), "precision")
	if err != nil {
		return err
	}
	if err := checkEnumValueNames(precEnum, "DAY", "MONTH", "YEAR"); err != nil {
		return err
	}
	day := precEnum.Values().ByName("DAY").Number()
	month := precEnum.Values().ByName("MONTH").Number()
	year := precEnum.Values().ByName("YEAR").Number()
	createDate := func(prec protoreflect.EnumNumber, valueUs int64) error {
		if err := accessor.SetValue(mr, prec, "precision"); err != nil {
			return err
		}
		if err := accessor.SetValue(mr, valueUs, "value_us"); err != nil {
			return err
		}
		if err := accessor.SetValue(mr, timeZone, "timezone"); err != nil {
			return err
		}
		return nil
	}
	if !matched {
		return fmt.Errorf("invalid date: %v", date)
	}
	if t, err := time.ParseInLocation(jsonpbhelper.LayoutDay, date, l); err == nil {
		return createDate(day, jsonpbhelper.GetTimestampUsec(t))
	}
	if t, err := time.ParseInLocation(jsonpbhelper.LayoutMonth, date, l); err == nil {
		return createDate(month, jsonpbhelper.GetTimestampUsec(t))
	}
	if t, err := time.ParseInLocation(jsonpbhelper.LayoutYear, date, l); err == nil {
		return createDate(year, jsonpbhelper.GetTimestampUsec(t))
	}
	return fmt.Errorf("invalid date layout (expecting YYYY or YYYY-MM or YYYY-MM-DD): %v", date)
}

// parseDateFromJSON parses a FHIR date string into a Date proto message, m.
func parseDateFromJSON(rm json.RawMessage, l *time.Location, m proto.Message) error {
	var date string
	if err := jsonpbhelper.JSP.Unmarshal(rm, &date); err != nil {
		return err
	}
	return parseDateFromStr(date, l, m)
}

// serializeDate serializes a FHIR Date proto message to a JSON string.
func serializeDate(d proto.Message) (string, error) {
	rd := d.ProtoReflect()
	valueUs, err := accessor.GetInt64(rd, "value_us")
	if err != nil {
		return "", err
	}
	tz, err := accessor.GetString(rd, "timezone")
	if err != nil {
		return "", err
	}
	ts, err := jsonpbhelper.GetTimeFromUsec(valueUs, tz)
	if err != nil {
		return "", err
	}
	dt := rd.Descriptor()
	precEnum, err := accessor.GetEnumDescriptor(dt, "precision")
	if err != nil {
		return "", err
	}
	if err := checkEnumValueNames(precEnum, "DAY", "MONTH", "YEAR"); err != nil {
		return "", err
	}
	day := precEnum.Values().ByName("DAY").Number()
	month := precEnum.Values().ByName("MONTH").Number()
	year := precEnum.Values().ByName("YEAR").Number()

	prec, err := accessor.GetEnumNumber(rd, "precision")
	if err != nil {
		return "", err
	}
	var dstr string
	switch prec {
	case day:
		dstr = ts.Format(jsonpbhelper.LayoutDay)
	case month:
		dstr = ts.Format(jsonpbhelper.LayoutMonth)
	case year:
		dstr = ts.Format(jsonpbhelper.LayoutYear)
	default:
		return "", fmt.Errorf("invalid Date precision in %v", d)
	}
	return dstr, nil
}

// parseDateTimeFromStr parses a FHIR dateTime string, and returns, if successful, a DateTime proto
// message with the parsed date/time information.
func parseDateTimeFromStr(date string, l *time.Location, m proto.Message) error {
	mr := m.ProtoReflect()
	// DateTime regular expression definition from https://www.hl7.org/fhir/datatypes.html
	matched := jsonpbhelper.DateTimeCompiledRegex.MatchString(date)
	if !matched {
		return fmt.Errorf("invalid dateTime: %v", date)
	}
	precEnum, err := accessor.GetEnumDescriptor(mr.Descriptor(), "precision")
	if err != nil {
		return err
	}
	if err := checkEnumValueNames(precEnum, "DAY", "MONTH", "YEAR", "SECOND", "MILLISECOND", "MICROSECOND"); err != nil {
		return err
	}
	day := precEnum.Values().ByName("DAY").Number()
	month := precEnum.Values().ByName("MONTH").Number()
	year := precEnum.Values().ByName("YEAR").Number()
	second := precEnum.Values().ByName("SECOND").Number()
	millisecond := precEnum.Values().ByName("MILLISECOND").Number()
	microsecond := precEnum.Values().ByName("MICROSECOND").Number()

	createDateTime := func(prec protoreflect.EnumNumber, valueUs int64, tz string) error {
		if err := accessor.SetValue(mr, prec, "precision"); err != nil {
			return err
		}
		if err := accessor.SetValue(mr, valueUs, "value_us"); err != nil {
			return err
		}
		if err := accessor.SetValue(mr, tz, "timezone"); err != nil {
			return err
		}
		return nil
	}

	precision := second
	if jsonpbhelper.IsSubmilli(date) {
		precision = microsecond
	} else if jsonpbhelper.IsSubsecond(date) {
		precision = millisecond
	}
	if t, err := time.Parse(jsonpbhelper.LayoutSeconds, date); err == nil {
		return createDateTime(precision, jsonpbhelper.GetTimestampUsec(t), jsonpbhelper.ExtractTimezone(t))
	} else if t, err := time.Parse(jsonpbhelper.LayoutSecondsUTC, date); err == nil {
		return createDateTime(precision, jsonpbhelper.GetTimestampUsec(t), jsonpbhelper.UTC)
	} else if t, err := time.ParseInLocation(jsonpbhelper.LayoutDay, date, l); err == nil {
		return createDateTime(day, jsonpbhelper.GetTimestampUsec(t), l.String())
	} else if t, err := time.ParseInLocation(jsonpbhelper.LayoutMonth, date, l); err == nil {
		return createDateTime(month, jsonpbhelper.GetTimestampUsec(t), l.String())
	} else if t, err := time.ParseInLocation(jsonpbhelper.LayoutYear, date, l); err == nil {
		return createDateTime(year, jsonpbhelper.GetTimestampUsec(t), l.String())
	}
	return fmt.Errorf("invalid dateTime layout: %v", date)
}

// parseDateTimeFromJSON parses a FHIR date string into a DateTime proto, m.
func parseDateTimeFromJSON(rm json.RawMessage, l *time.Location, m proto.Message) error {
	var date string
	if err := jsonpbhelper.JSP.Unmarshal(rm, &date); err != nil {
		return err
	}
	return parseDateTimeFromStr(date, l, m)
}

// serializeDateTime serializes a FHIR DateTime proto message to a JSON string.
func serializeDateTime(dt proto.Message) (string, error) {
	rdt := dt.ProtoReflect()
	valueUs, err := accessor.GetInt64(rdt, "value_us")
	if err != nil {
		return "", err
	}
	tz, err := accessor.GetString(rdt, "timezone")
	if err != nil {
		return "", err
	}
	ts, err := jsonpbhelper.GetTimeFromUsec(valueUs, tz)
	if err != nil {
		return "", err
	}
	ty := rdt.Descriptor()
	precEnum, err := accessor.GetEnumDescriptor(ty, "precision")
	if err != nil {
		return "", err
	}
	if err := checkEnumValueNames(precEnum, "DAY", "MONTH", "YEAR", "SECOND", "MILLISECOND", "MICROSECOND"); err != nil {
		return "", err
	}
	day := precEnum.Values().ByName("DAY").Number()
	month := precEnum.Values().ByName("MONTH").Number()
	year := precEnum.Values().ByName("YEAR").Number()
	second := precEnum.Values().ByName("SECOND").Number()
	millisecond := precEnum.Values().ByName("MILLISECOND").Number()
	microsecond := precEnum.Values().ByName("MICROSECOND").Number()

	prec, err := accessor.GetEnumNumber(rdt, "precision")
	if err != nil {
		return "", err
	}

	var dtstr string
	switch prec {
	case second:
		if tz == jsonpbhelper.UTC {
			dtstr = ts.Format(jsonpbhelper.LayoutSecondsUTC)
		} else {
			dtstr = ts.Format(jsonpbhelper.LayoutSeconds)
		}
	case millisecond:
		if tz == jsonpbhelper.UTC {
			dtstr = ts.Format(jsonpbhelper.LayoutMillisUTC)
		} else {
			dtstr = ts.Format(jsonpbhelper.LayoutMillis)
		}
	case microsecond:
		if tz == jsonpbhelper.UTC {
			dtstr = ts.Format(jsonpbhelper.LayoutMicrosUTC)
		} else {
			dtstr = ts.Format(jsonpbhelper.LayoutMicros)
		}
	case day:
		dtstr = ts.Format(jsonpbhelper.LayoutDay)
	case month:
		dtstr = ts.Format(jsonpbhelper.LayoutMonth)
	case year:
		dtstr = ts.Format(jsonpbhelper.LayoutYear)
	default:
		return "", fmt.Errorf("invalid DateTime precision in %v", dt)
	}

	return dtstr, nil
}

// parseTime parses a FHIR time string into a Time proto message.
func parseTime(rm json.RawMessage, m proto.Message) error {
	mr := m.ProtoReflect()
	t, err := jsonpbhelper.ParseTime(rm)
	if err != nil {
		return err
	}
	precEnum, err := accessor.GetEnumDescriptor(mr.Descriptor(), "precision")
	if err != nil {
		return err
	}
	if err := checkEnumValueNames(precEnum, "SECOND", "MILLISECOND", "MICROSECOND"); err != nil {
		return err
	}
	second := precEnum.Values().ByName("SECOND").Number()
	millisecond := precEnum.Values().ByName("MILLISECOND").Number()
	microsecond := precEnum.Values().ByName("MICROSECOND").Number()

	var p protoreflect.EnumNumber
	switch t.Precision {
	case jsonpbhelper.PrecisionSecond:
		p = second
	case jsonpbhelper.PrecisionMillisecond:
		p = millisecond
	case jsonpbhelper.PrecisionMicrosecond:
		p = microsecond
	default:
		return fmt.Errorf("invalid time precision: %v", t.Precision)
	}
	if err := accessor.SetValue(mr, p, "precision"); err != nil {
		return err
	}
	if err := accessor.SetValue(mr, t.ValueUs, "value_us"); err != nil {
		return err
	}
	return nil
}

// serializeTime serializes a FHIR Time proto message to a JSON string.
func serializeTime(t proto.Message) (string, error) {
	rt := t.ProtoReflect()
	precEnum, err := accessor.GetEnumDescriptor(rt.Descriptor(), "precision")
	if err != nil {
		return "", err
	}
	if err := checkEnumValueNames(precEnum, "SECOND", "MILLISECOND", "MICROSECOND"); err != nil {
		return "", err
	}
	second := precEnum.Values().ByName("SECOND").Number()
	millisecond := precEnum.Values().ByName("MILLISECOND").Number()
	microsecond := precEnum.Values().ByName("MICROSECOND").Number()
	prec, err := accessor.GetEnumNumber(rt, "precision")
	if err != nil {
		return "", err
	}
	var p jsonpbhelper.Precision
	switch prec {
	case second:
		p = jsonpbhelper.PrecisionSecond
	case millisecond:
		p = jsonpbhelper.PrecisionMillisecond
	case microsecond:
		p = jsonpbhelper.PrecisionMicrosecond
	default:
		return "", fmt.Errorf("invalid time precision in %v", t)
	}
	valueUs, err := accessor.GetInt64(rt, "value_us")
	if err != nil {
		return "", err
	}
	return jsonpbhelper.SerializeTime(valueUs, p)
}

// parseInstant parses a FHIR instant string into an Instant proto message, m.
func parseInstant(rm json.RawMessage, m proto.Message) error {
	mr := m.ProtoReflect()
	var instant string
	if err := jsonpbhelper.JSP.Unmarshal(rm, &instant); err != nil {
		return err
	}
	// Instant is dateTime to the precision of at least SECOND and always includes a timezone,
	// as specified in https://www.hl7.org/fhir/datatypes.html
	matched := jsonpbhelper.InstantCompiledRegex.MatchString(instant)
	if !matched {
		return fmt.Errorf("invalid instant")
	}
	precEnum, err := accessor.GetEnumDescriptor(mr.Descriptor(), "precision")
	if err != nil {
		return err
	}
	if err := checkEnumValueNames(precEnum, "SECOND", "MILLISECOND", "MICROSECOND"); err != nil {
		return err
	}
	second := precEnum.Values().ByName("SECOND").Number()
	millisecond := precEnum.Values().ByName("MILLISECOND").Number()
	microsecond := precEnum.Values().ByName("MICROSECOND").Number()

	createInstant := func(prec protoreflect.EnumNumber, valueUs int64, tz string) error {
		if err := accessor.SetValue(mr, prec, "precision"); err != nil {
			return err
		}
		if err := accessor.SetValue(mr, valueUs, "value_us"); err != nil {
			return err
		}
		if err := accessor.SetValue(mr, tz, "timezone"); err != nil {
			return err
		}
		return nil
	}

	precision := second
	if jsonpbhelper.IsSubmilli(instant) {
		precision = microsecond
	} else if jsonpbhelper.IsSubsecond(instant) {
		precision = millisecond
	}
	if t, err := time.Parse(jsonpbhelper.LayoutSecondsUTC, instant); err == nil {
		return createInstant(precision, jsonpbhelper.GetTimestampUsec(t), jsonpbhelper.UTC)
	} else if t, err := time.Parse(jsonpbhelper.LayoutSeconds, instant); err == nil {
		return createInstant(precision, jsonpbhelper.GetTimestampUsec(t), jsonpbhelper.ExtractTimezone(t))
	}
	return fmt.Errorf("invalid instant layout: %v", instant)
}

// SerializeInstant takes an Instant proto message and serializes it to a datetime string.
func SerializeInstant(instant proto.Message) (string, error) {
	rinstant := instant.ProtoReflect()
	precEnum, err := accessor.GetEnumDescriptor(rinstant.Descriptor(), "precision")
	if err != nil {
		return "", err
	}
	if err := checkEnumValueNames(precEnum, "SECOND", "MILLISECOND", "MICROSECOND"); err != nil {
		return "", err
	}
	second := precEnum.Values().ByName("SECOND").Number()
	millisecond := precEnum.Values().ByName("MILLISECOND").Number()
	microsecond := precEnum.Values().ByName("MICROSECOND").Number()

	usec, err := accessor.GetInt64(rinstant, "value_us")
	if err != nil {
		return "", err
	}
	precision, err := accessor.GetEnumNumber(rinstant, "precision")
	if err != nil {
		return "", err
	}
	tz, err := accessor.GetString(rinstant, "timezone")
	if err != nil {
		return "", err
	}
	ts, err := jsonpbhelper.GetTimeFromUsec(usec, tz)
	if err != nil {
		return "", fmt.Errorf("in GetTimeFromUsec(): %w", err)
	}
	switch precision {
	case second:
		if tz == jsonpbhelper.UTC {
			return ts.Format(jsonpbhelper.LayoutSecondsUTC), nil
		}
		return ts.Format(jsonpbhelper.LayoutSeconds), nil
	case millisecond:
		if tz == jsonpbhelper.UTC {
			return ts.Format(jsonpbhelper.LayoutMillisUTC), nil
		}
		return ts.Format(jsonpbhelper.LayoutMillis), nil
	case microsecond:
		if tz == jsonpbhelper.UTC {
			return ts.Format(jsonpbhelper.LayoutMicrosUTC), nil
		}
		return ts.Format(jsonpbhelper.LayoutMicros), nil
	default:
		return "", fmt.Errorf("invalid instant precision: %v", precision)
	}
}
