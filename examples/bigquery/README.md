# Example code to upload FHIR resources into BigQuery

This directory contains example code to upload FHIR data into BigQuery.

We use synthetic data from [Synthea](https://syntheticmass.mitre.org/) which is parsed into protocol buffers. We split the FHIR bundles into ndjson per resource type and upload them into BigQuery, and run a few example queries.

Note: do not download [this set of Synthea patients](https://syntheticmass.mitre.org/downloads/2017_11_06/synthea_sample_data_fhir_stu3_nov2017.zip) since the Claim resource in that release is not valid STU3.

Before you can upload to BigQuery, you need to install the [Cloud SDK](https://cloud.google.com/bigquery/quickstart-command-line) and initialize a project.

To run:

```
./01-get-synthea.sh
./02-parse-into-protobuf.sh
./03-upload-to-bq.sh
./04-run-queries.sh
```
