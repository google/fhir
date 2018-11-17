package json

import (
	"reflect"
	"testing"

	"github.com/golang/protobuf/jsonpb"
	"github.com/golang/protobuf/proto"

	pb "github.com/google/fhir/proto/stu3"
)

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
	}

	for _, tt := range tests {
		msg := tt.msg
		json := tt.json

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
			concrete := reflect.ValueOf(msg).Elem().Type()
			got, ok := reflect.New(concrete).Interface().(proto.Message)
			if !ok {
				t.Fatalf("bad test setup; got ok==false when casting zero-valued proto message to Message interface")
			}
			if err := jsonpb.UnmarshalString(json, got); err != nil {
				t.Fatalf("UnmarshalString(%s, %T) got err %v; want nil err", json, got, err)
			}
			if want := msg; !proto.Equal(got, want) {
				t.Errorf("unmarshalling JSON %s got %+v; want %+v", got, want)
			}
		})
	}
}
