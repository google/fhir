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
			msg:             &pb.Integer{},
			json:            `"x"`,
			wantErrContains: "regex",
		},
		{
			msg:             &pb.Integer{},
			json:            fmt.Sprintf("%d", math.MaxInt64), // only supports 32-bit
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
				t.Fatalf("UnmarshalString(%s, %T) got err %v; want containing", json, got, err, tt.wantErrContains)
			}
		})
	}
}
