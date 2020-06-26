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

	"github.com/bazelbuild/rules_go/go/tools/bazel"
	"google/fhir/jsonformat/internal/jsonpbhelper/jsonpbhelper"
	"google/fhir/jsonformat/internal/protopath/protopath"
	"google/fhir/jsonformat/jsonformat"
	"github.com/google/go-cmp/cmp"
	"github.com/golang/protobuf/proto"
	"google.golang.org/protobuf/testing/protocmp"
	"bitbucket.org/creachadair/stringset"

	r4pb "google/fhir/proto/r4/core/resources/bundle_and_contained_resource_go_proto"
	r4compositionpb "google/fhir/proto/r4/core/resources/composition_go_proto"
	r4observationpb "google/fhir/proto/r4/core/resources/observation_go_proto"
	r4patientpb "google/fhir/proto/r4/core/resources/patient_go_proto"
	r3pb "google/fhir/proto/stu3/resources_go_proto"
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
		"Bundle-types",
		"DataElement-integer.value",
		"StructureDefinition-integer",
		// Invalid "reason_reference" JSON field.
		"MedicationDispense-meddisp0303",
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
		// Other ignores.
		"Patient-null",
	)
	r4Ignores = stringset.New(
		// TODO contains timezone after 2038 affected by https://github.com/golang/go/issues/36654
		"Person-pp",
		// TODO: decoding overflows 32 bit float.
		"Observation-decimal",
		// Invalid "reason_reference" JSON field.
		"MedicationDispense-meddisp0303",
		// Invalid "value_integer" JSON field.
		"Bundle-dataelements",
		"Bundle-types",
		"DataElement-integer.value",
		"StructureDefinition-integer",
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
	)

	versionToJSONPath = map[jsonformat.Version]string{
		jsonformat.STU3:  "spec/hl7.fhir.core/3.0.1/package/",
		jsonformat.R4:    "spec/hl7.fhir.r4.examples/4.0.1/package/",
	}
	versionToProtoPath = map[jsonformat.Version]string{
		jsonformat.STU3: "testdata/stu3/examples",
		jsonformat.R4:   "testdata/r4/examples",
	}
	versionToBigQueryJSONPath = map[jsonformat.Version]string{
		jsonformat.STU3: "testdata/stu3/bigquery/",
		jsonformat.R4:   "testdata/r4/bigquery/",
	}
)

func TestMarshal_STU3(t *testing.T) {
	t.Parallel()
	tests := readTestCases(t, jsonformat.STU3)
	for _, tc := range tests {
		tc := tc
		t.Run(tc.name, func(t *testing.T) {
			t.Parallel()
			if stu3Ignores.Contains(tc.name) {
				t.Skipf("Skipping %s", tc.name)
			}
			testMarshal(t, tc, jsonformat.STU3)
		})
	}
}

func TestMarshal_R4(t *testing.T) {
	t.Parallel()
	tests := readTestCases(t, jsonformat.R4)
	for _, tc := range tests {
		tc := tc
		t.Run(tc.name, func(t *testing.T) {
			t.Parallel()
			if r4Ignores.Contains(tc.name) {
				t.Skipf("Skipping %s", tc.name)
			}
			testMarshal(t, tc, jsonformat.R4)
		})
	}
}

func testMarshal(t *testing.T, tc *testCase, ver jsonformat.Version) {
	res, err := getZeroResource(tc.jsonData, ver)
	if err != nil {
		t.Fatalf("Failed to get empty resource from JSON data %v", err)
	}
	if err = proto.UnmarshalText(string(tc.protoData), res); err != nil {
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
	if err := json.Unmarshal(tc.jsonData, &want); err != nil {
		t.Fatalf("Failed to unmarshal JSON")
	}
	if diff := cmp.Diff(want, got); diff != "" {
		t.Errorf("Unexpected diff between want and marshalled %v: (-want, +got) %v", tc.name, diff)
	}
}

// TODO: add tests and goldens for analytics marshaller with inferred schema.
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
				testMarshalAnalytics(t, file, tc.zeroRes, jsonformat.STU3)
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
				testMarshalAnalytics(t, file, tc.zeroRes, jsonformat.R4)
			}
		})
	}
}

func testMarshalAnalytics(t *testing.T, file string, res proto.Message, ver jsonformat.Version) {
	jsonData, err := readFile(versionToBigQueryJSONPath[ver], file, jsonExt)
	if err != nil {
		t.Fatalf("Failed to read resource json file %s: %v", file, err)
	}
	protoData, err := readFile(versionToProtoPath[ver], file, txtprotoExt)
	if err != nil {
		t.Fatalf("Failed to read resource text proto file %s: %v", file, err)
	}

	if err := proto.UnmarshalText(string(protoData), res); err != nil {
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
	tests := readTestCases(t, jsonformat.STU3)
	for _, tc := range tests {
		tc := tc
		t.Run(tc.name, func(t *testing.T) {
			t.Parallel()
			if stu3Ignores.Contains(tc.name) {
				t.Skipf("Skipping %s", tc.name)
			}
			testUnmarshal(t, tc, jsonformat.STU3)
		})
	}
}

func TestUnmarshal_R4(t *testing.T) {
	t.Parallel()
	tests := readTestCases(t, jsonformat.R4)
	for _, tc := range tests {
		tc := tc
		t.Run(tc.name, func(t *testing.T) {
			t.Parallel()
			if r4Ignores.Contains(tc.name) {
				t.Skipf("Skipping %s", tc.name)
			}
			testUnmarshal(t, tc, jsonformat.R4)
		})
	}
}

func testUnmarshal(t *testing.T, tc *testCase, ver jsonformat.Version) {
	um, err := jsonformat.NewUnmarshaller(timeZone, ver)
	if err != nil {
		t.Fatalf("Failed to create unmarshaller: %v", err)
	}
	unmarshalled, err := um.Unmarshal(tc.jsonData)
	if err != nil {
		t.Fatalf("Failed to unmarshal resource JSON: %v", err)
	}
	res, err := unwrapFromContainedResource(unmarshalled)
	if err != nil {
		t.Fatalf("Failed to unwrap resource: %v", err)
	}

	want, err := getZeroResource(tc.jsonData, ver)
	if err != nil {
		t.Fatalf("Failed to get zero resource from JSON %v", err)
	}
	if err = proto.UnmarshalText(string(tc.protoData), want); err != nil {
		t.Fatalf("Failed to unmarshal resource text proto: %v", err)
	}
	if diff := cmp.Diff(want, res, protocmp.Transform()); diff != "" {
		t.Errorf("Unexpected diff between want and unmarshalled %v: (-want, +got) %v", tc.name, diff)
	}
}

func TestMarshalUnmarshalIdentity_STU3(t *testing.T) {
	t.Parallel()
	tests := readTestCaseFileNames(t, versionToJSONPath[jsonformat.STU3])
	for _, tc := range tests {
		tc := tc
		t.Run(tc, func(t *testing.T) {
			t.Parallel()
			if stu3Ignores.Contains(tc) {
				t.Skipf("Skipping %s", tc)
			}
			testMarshalUnmarshalIdentity(t, tc, jsonformat.STU3)
		})
	}
}

func TestMarshalUnmarshalIdentity_R4(t *testing.T) {
	t.Parallel()
	tests := readTestCaseFileNames(t, versionToJSONPath[jsonformat.R4])
	for _, tc := range tests {
		tc := tc
		t.Run(tc, func(t *testing.T) {
			t.Parallel()
			if r4Ignores.Contains(tc) {
				t.Skipf("Skipping %s", tc)
			}
			testMarshalUnmarshalIdentity(t, tc, jsonformat.R4)
		})
	}
}

// testMarshalUnmarshalIdentity tests that the unmarshalling and marshalling a
// JSON resource results in equal JSON to the original.
func testMarshalUnmarshalIdentity(t *testing.T, fileName string, ver jsonformat.Version) {
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
func getZeroResource(jsonData []byte, ver jsonformat.Version) (proto.Message, error) {
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

	rcr := proto.MessageReflect(cr)
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
	res, err := protopath.Get(proto.MessageV2(containedRes), protopath.NewPath(oneofResourceProtopath), nil)
	if err != nil {
		return nil, err
	}
	return res.(proto.Message), nil
}

// wrapInContainedResource wraps a resource within a contained resource.
func wrapInContainedResource(res proto.Message, ver jsonformat.Version) (proto.Message, error) {
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
	if err := protopath.Set(proto.MessageV2(contained), resPath, res); err != nil {
		return nil, err
	}

	return contained, nil
}

func newContainedResource(ver jsonformat.Version) (proto.Message, error) {
	switch ver {
	case jsonformat.STU3:
		return &r3pb.ContainedResource{}, nil
	case jsonformat.R4:
		return &r4pb.ContainedResource{}, nil
	default:
		panic(fmt.Sprintf("Invalid version specified %v", ver))
	}
}

type testCase struct {
	name                string
	jsonData, protoData []byte
}

// readTestCases reads test files and builds all test cases for a given version.
func readTestCases(t *testing.T, ver jsonformat.Version) []*testCase {
	t.Helper()
	fileNames := readTestCaseFileNames(t, versionToJSONPath[ver])

	// Read JSON and text proto file for each file name and create a test case if both exist.
	var tcs []*testCase
	for _, name := range fileNames {
		jsonData, err := readFile(versionToJSONPath[ver], name, jsonExt)
		if err != nil {
			t.Fatalf("Failed to read resource json file %s: %v", name, err)
		}

		protoData, err := readFile(versionToProtoPath[ver], name, txtprotoExt)
		if err != nil {
			if os.IsNotExist(err) {
				// Silently skip test case if there is no corresponding text proto to reduce noise.
				continue
			}
			t.Fatalf("Failed to read resource text proto file %s: %v", name, err)
		}

		tcs = append(tcs, &testCase{
			name:      name,
			jsonData:  jsonData,
			protoData: protoData,
		})
	}

	return tcs
}

func readTestCaseFileNames(t *testing.T, basePath string) []string {
	t.Helper()
	rp, err := bazel.RunfilesPath()
	if err != nil {
		t.Fatalf("Failed to get runfiles tree path: %v", err)
	}
	files, err := filepath.Glob(path.Join(rp, fmt.Sprintf("%s*.json", basePath)))
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
	root, err := bazel.RunfilesPath()
	if err != nil {
		return nil, err
	}
	filePath := path.Join(root, basePath, fmt.Sprintf("%s.%s", fileName, ext))
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
