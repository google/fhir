// Package dummy exists purely to ensure proper Go compilation of protos as part
// of bazel tests.
package dummy

import (
	"testing"

	pb "github.com/google/fhir/proto/stu3"
)

func TestGoCompilation(t *testing.T) {
	t.Logf("%+v", pb.Patient{})
}
