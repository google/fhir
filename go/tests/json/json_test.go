package json

import (
	"fmt"
	"math"
	"reflect"
	"strings"
	"testing"

	"github.com/golang/protobuf/jsonpb"
	"github.com/golang/protobuf/proto"

	pb "github.com/google/fhir/proto/stu3"
)

// newEmptyProto returns a new proto.Message with the same concrete type as msg.
func newEmptyProto(t *testing.T, msg proto.Message) proto.Message {
	t.Helper()
	// TODO(arrans) is there a simpler way to get a new(x) without
	// explicitly having the type?
	concrete := reflect.ValueOf(msg).Elem().Type()
	empty, ok := reflect.New(concrete).Interface().(proto.Message)
	if !ok {
		// If this happens then the test is badly coded, not actually failing.
		t.Fatalf("bad test setup; got ok==false when casting zero-valued proto message to Message interface")
	}
	return empty
}

func TestGoodConversions(t *testing.T) {
	tests := []struct {
		msg  proto.Message
		json string
	}{
		{
			msg: &pb.Base64Binary{
				Value: []byte("foo\x00bar"),
			},
			json: `"Zm9vAGJhcg=="`,
		},
		{
			msg: &pb.Boolean{
				Value: true,
			},
			json: `true`,
		},
		{
			msg: &pb.Boolean{
				Value: false,
			},
			json: `false`,
		},
		{
			msg: &pb.Date{
				ValueUs:   504921600000000,
				Precision: pb.Date_YEAR,
			},
			json: `"1986"`,
		},
		{
			msg: &pb.Date{
				ValueUs:   528508800000000,
				Precision: pb.Date_MONTH,
			},
			json: `"1986-10"`,
		},
		{
			msg: &pb.Date{
				ValueUs:   529977600000000,
				Precision: pb.Date_DAY,
			},
			json: `"1986-10-18"`,
		},
		{
			msg: &pb.Integer{
				Value: 0,
			},
			json: `0`,
		},
		{
			msg: &pb.Integer{
				Value: 42,
			},
			json: `42`,
		},
		{
			msg: &pb.Integer{
				Value: -42,
			},
			json: `-42`,
		},
		{
			msg: &pb.Integer{
				Value: math.MaxInt32,
			},
			json: `2147483647`,
		},
		{
			msg: &pb.Integer{
				Value: math.MinInt32,
			},
			json: `-2147483648`,
		},
		{
			msg: &pb.PositiveInt{
				Value: 1,
			},
			json: `1`,
		},
		{
			msg: &pb.PositiveInt{
				Value: 42,
			},
			json: `42`,
		},
		{
			msg: &pb.PositiveInt{
				Value: math.MaxUint32,
			},
			json: `4294967295`,
		},
		{
			msg: &pb.String{
				Value: `hello world`,
			},
			json: `"hello world"`,
		},
		{
			msg: &pb.String{
				Value: `"double" 'single'`,
			},
			json: `"\"double\" 'single'"`,
		},
		{
			msg: &pb.UnsignedInt{
				Value: 0,
			},
			json: `0`,
		},
		{
			msg: &pb.UnsignedInt{
				Value: 42,
			},
			json: `42`,
		},
		{
			msg: &pb.UnsignedInt{
				Value: math.MaxUint32,
			},
			json: `4294967295`,
		},
	}

	for _, tt := range tests {
		msg := tt.msg
		json := tt.json
		t.Run(fmt.Sprintf("%T", msg), func(t *testing.T) {
			t.Run("proto to JSON", func(t *testing.T) {
				got, err := (&jsonpb.Marshaler{}).MarshalToString(msg)
				if err != nil {
					t.Fatalf("MarshalToString(%+v) got err %v; want nil err", msg, err)
				}
				if want := json; got != want {
					t.Errorf("marshalling %T(%+v) got JSON %s; want %s", msg, msg, got, want)
				}
			})

			t.Run("JSON to proto", func(t *testing.T) {
				got := newEmptyProto(t, msg)
				if err := jsonpb.UnmarshalString(json, got); err != nil {
					t.Fatalf("UnmarshalString(%s, %T) got err %v; want nil err", json, got, err)
				}
				if want := msg; !proto.Equal(got, want) {
					t.Errorf("unmarshalling JSON %s got %+v; want %+v", json, got, want)
				}
			})
		})
	}
}

func TestBadJSON(t *testing.T) {
	tests := []struct {
		// msg is used merely to define the type to which the JSON should be
		// unmarshalled.
		msg             proto.Message
		json            string
		wantErrContains string
	}{
		{
			msg:             &pb.Date{},
			json:            `"1986-"`,
			wantErrContains: "regex",
		},
		{
			msg:             &pb.Integer{},
			json:            `"x"`,
			wantErrContains: "regex",
		},
		{
			msg:             &pb.Integer{},
			json:            fmt.Sprintf("%d", int64(math.MaxInt32)+1),
			wantErrContains: "32",
		},
		{
			msg:             &pb.Integer{},
			json:            fmt.Sprintf("%d", int64(math.MinInt32)-1),
			wantErrContains: "32",
		},
		{
			msg:             &pb.PositiveInt{},
			json:            `"x"`,
			wantErrContains: "regex",
		},
		{
			msg:  &pb.PositiveInt{},
			json: `0`,
			// there is also an explicit test, but the regex catches it first
			wantErrContains: "regex",
		},
		{
			msg:             &pb.PositiveInt{},
			json:            fmt.Sprintf("%d", uint64(math.MaxUint32)+1),
			wantErrContains: "32",
		},
		{
			msg:             &pb.String{},
			json:            `""`,
			wantErrContains: "empty",
		},
		{
			msg:             &pb.UnsignedInt{},
			json:            `"x"`,
			wantErrContains: "regex",
		},
		{
			msg:             &pb.UnsignedInt{},
			json:            fmt.Sprintf("%d", uint64(math.MaxUint32)+1),
			wantErrContains: "32",
		},
	}

	for _, tt := range tests {
		msg := tt.msg
		json := tt.json
		t.Run(fmt.Sprintf("%T", msg), func(t *testing.T) {
			got := newEmptyProto(t, msg)
			if err := jsonpb.UnmarshalString(json, got); err == nil {
				t.Fatalf("UnmarshalString(%s, %T) got nil err; want err", json, got)
			} else if !strings.Contains(err.Error(), tt.wantErrContains) {
				t.Fatalf("UnmarshalString(%s, %T) got err %v; want containing %q", json, got, err, tt.wantErrContains)
			}
		})
	}
}

func TestBadProto(t *testing.T) {
	tests := []struct {
		// msg is used merely to define the type to which the JSON should be
		msg             proto.Message
		wantErrContains string
	}{
		{
			msg: &pb.Date{
				Timezone: "+0100",
			},
			wantErrContains: "zone",
		},
		{
			msg: &pb.String{
				Value: "",
			},
			wantErrContains: "empty",
		},
	}

	for _, tt := range tests {
		msg := tt.msg
		t.Run(fmt.Sprintf("%T", msg), func(t *testing.T) {
			if _, err := (&jsonpb.Marshaler{}).MarshalToString(msg); err == nil {
				t.Fatalf("MarshalToString(%T(%+v)) got nil err; want err", msg, msg)
			} else if !strings.Contains(err.Error(), tt.wantErrContains) {
				t.Fatalf("MarshalToString(%T(%+v)) got err %v; want containing %q", msg, msg, err, tt.wantErrContains)
			}
		})
	}
}

// TODO(arrans) test pb.Date.Time() rounding
