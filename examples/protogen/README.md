# Example Code for Generating STU3 FHIR Protos and Test Data.

This directory contains example code for generating Proto definitions and test data based on FHIR STU3 StructureDefinitions defined by https://www.hl7.org/fhir/.

They use StructureDefinitions defined in `fhir/testdata/stu3/structure_definitions/`, which were obtained from https://www.hl7.org/fhir/fhir.schema.json.zip.

All of these scripts should be run out of this directory, have default input directories in `fhir/testdata/stu3/`, and use `./` as default output directory.

To generate STU3 FHIR Proto definition (`.proto`) files, run:

```
./generate-proto.sh [-i input-dir] [-o output-dir]
```

To generate DescriptorProtos as `.prototxt` files, run:

```
./generate-descriptors.sh [-i input-dir] [-o output-dir]
```

A third script uses `com.google.fhir.examples.JsonToProtoMain` to generate `.prototxt` files from FHIR JSON example data.  Defaults to use examples in `fhir/testdata/stu3/examples/` and available at https://www.hl7.org/fhir/examples-json.zip:

```
./generate-testdata.sh [-i input-dir] [-o output-dir]
```
