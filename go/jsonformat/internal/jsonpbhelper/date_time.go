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
	"encoding/json"
	"fmt"
	"time"

	"github.com/google/fhir/go/jsonformat/internal/accessor"
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

// ParseDateFromString parses a FHIR date string into a Date proto message, m.
func ParseDateFromString(date string, l *time.Location, m proto.Message) error {
	mr := m.ProtoReflect()
	// Date regular expression definition from https://www.hl7.org/fhir/datatypes.html
	matched := DateCompiledRegex.MatchString(date)
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
	if t, err := time.ParseInLocation(LayoutDay, date, l); err == nil {
		return createDate(day, GetTimestampUsec(t))
	}
	if t, err := time.ParseInLocation(LayoutMonth, date, l); err == nil {
		return createDate(month, GetTimestampUsec(t))
	}
	if t, err := time.ParseInLocation(LayoutYear, date, l); err == nil {
		return createDate(year, GetTimestampUsec(t))
	}
	return fmt.Errorf("invalid date layout (expecting YYYY or YYYY-MM or YYYY-MM-DD): %v", date)
}

// SerializeDate serializes a FHIR Date proto message to a JSON string.
func SerializeDate(d proto.Message) (string, error) {
	rd := d.ProtoReflect()
	valueUs, err := accessor.GetInt64(rd, "value_us")
	if err != nil {
		return "", err
	}
	tz, err := accessor.GetString(rd, "timezone")
	if err != nil {
		return "", err
	}
	ts, err := GetTimeFromUsec(valueUs, tz)
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
		dstr = ts.Format(LayoutDay)
	case month:
		dstr = ts.Format(LayoutMonth)
	case year:
		dstr = ts.Format(LayoutYear)
	default:
		return "", fmt.Errorf("invalid Date precision in %v", d)
	}
	return dstr, nil
}

// ParseDateTimeFromString parses a FHIR dateTime string, and returns, if successful, a DateTime proto
// message with the parsed date/time information.
func ParseDateTimeFromString(date string, l *time.Location, m proto.Message) error {
	mr := m.ProtoReflect()
	// DateTime regular expression definition from https://www.hl7.org/fhir/datatypes.html
	matched := DateTimeCompiledRegex.MatchString(date)
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
	if IsSubmilli(date) {
		precision = microsecond
	} else if IsSubsecond(date) {
		precision = millisecond
	}
	if t, err := time.Parse(LayoutSeconds, date); err == nil {
		return createDateTime(precision, GetTimestampUsec(t), ExtractTimezone(t))
	} else if t, err := time.Parse(LayoutSecondsUTC, date); err == nil {
		return createDateTime(precision, GetTimestampUsec(t), UTC)
	} else if t, err := time.ParseInLocation(LayoutDay, date, l); err == nil {
		return createDateTime(day, GetTimestampUsec(t), l.String())
	} else if t, err := time.ParseInLocation(LayoutMonth, date, l); err == nil {
		return createDateTime(month, GetTimestampUsec(t), l.String())
	} else if t, err := time.ParseInLocation(LayoutYear, date, l); err == nil {
		return createDateTime(year, GetTimestampUsec(t), l.String())
	}
	return fmt.Errorf("invalid dateTime layout: %v", date)
}

// SerializeDateTime serializes a FHIR DateTime proto message to a JSON string.
func SerializeDateTime(dt proto.Message) (string, error) {
	rdt := dt.ProtoReflect()
	valueUs, err := accessor.GetInt64(rdt, "value_us")
	if err != nil {
		return "", err
	}
	tz, err := accessor.GetString(rdt, "timezone")
	if err != nil {
		return "", err
	}
	ts, err := GetTimeFromUsec(valueUs, tz)
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
		if tz == UTC {
			dtstr = ts.Format(LayoutSecondsUTC)
		} else {
			dtstr = ts.Format(LayoutSeconds)
		}
	case millisecond:
		if tz == UTC {
			dtstr = ts.Format(LayoutMillisUTC)
		} else {
			dtstr = ts.Format(LayoutMillis)
		}
	case microsecond:
		if tz == UTC {
			dtstr = ts.Format(LayoutMicrosUTC)
		} else {
			dtstr = ts.Format(LayoutMicros)
		}
	case day:
		dtstr = ts.Format(LayoutDay)
	case month:
		dtstr = ts.Format(LayoutMonth)
	case year:
		dtstr = ts.Format(LayoutYear)
	default:
		return "", fmt.Errorf("invalid DateTime precision in %v", dt)
	}

	return dtstr, nil
}

// ParseTime parses a FHIR time string into a Time proto message.
func ParseTime(rm json.RawMessage, m proto.Message) error {
	mr := m.ProtoReflect()
	t, err := parseTimeToGoTime(rm)
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
	case PrecisionSecond:
		p = second
	case PrecisionMillisecond:
		p = millisecond
	case PrecisionMicrosecond:
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

// SerializeTime serializes a FHIR Time proto message to a JSON string.
func SerializeTime(t proto.Message) (string, error) {
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
	var p Precision
	switch prec {
	case second:
		p = PrecisionSecond
	case millisecond:
		p = PrecisionMillisecond
	case microsecond:
		p = PrecisionMicrosecond
	default:
		return "", fmt.Errorf("invalid time precision in %v", t)
	}
	valueUs, err := accessor.GetInt64(rt, "value_us")
	if err != nil {
		return "", err
	}
	return serializeTime(valueUs, p)
}

// serializeTime serializes the values from a Time proto message to a JSON string.
func serializeTime(us int64, precision Precision) (string, error) {
	ts, err := GetTimeFromUsec(us, UTC)
	if err != nil {
		return "", fmt.Errorf("in GetTimeFromUsec(): %v", err)
	}
	var tstr string
	switch precision {
	case PrecisionSecond:
		tstr = ts.Format(LayoutTimeSecond)
	case PrecisionMillisecond:
		tstr = ts.Format(LayoutTimeMilliSecond)
	case PrecisionMicrosecond:
		tstr = ts.Format(LayoutTimeMicroSecond)
	default:
		return "", fmt.Errorf("invalid time precision %v", precision)
	}
	return tstr, nil
}

// ParseInstant parses a FHIR instant string into an Instant proto message, m.
func ParseInstant(rm json.RawMessage, m proto.Message) error {
	mr := m.ProtoReflect()
	var instant string
	if err := JSP.Unmarshal(rm, &instant); err != nil {
		return err
	}
	// Instant is dateTime to the precision of at least SECOND and always includes a timezone,
	// as specified in https://www.hl7.org/fhir/datatypes.html
	matched := InstantCompiledRegex.MatchString(instant)
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
	if IsSubmilli(instant) {
		precision = microsecond
	} else if IsSubsecond(instant) {
		precision = millisecond
	}
	if t, err := time.Parse(LayoutSecondsUTC, instant); err == nil {
		return createInstant(precision, GetTimestampUsec(t), UTC)
	} else if t, err := time.Parse(LayoutSeconds, instant); err == nil {
		return createInstant(precision, GetTimestampUsec(t), ExtractTimezone(t))
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
	ts, err := GetTimeFromUsec(usec, tz)
	if err != nil {
		return "", fmt.Errorf("in GetTimeFromUsec(): %w", err)
	}
	switch precision {
	case second:
		if tz == UTC {
			return ts.Format(LayoutSecondsUTC), nil
		}
		return ts.Format(LayoutSeconds), nil
	case millisecond:
		if tz == UTC {
			return ts.Format(LayoutMillisUTC), nil
		}
		return ts.Format(LayoutMillis), nil
	case microsecond:
		if tz == UTC {
			return ts.Format(LayoutMicrosUTC), nil
		}
		return ts.Format(LayoutMicros), nil
	default:
		return "", fmt.Errorf("invalid instant precision: %v", precision)
	}
}
