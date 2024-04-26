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

// Package integration_test contains integration tests for the jsonformatter
package integration_test

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"os"
	"path"
	"path/filepath"
	"reflect"
	"strings"
	"testing"

	"runtime"

	"github.com/bazelbuild/rules_go/go/tools/bazel"
	"github.com/google/fhir/go/fhirversion"
	"github.com/google/fhir/go/jsonformat/internal/jsonpbhelper"
	"github.com/google/fhir/go/jsonformat"
	"github.com/google/fhir/go/protopath"
	"github.com/google/go-cmp/cmp"
	"google.golang.org/protobuf/encoding/prototext"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/testing/protocmp"
	"bitbucket.org/creachadair/stringset"

	r4pb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/bundle_and_contained_resource_go_proto"
	r4compositionpb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/composition_go_proto"
	r4observationpb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/observation_go_proto"
	r4patientpb "github.com/google/fhir/go/proto/google/fhir/proto/r4/core/resources/patient_go_proto"
	r3pb "github.com/google/fhir/go/proto/google/fhir/proto/stu3/resources_go_proto"
)

const (
	timeZone               = "Australia/Sydney"
	oneofResourceProtopath = "oneof_resource"
	jsonExt     = "json"
	txtprotoExt = "prototxt"
)

var (
	stu3Ignores = stringset.New(
		// Invalid "value_integer" JSON field.
		"Bundle-dataelements",
		// Missing required field "resourceType".
		"package",
		// Invalid "license" JSON field.
		"ig-r4",
		// Missing required field "type"
		"StructureDefinition-Request",
		"StructureDefinition-Event",
		"StructureDefinition-Definition",
		// Missing required field "base".
		"SearchParameter-valueset-extensions-ValueSet-workflow",
		"SearchParameter-valueset-extensions-ValueSet-keyword",
		"SearchParameter-valueset-extensions-ValueSet-effective",
		"SearchParameter-valueset-extensions-ValueSet-author",
		"SearchParameter-valueset-extensions-ValueSet-end",
		"SearchParameter-organization-extensions-Organization-alias",
		"SearchParameter-location-extensions-Location-alias",
		"SearchParameter-codesystem-extensions-CodeSystem-keyword",
		"SearchParameter-codesystem-extensions-CodeSystem-workflow",
		"SearchParameter-codesystem-extensions-CodeSystem-end",
		"SearchParameter-codesystem-extensions-CodeSystem-author",
		"SearchParameter-codesystem-extensions-CodeSystem-effective",
		// Missing required field "linkID".
		"Questionnaire-qs1",
		// Missing required field "name".
		"ImplementationGuide-fhir",
		// Failing FHIRPath constraints.
		"DataElement-base64Binary.id",
		"DataElement-boolean.id",
		"DataElement-date.id",
		"DataElement-dateTime.id",
		"DataElement-decimal.id",
		"DataElement-DomainResource.modifierExtension",
		"DataElement-DomainResource.extension",
		"DataElement-Element.extension",
		"DataElement-instant.id",
		"DataElement-integer.id",
		"DataElement-string.id",
		"DataElement-time.id",
		"DataElement-uri.id",
		"DataElement-xhtml.id",
		// Other ignores.
		"Patient-null",
	)
	r4Ignores = stringset.New(
		// Invalid "value_integer" JSON field.
		"Bundle-dataelements",
		// Version fields aren't equal between JSON and prototxt.
		"ig-r4",
		// Missing required field resourceType.
		"package",
		// Missing required field "base".
		"SearchParameter-valueset-extensions-ValueSet-workflow",
		"SearchParameter-valueset-extensions-ValueSet-keyword",
		"SearchParameter-valueset-extensions-ValueSet-effective",
		"SearchParameter-valueset-extensions-ValueSet-author",
		"SearchParameter-valueset-extensions-ValueSet-end",
		"SearchParameter-codesystem-extensions-CodeSystem-keyword",
		"SearchParameter-codesystem-extensions-CodeSystem-workflow",
		"SearchParameter-codesystem-extensions-CodeSystem-end",
		"SearchParameter-codesystem-extensions-CodeSystem-author",
		"SearchParameter-codesystem-extensions-CodeSystem-effective",
		// Invalid ID at SearchParameter.id.
		"SearchParameter-questionnaireresponse-extensions-QuestionnaireResponse-item-subject",
		// Missing required field "linkID".
		"Questionnaire-qs1",
		// Invalid references.
		"ImplementationGuide-fhir",
		"Observation-clinical-gender",
		"MedicationRequest-medrx0301",
		"DeviceMetric-example",
		"DeviceUseStatement-example",
		// Failing FHIRPath constraints.
		"StructureDefinition-Definition",
		"StructureDefinition-Event",
		"StructureDefinition-FiveWs",
		"StructureDefinition-Request",
	)

	versionToJSONPath = map[fhirversion.Version]string{
		fhirversion.STU3:  "spec/hl7.fhir.core/3.0.1/package/",
		fhirversion.R4:    "spec/hl7.fhir.r4.examples/4.0.1/package/",
	}
	versionToProtoPath = map[fhirversion.Version]string{
		fhirversion.STU3: "testdata/stu3/examples",
		fhirversion.R4:   "testdata/r4/examples",
	}
	versionToBigQueryJSONPath = map[fhirversion.Version]string{
		fhirversion.STU3: "testdata/stu3/bigquery/",
		fhirversion.R4:   "testdata/r4/bigquery/",
	}
)

func TestMarshal_STU3(t *testing.T) {
	t.Parallel()
	tests := readTestCases(t, fhirversion.STU3)
	for _, tc := range tests {
		tc := tc
		t.Run(tc, func(t *testing.T) {
			t.Parallel()
			if stu3Ignores.Contains(tc) {
				t.Skipf("Skipping %s", tc)
			}
			testMarshal(t, tc, fhirversion.STU3)
		})
	}
}

func TestMarshal_R4(t *testing.T) {
	t.Parallel()
	tests := readTestCases(t, fhirversion.R4)
	for _, tc := range tests {
		tc := tc
		t.Run(tc, func(t *testing.T) {
			t.Parallel()
			if r4Ignores.Contains(tc) {
				t.Skipf("Skipping %s", tc)
			}
			testMarshal(t, tc, fhirversion.R4)
		})
	}
}

func testMarshal(t *testing.T, name string, ver fhirversion.Version) {
	jsonData, protoData, err := readTestCaseFile(ver, name)
	if err != nil {
		t.Fatal(err)
	}
	res, err := getZeroResource(jsonData, ver)
	if err != nil {
		t.Fatalf("Failed to get empty resource from JSON data %v", err)
	}
	if err = prototext.Unmarshal(protoData, res); err != nil {
		t.Fatalf("Failed to unmarshal resource text proto: %v", err)
	}
	containedRes, err := wrapInContainedResource(res, ver)
	if err != nil {
		t.Fatalf("Failed to wrap resource in contained resource: %v", err)
	}

	m, err := jsonformat.NewPrettyMarshaller(ver)
	if err != nil {
		t.Fatalf("Failed to create marshaller: %v", err)
	}
	marshalled, err := m.Marshal(containedRes)
	if err != nil {
		t.Fatalf("Failed to marshall containedRes emptyRes: %v", err)
	}

	var got, want interface{}
	if err := json.Unmarshal(marshalled, &got); err != nil {
		t.Fatalf("Failed to unmarshal JSON")
	}
	if err := json.Unmarshal(jsonData, &want); err != nil {
		t.Fatalf("Failed to unmarshal JSON")
	}
	if diff := cmp.Diff(want, got); diff != "" {
		t.Errorf("Unexpected diff between want and marshalled %v: (-want, +got) %v", name, diff)
	}
}

// TODO(b/156023087): add tests and goldens for analytics marshaller with inferred schema.
func TestMarshalAnalytics_STU3(t *testing.T) {
	t.Parallel()
	tests := []struct {
		name    string
		files   []string
		zeroRes proto.Message
	}{
		{
			name:    "composition",
			files:   []string{"Composition-example"},
			zeroRes: &r3pb.Composition{},
		},
		{
			name:    "observation",
			files:   []string{"Observation-example-genetics-1"},
			zeroRes: &r3pb.Observation{},
		},
		{
			name:    "patient",
			files:   []string{"patient-example"},
			zeroRes: &r3pb.Patient{},
		},
	}
	for _, tc := range tests {
		tc := tc
		t.Run(tc.name, func(t *testing.T) {
			t.Parallel()
			for _, file := range tc.files {
				testMarshalAnalytics(t, file, tc.zeroRes, fhirversion.STU3)
			}
		})
	}
}

func TestMarshalAnalytics_R4(t *testing.T) {
	t.Parallel()
	tests := []struct {
		name    string
		files   []string
		zeroRes proto.Message
	}{
		{
			name:    "composition",
			files:   []string{"Composition-example"},
			zeroRes: &r4compositionpb.Composition{},
		},
		{
			name:    "observation",
			files:   []string{"Observation-example-genetics-1"},
			zeroRes: &r4observationpb.Observation{},
		},
		{
			name:    "patient",
			files:   []string{"Patient-example"},
			zeroRes: &r4patientpb.Patient{},
		},
	}
	for _, tc := range tests {
		tc := tc
		t.Run(tc.name, func(t *testing.T) {
			t.Parallel()
			for _, file := range tc.files {
				testMarshalAnalytics(t, file, tc.zeroRes, fhirversion.R4)
			}
		})
	}
}

func testMarshalAnalytics(t *testing.T, file string, res proto.Message, ver fhirversion.Version) {
	jsonData, err := readFile(versionToBigQueryJSONPath[ver], file, jsonExt)
	if err != nil {
		t.Fatalf("Failed to read resource json file %s: %v", file, err)
	}
	protoData, err := readFile(versionToProtoPath[ver], file, txtprotoExt)
	if err != nil {
		t.Fatalf("Failed to read resource text proto file %s: %v", file, err)
	}

	if err := prototext.Unmarshal(protoData, res); err != nil {
		t.Fatalf("Failed to unmarshal resource text proto: %v", err)
	}
	containedRes, err := wrapInContainedResource(res, ver)
	if err != nil {
		t.Fatalf("Failed to wrap resource in contained resource: %v", err)
	}

	m, err := jsonformat.NewAnalyticsMarshaller(0, ver)
	if err != nil {
		t.Fatalf("Failed to create marshaller: %v", err)
	}
	marshalled, err := m.Marshal(containedRes)
	if err != nil {
		t.Fatalf("Failed to marshall containedRes emptyRes: %v", err)
	}

	var got, want interface{}
	if err := json.Unmarshal(marshalled, &got); err != nil {
		t.Fatalf("Failed to unmarshal JSON")
	}
	if err := json.Unmarshal(jsonData, &want); err != nil {
		t.Fatalf("Failed to unmarshal JSON")
	}
	if diff := cmp.Diff(want, got); diff != "" {
		t.Errorf("Unexpected diff between want and marshalled %v: (-want, +got) %v", file, diff)
	}
}

func TestUnmarshal_STU3(t *testing.T) {
	t.Parallel()
	tests := readTestCases(t, fhirversion.STU3)
	for _, tc := range tests {
		tc := tc
		t.Run(tc, func(t *testing.T) {
			t.Parallel()
			if stu3Ignores.Contains(tc) {
				t.Skipf("Skipping %s", tc)
			}
			testUnmarshal(t, tc, fhirversion.STU3)
		})
	}
}

func TestUnmarshal_R4(t *testing.T) {
	t.Parallel()
	tests := readTestCases(t, fhirversion.R4)
	for _, tc := range tests {
		tc := tc
		t.Run(tc, func(t *testing.T) {
			t.Parallel()
			if r4Ignores.Contains(tc) {
				t.Skipf("Skipping %s", tc)
			}
			testUnmarshal(t, tc, fhirversion.R4)
		})
	}
}

func testUnmarshal(t *testing.T, name string, ver fhirversion.Version) {
	jsonData, protoData, err := readTestCaseFile(ver, name)
	if err != nil {
		t.Fatal(err)
	}
	um, err := jsonformat.NewUnmarshaller(timeZone, ver)
	if err != nil {
		t.Fatalf("Failed to create unmarshaller: %v", err)
	}
	unmarshalled, err := um.Unmarshal(jsonData)
	if err != nil {
		t.Fatalf("Failed to unmarshal resource JSON: %v", err)
	}
	res, err := unwrapFromContainedResource(unmarshalled)
	if err != nil {
		t.Fatalf("Failed to unwrap resource: %v", err)
	}

	want, err := getZeroResource(jsonData, ver)
	if err != nil {
		t.Fatalf("Failed to get zero resource from JSON %v", err)
	}
	if err = prototext.Unmarshal(protoData, want); err != nil {
		t.Fatalf("Failed to unmarshal resource text proto: %v", err)
	}
	if diff := cmp.Diff(want, res, protocmp.Transform()); diff != "" {
		t.Errorf("Unexpected diff between want and unmarshalled %v: (-want, +got) %v", name, diff)
	}
}

func TestMarshalUnmarshalIdentity_STU3(t *testing.T) {
	t.Parallel()
	tests := readTestCaseFileNames(t, versionToJSONPath[fhirversion.STU3])
	for _, tc := range tests {
		tc := tc
		t.Run(tc, func(t *testing.T) {
			t.Parallel()
			if stu3Ignores.Contains(tc) {
				t.Skipf("Skipping %s", tc)
			}
			testMarshalUnmarshalIdentity(t, tc, fhirversion.STU3)
		})
	}
}

func TestMarshalUnmarshalIdentity_R4(t *testing.T) {
	t.Parallel()
	tests := readTestCaseFileNames(t, versionToJSONPath[fhirversion.R4])
	for _, tc := range tests {
		tc := tc
		t.Run(tc, func(t *testing.T) {
			t.Parallel()
			if r4Ignores.Contains(tc) {
				t.Skipf("Skipping %s", tc)
			}
			testMarshalUnmarshalIdentity(t, tc, fhirversion.R4)
		})
	}
}

// testMarshalUnmarshalIdentity tests that the unmarshalling and marshalling a
// JSON resource results in equal JSON to the original.
func testMarshalUnmarshalIdentity(t *testing.T, fileName string, ver fhirversion.Version) {
	t.Helper()
	jsonData, err := readFile(versionToJSONPath[ver], fileName, jsonExt)
	if err != nil {
		t.Fatalf("Failed to get empty resource from JSON data %v", err)
	}

	um, err := jsonformat.NewUnmarshaller("UTC", ver)
	if err != nil {
		t.Fatalf("failed to create unmarshaler: %v", err)
	}
	unmarshalled, err := um.Unmarshal(jsonData)
	if err != nil {
		t.Fatalf("unmarshal: %v", err)
	}
	m, err := jsonformat.NewPrettyMarshaller(ver)
	if err != nil {
		t.Fatalf("failed to create marshaler: %v", err)
	}
	marshalled, err := m.Marshal(unmarshalled)
	if err != nil {
		t.Fatalf("marshal: %v", err)
	}

	if diff := diffJSON(jsonData, marshalled, ""); diff != "" {
		t.Errorf("Unexpected diff between want and marshalled %v: %v", fileName, diff)
	}
}

// getZeroResource returns a zeroed version of a resource from its JSON.
func getZeroResource(jsonData []byte, ver fhirversion.Version) (proto.Message, error) {
	cr, err := newContainedResource(ver)
	if err != nil {
		return nil, err
	}

	var unmarshalled map[string]interface{}
	if err := json.Unmarshal(jsonData, &unmarshalled); err != nil {
		return nil, err
	}
	resType, ok := unmarshalled[jsonpbhelper.ResourceTypeField]
	if !ok {
		return nil, fmt.Errorf("failed getting resource type from resource JSON")
	}

	rcr := cr.ProtoReflect()
	crDesc := rcr.Descriptor()
	oneofDesc := crDesc.Oneofs().ByName(jsonpbhelper.OneofName)
	if oneofDesc == nil {
		return nil, fmt.Errorf("oneof field not found: %v", jsonpbhelper.OneofName)
	}
	for i := 0; i < oneofDesc.Fields().Len(); i++ {
		f := oneofDesc.Fields().Get(i)
		if f.Message() != nil && string(f.Message().Name()) == resType.(string) {
			return rcr.Mutable(f).Message().Interface().(proto.Message), nil
		}
	}

	return nil, fmt.Errorf("failed to get zero resource from JSON")
}

// unwrapFromContainedResource returns the resource contained within a contained resource.
func unwrapFromContainedResource(containedRes proto.Message) (proto.Message, error) {
	return protopath.Get[proto.Message](containedRes, protopath.NewPath(oneofResourceProtopath))
}

// wrapInContainedResource wraps a resource within a contained resource.
func wrapInContainedResource(res proto.Message, ver fhirversion.Version) (proto.Message, error) {
	contained, err := newContainedResource(ver)
	if err != nil {
		return nil, err
	}

	// Resource types are of the form "*.Type" where "Type" is the resource type.
	resType := reflect.TypeOf(res).String()
	idx := strings.LastIndex(resType, ".")
	if idx == -1 {
		return nil, fmt.Errorf("failed to parse resource type %s", resType)
	}
	resType = resType[idx+1:]

	resPath := protopath.NewPath(fmt.Sprintf("%s.%s", oneofResourceProtopath, jsonpbhelper.CamelToSnake(resType)))
	if err := protopath.Set(contained, resPath, res); err != nil {
		return nil, err
	}

	return contained, nil
}

func newContainedResource(ver fhirversion.Version) (proto.Message, error) {
	switch ver {
	case fhirversion.STU3:
		return &r3pb.ContainedResource{}, nil
	case fhirversion.R4:
		return &r4pb.ContainedResource{}, nil
	default:
		panic(fmt.Sprintf("Invalid version specified %v", ver))
	}
}

var (
	_, b, _, _ = runtime.Caller(0)
	callerRoot = filepath.Dir(b)
)
// getRootPath returns the root bazel runfiles path if running in a bazel
// environment, otherwise it will return the root path of the FHIR proto
// repository. Typically this is used to access files such as testdata.
// As usual, the caller should check to see if the expected file(s)
// exist, and if not, report an error.
func getRootPath() string {
	var root string
 	root, err := bazel.RunfilesPath()
 	if err != nil {
		// Fall back to the non-bazel way to get to the root directory.
		root = callerRoot + "/../../../../"
	}
	return root
}

// readTestCases reads the JSON example files and returns the ones that have a
// matching prototxt file, to be used as a test case.
func readTestCases(t *testing.T, ver fhirversion.Version) []string {
	t.Helper()
	fileNames := readTestCaseFileNames(t, versionToJSONPath[ver])
	var withGolden []string
	for _, fileName := range fileNames {
		fullPath := path.Join(getRootPath(), versionToProtoPath[ver], fmt.Sprintf("%s.%s", fileName, txtprotoExt))
		if _, err := os.Stat(fullPath); err != nil {
			if os.IsNotExist(err) {
				continue
			}
			t.Fatalf("failed to read %s: %v", fullPath, err)
		}
		withGolden = append(withGolden, fileName)
	}
	return withGolden
}

func readTestCaseFile(ver fhirversion.Version, name string) ([]byte, []byte, error) {
	jsonData, err := readFile(versionToJSONPath[ver], name, jsonExt)
	if err != nil {
		return nil, nil, fmt.Errorf("failed to read resource json file %s: %v", name, err)
	}

	protoData, err := readFile(versionToProtoPath[ver], name, txtprotoExt)
	if err != nil {
		return nil, nil, fmt.Errorf("failed to read resource text proto file %s: %v", name, err)
	}

	return jsonData, protoData, nil
}

func readTestCaseFileNames(t *testing.T, basePath string) []string {
	t.Helper()
	files, err := filepath.Glob(path.Join(getRootPath(), fmt.Sprintf("%s*.json", basePath)))
	if err != nil {
		t.Fatalf("Failed to read test cases with err: %v", err)
	}
	var ret []string
	for _, f := range files {
		ret = append(ret, strings.TrimSuffix(filepath.Base(f), ".json"))

	}
	if len(ret) == 0 {
		t.Fatalf("Read 0 test cases for basePath %s", basePath)
	}
	return ret
}

func readFile(basePath, fileName, ext string) ([]byte, error) {
	filePath := path.Join(getRootPath(), basePath, fmt.Sprintf("%s.%s", fileName, ext))
	return ioutil.ReadFile(filePath)
}

func diffJSON(a, b json.RawMessage, indent string) string {
	if bytes.Equal(a, b) {
		return ""
	}
	if diff, err := diffJSONObject(a, b, indent); err == nil {
		return diff
	}
	if diff, err := diffJSONArray(a, b, indent); err == nil {
		return diff
	}
	// A wildcard catch for all other cases.
	return fmt.Sprintf("%v-%q\n%v+%q\n", indent, a, indent, b)
}

// onlyContainField checks if the data only contains a field. To contain field,
// the data has to be a map, then either it directly contains the field or its
// children contain the field.
func onlyContainField(data json.RawMessage, field string) bool {
	// Try to parse it as an object. If it's not an object, then return false.
	var obj map[string]json.RawMessage
	if err := json.Unmarshal(data, &obj); err != nil {
		return false
	}
	delete(obj, field)
	for _, v := range obj {
		if !onlyContainField(v, field) {
			return false
		}
	}
	return true
}

func diffJSONObject(a, b json.RawMessage, indent string) (string, error) {
	// Try unmarshalling both a and b as JSON objects.
	var objA, objB map[string]json.RawMessage
	if err := json.Unmarshal(a, &objA); err != nil {
		return "", err
	}
	if err := json.Unmarshal(b, &objB); err != nil {
		return "", err
	}
	diff := ""
	for k, va := range objA {
		vb, ok := objB[k]
		if !ok {
			diff += fmt.Sprintf("%v%v: -%v +nil\n", indent, k, string(va))
			continue
		}
		// Remove the item from objB.
		delete(objB, k)
		if subDiff := diffJSON(va, vb, indent+"  "); subDiff != "" {
			diff += fmt.Sprintf("%v%v:\n", indent, k)
			diff += subDiff
		}
	}
	for k, vb := range objB {
		diff += fmt.Sprintf("%v%v: -nil +%v\n", indent, k, string(vb))
	}
	return diff, nil
}

func diffJSONArray(a, b json.RawMessage, indent string) (string, error) {
	var objA, objB []json.RawMessage
	if err := json.Unmarshal(a, &objA); err != nil {
		return "", err
	}
	if err := json.Unmarshal(b, &objB); err != nil {
		return "", err
	}
	if len(objA) != len(objB) {
		return fmt.Sprintf("%v-array of %v elements\n%v+array of %v elements", indent, len(objA), indent, len(objB)), nil
	}
	diff := ""
	for i := 0; i < len(objA); i++ {
		if d := diffJSON(objA[i], objB[i], indent+"  "); d != "" {
			diff += fmt.Sprintf("%varray item %v:\n", indent, i)
			diff += d
		}
	}
	return diff, nil
}
