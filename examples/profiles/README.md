# Example code for generating profiles, and converting data into that profile.

This directory contains example code for creating a custom profile, as well as a c++ script that converts existing data into that profile.  The profiled data can then be uploaded to BigQuery.

This should be done in between steps 2 & 3 in the [BigQuery examples](https://github.com/google/fhir/tree/master/examples/bigquery)

To run, assuming MY_DIR has the directory used in [BigQuery examples](https://github.com/google/fhir/tree/master/examples/bigquery):

```
# Generate .proto files
./generate_definitions_and_protos.sh //examples/profiles:demo

# Convert Patient to DemoPatient (modify if you change name from DemoPatient)
bazel run //examples/profiles:LocalProfiler $MY_DIR

# Generate Schema
bazel run //java:BigQuerySchemaGenerator $MY_DIR

# Upload DemoPatient
bq load --source_format=NEWLINE_DELIMITED_JSON --schema=$MY_DIR/DemoPatient.schema.json synthea.DemoPatient $MY_DIR/DemoPatient.ndjson

# Run a query
bq query --nouse_legacy_sql "\
  SELECT \
    birthPlace.city, \
    APPROX_TOP_COUNT(SUBSTR(mothersMaidenName, 0, 3), 2), \
    count(*) \
  FROM \
    synthea.DemoPatient p \
  GROUP BY 1 \
  ORDER BY 3 DESC \
"
```
