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
	"strconv"
	"testing"
	"time"

	"github.com/google/fhir/go/jsonformat/internal/accessor"
	"github.com/google/go-cmp/cmp"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/reflect/protoreflect"
	"google.golang.org/protobuf/testing/protocmp"

	d4pb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/datatypes_go_proto"
	d3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/datatypes_go_proto"
)

func newDatesForTest(val int64, precName string, tz string) []proto.Message {
	ret := []proto.Message{
		&d3pb.Date{ValueUs: val, Timezone: tz},
		&d4pb.Date{ValueUs: val, Timezone: tz},
	}
	for _, p := range ret {
		rp := p.ProtoReflect()
		precEnum, _ := accessor.GetEnumDescriptor(rp.Descriptor(), "precision")
		prec := precEnum.Values().ByName(protoreflect.Name(precName)).Number()
		_ = accessor.SetValue(rp, prec, "precision")
	}
	return ret
}

func TestDate(t *testing.T) {
	tests := []struct {
		json    string
		val     int64
		prec    string
		protoTz string
		tz      string
	}{
		{
			json:    "1970-01-01",
			val:     0,
			prec:    "DAY",
			protoTz: "UTC",
			tz:      "UTC",
		},
		{
			json:    "1970-01",
			val:     0,
			prec:    "MONTH",
			protoTz: "UTC",
			tz:      "UTC",
		},
		{
			json:    "1970",
			val:     0,
			prec:    "YEAR",
			protoTz: "UTC",
			tz:      "UTC",
		},
		{
			json:    "2017-12-31",
			val:     1514707200000000,
			prec:    "DAY",
			protoTz: "America/Los_Angeles",
			tz:      "America/Los_Angeles", // GMT-7
		},
		{
			json:    "2017-12",
			val:     1512115200000000,
			prec:    "MONTH",
			protoTz: "America/Los_Angeles",
			tz:      "America/Los_Angeles",
		},
		{
			json:    "2017",
			val:     1483257600000000,
			prec:    "YEAR",
			protoTz: "America/Los_Angeles",
			tz:      "America/Los_Angeles",
		},
		{
			json:    "2018-01-01",
			val:     1514736000000000,
			prec:    "DAY",
			protoTz: "Asia/Shanghai",
			tz:      "Asia/Shanghai", // GMT+8
		},
		{
			json:    "2018-01",
			val:     1514736000000000,
			prec:    "MONTH",
			protoTz: "Asia/Shanghai",
			tz:      "Asia/Shanghai",
		},
		{
			json:    "2018",
			val:     1514736000000000,
			prec:    "YEAR",
			protoTz: "Asia/Shanghai",
			tz:      "Asia/Shanghai",
		},
		{
			json:    "3000-01-01",
			val:     32503651200000000,
			prec:    "DAY",
			protoTz: "Asia/Shanghai",
			tz:      "Asia/Shanghai",
		},
	}
	for _, test := range tests {
		l, _ := time.LoadLocation(test.tz)
		dates := newDatesForTest(test.val, test.prec, test.protoTz)
		for _, d := range dates {
			parsed := d.ProtoReflect().New().Interface().(proto.Message)
			if err := ParseDateFromString(test.json, l, parsed); err != nil {
				t.Fatalf("ParseDateFromJSON(%q, %s, %T): %v", test.json, l, parsed, err)
			}
			if want := d; !cmp.Equal(want, parsed, protocmp.Transform()) {
				t.Errorf("ParseDateFromJSON(%q, %s, %T): got %v, want %v", test.json, l, parsed, parsed, want)
			}

			serialized, err := SerializeDate(d)
			if err != nil {
				t.Fatalf("SerializeDate(%q): %v", d, err)
			}
			if want := test.json; want != serialized {
				t.Errorf("SerializeDate(%q): got %q, want %q", d, serialized, want)
			}
		}
	}
}

func TestParseDateFromJSON_Invalid(t *testing.T) {
	loc, _ := time.LoadLocation("Asia/Shanghai")
	tests := []string{
		"2018-6-1",
		"236-06-01",
		"12345",
		"2018-02-18Z",
		"2019-02-29",
	}
	for _, test := range tests {
		messages := []proto.Message{
			&d3pb.Date{},
			&d4pb.Date{},
		}
		for _, m := range messages {
			if err := ParseDateFromString(test, loc, m); err == nil {
				t.Errorf("ParseDateFromJSON(%q, %q, %T) succeeded, want error", test, loc, m)
			}
		}
	}
}

func newDateTimesForTest(val int64, precName string, tz string) []proto.Message {
	ret := []proto.Message{
		&d3pb.DateTime{ValueUs: val, Timezone: tz},
		&d4pb.DateTime{ValueUs: val, Timezone: tz},
	}
	for _, p := range ret {
		rp := p.ProtoReflect()
		precEnum, _ := accessor.GetEnumDescriptor(rp.Descriptor(), "precision")
		prec := precEnum.Values().ByName(protoreflect.Name(precName)).Number()
		_ = accessor.SetValue(rp, prec, "precision")
	}
	return ret
}

func TestDateTime(t *testing.T) {
	tests := []struct {
		json    string
		val     int64
		prec    string
		protoTz string
		tz      string
	}{
		{
			"2014-10-09T14:58:00Z",
			1412866680000000,
			"SECOND",
			"Z",
			"UTC",
		},
		{
			"2014-10-09T14:58:00.000000Z",
			1412866680000000,
			"MICROSECOND",
			"Z",
			"UTC",
		},
		{
			"2014-10-09T14:58:00.123Z",
			1412866680123000,
			"MILLISECOND",
			"Z",
			"UTC",
		},
		{
			"2014-10-09T14:58:00+11:00",
			1412827080000000,
			"SECOND",
			"+11:00",
			"UTC",
		},
		{
			"2014-10-09T14:58:00-11:30",
			1412908080000000,
			"SECOND",
			"-11:30",
			"UTC",
		},
		{
			"2014-10-09T14:58:00.123456-11:30",
			1412908080123456,
			"MICROSECOND",
			"-11:30",
			"UTC",
		},
		{
			"2017-12-31",
			1514707200000000,
			"DAY",
			"America/Los_Angeles",
			"America/Los_Angeles", // GMT-7
		},
		{
			"2017-12",
			1512115200000000,
			"MONTH",
			"America/Los_Angeles",
			"America/Los_Angeles",
		},
		{
			"2017",
			1483257600000000,
			"YEAR",
			"America/Los_Angeles",
			"America/Los_Angeles",
		},
		{
			"2018-01-01",
			1514736000000000,
			"DAY",
			"Asia/Shanghai",
			"Asia/Shanghai", // GMT+8
		},
		{
			"2018-01-01",
			1514793600000000,
			"DAY",
			"America/Los_Angeles",
			"America/Los_Angeles",
		},
		{
			"2018-01",
			1514736000000000,
			"MONTH",
			"Asia/Shanghai",
			"Asia/Shanghai",
		},
		{
			"2018",
			1514736000000000,
			"YEAR",
			"Asia/Shanghai",
			"Asia/Shanghai",
		},
	}
	for _, test := range tests {
		t.Run(test.json, func(t *testing.T) {
			l, _ := time.LoadLocation(test.tz)
			dts := newDateTimesForTest(test.val, test.prec, test.protoTz)
			for _, dt := range dts {
				parsed := dt.ProtoReflect().Interface().(proto.Message)
				if err := ParseDateTimeFromString(test.json, l, parsed); err != nil {
					t.Fatalf("parseDateTimeFromJSON(%q, %q, %T): %v", test.json, l, parsed, err)
				}
				if want := dt; !cmp.Equal(want, parsed, protocmp.Transform()) {
					t.Errorf("ParseDateTimeFromJSON(%q, %q, %T): got %v, want %v", test.json, l, parsed, parsed, want)
				}

				serialized, err := SerializeDateTime(dt)
				if err != nil {
					t.Fatalf("SerializeDateTime(%q): %v", dt, err)
				}
				if want := string(test.json); want != serialized {
					t.Errorf("SerializeDateTime(%q): got %q, want %q", dt, serialized, want)
				}
			}
		})
	}
}

func TestParseDateTimeFromJSON_Invalid(t *testing.T) {
	loc, _ := time.LoadLocation("Asia/Shanghai")
	tests := []string{
		"2018-06-01 12:00",
		"2018-06-01T12:61",
		"2018-06-01T12:61-07:00",
		"2018-06-01T12:34:56.789+14:61",
		"2018-06-01Z",
		"2018-02-29T00:00:00Z",
	}
	for _, test := range tests {
		messages := []proto.Message{
			&d3pb.DateTime{},
			&d4pb.DateTime{},
		}
		for _, m := range messages {
			if err := ParseDateTimeFromString(test, loc, m); err == nil {
				t.Errorf("ParseDateTimeFromString(%q, %q, %T) succeeded, want error", test, loc, m)
			}
		}
	}
}

func newTimesForTest(val int64, precName string) []proto.Message {
	ret := []proto.Message{
		&d3pb.Time{ValueUs: val},
		&d4pb.Time{ValueUs: val},
	}
	for _, p := range ret {
		rp := p.ProtoReflect()
		precEnum, _ := accessor.GetEnumDescriptor(rp.Descriptor(), "precision")
		prec := precEnum.Values().ByName(protoreflect.Name(precName)).Number()
		_ = accessor.SetValue(rp, prec, "precision")
	}
	return ret
}

func TestTimeProto(t *testing.T) {
	tests := []struct {
		json string
		val  int64
		prec string
	}{
		{
			"12:00:00",
			43200000000,
			"SECOND",
		},
		{
			"12:00:00.000",
			43200000000,
			"MILLISECOND",
		},
		{
			"12:00:00.000000",
			43200000000,
			"MICROSECOND",
		},
	}
	for _, test := range tests {
		ts := newTimesForTest(test.val, test.prec)
		for _, tm := range ts {
			parsed := tm.ProtoReflect().New().Interface().(proto.Message)
			if err := ParseTime(json.RawMessage(strconv.Quote(test.json)), parsed); err != nil {
				t.Fatalf("parseTime(m, %q): %v", test.json, err)
			}
			if want := tm; !cmp.Equal(want, parsed, protocmp.Transform()) {
				t.Errorf("parseTime(m, %q): got %v, want %v", test.json, parsed, want)
			}

			serialized, err := SerializeTime(tm)
			if err != nil {
				t.Fatalf("SerializeTime(%q): %v", tm, err)
			}
			if want := test.json; want != serialized {
				t.Errorf("SerializeTime(%q): got %q, want %q", tm, serialized, want)
			}
		}
	}
}

func newInstantsForTest(val int64, precName string, tz string) []proto.Message {
	ret := []proto.Message{
		&d3pb.Instant{ValueUs: val, Timezone: tz},
		&d4pb.Instant{ValueUs: val, Timezone: tz},
	}
	for _, p := range ret {
		rp := p.ProtoReflect()
		precEnum, _ := accessor.GetEnumDescriptor(rp.Descriptor(), "precision")
		prec := precEnum.Values().ByName(protoreflect.Name(precName)).Number()
		_ = accessor.SetValue(rp, prec, "precision")
	}
	return ret
}

func TestInstant(t *testing.T) {
	tests := []struct {
		name      string
		datetime  string
		value     int64
		protoTz   string
		precision string
	}{
		{
			"Start of unix epoch time with second precision",
			"1970-01-01T00:00:00+00:00",
			0,
			"+00:00",
			"SECOND",
		},
		{
			"Datetime at a random second in UTC timezone with second precision",
			"2012-06-03T23:45:32Z",
			1338767132000000,
			"Z",
			"SECOND",
		},
		{
			"Datetime on the hour in UTC timezone with second precision",
			"2013-12-09T11:00:00Z",
			1386586800000000,
			"Z",
			"SECOND",
		},
		{
			"Datetime with nanosecond in UTC timezone with ms precision",
			"2013-12-09T11:00:00.123Z",
			1386586800123000,
			"Z",
			"MILLISECOND",
		},
		{
			"Datetime in EST timezone with second precision",
			"2009-09-28T02:37:43-05:00",
			1254123463000000,
			"-05:00",
			"SECOND",
		},
		{
			"Datetime with nanosecond in EST timezone with ms precision",
			"2009-09-28T02:37:43.123-05:00",
			1254123463123000,
			"-05:00",
			"MILLISECOND",
		},
		{
			"Datetime with nanosecond in EST timezone with microsecond precision",
			"2009-09-28T02:37:43.123456-05:00",
			1254123463123456,
			"-05:00",
			"MICROSECOND",
		},
		{
			"Datetime with nanosecond with trailing zeros in EST timezone with microsecond precision",
			"2009-09-28T02:37:43.123000-05:00",
			1254123463123000,
			"-05:00",
			"MICROSECOND",
		},
		{
			"Datetime with nanosecond in UTC timezone with microsecond precision",
			"2013-12-09T11:00:00.123456Z",
			1386586800123456,
			"Z",
			"MICROSECOND",
		},
	}
	for _, test := range tests {
		instants := newInstantsForTest(test.value, test.precision, test.protoTz)
		for _, it := range instants {
			parsed := it.ProtoReflect().New().Interface().(proto.Message)
			if err := ParseInstant(json.RawMessage(strconv.Quote(test.datetime)), parsed); err != nil {
				t.Fatalf("%s ParseInstant(%q, %T): %v", test.name, test.datetime, parsed, err)
			}
			if want := it; !cmp.Equal(want, parsed, protocmp.Transform()) {
				t.Errorf("%s ParseInstant(%q): got %v, want %v", test.name, test.datetime, parsed, want)
			}

			serialized, err := SerializeInstant(it)
			if err != nil {
				t.Fatalf("%s SerializeInstant(%q): %v", test.name, it, err)
			}
			if want := test.datetime; want != serialized {
				t.Errorf("%s SerializeInstant(%q): got %q, want %q", test.name, it, serialized, want)
			}
		}
	}
}

func TestParseInstant_Invalid(t *testing.T) {
	tests := []struct {
		instant json.RawMessage
	}{
		{
			json.RawMessage(`"2013-12-09T11:00:00"`),
		},
		{
			json.RawMessage(`"2013-12-09T11:00Z"`),
		},
		{
			json.RawMessage(`"2013-12-09T11:00:00+15:00"`),
		},
	}
	for _, test := range tests {
		messages := []proto.Message{
			&d3pb.Instant{},
			&d4pb.Instant{},
		}
		for _, m := range messages {
			if err := ParseInstant(test.instant, m); err == nil {
				t.Errorf("ParseInstant(%q, %T) succeeded, want error", test.instant, m)
			}
		}
	}
}

func TestSerializeInstant_Invalid(t *testing.T) {
	tests := []struct {
		name   string
		protos []proto.Message
	}{
		{
			"zero'd precision",
			[]proto.Message{
				&d3pb.Instant{Timezone: "UTC", Precision: d3pb.Instant_PRECISION_UNSPECIFIED},
				&d4pb.Instant{Timezone: "UTC", Precision: d4pb.Instant_PRECISION_UNSPECIFIED},
			},
		},
		{
			"invalid timezone",
			[]proto.Message{
				&d3pb.Instant{Timezone: "XYZ", Precision: d3pb.Instant_SECOND},
				&d4pb.Instant{Timezone: "XYZ", Precision: d4pb.Instant_SECOND},
			},
		},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			for _, p := range test.protos {
				_, err := SerializeInstant(p)
				if err == nil {
					t.Errorf("SerializeInstant(%q) succeeded, want error", p)
				}
			}
		})
	}
}
