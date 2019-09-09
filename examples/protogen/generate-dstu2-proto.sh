#!/bin/bash
# Copyright 2018 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Build the binary.
bazel build //java:ProtoGenerator
bazel build //spec:fhir_dstu2_structure_definitions.zip

ROOT_PATH="$(dirname $0)../.."
INPUT_PATH=$ROOT_PATH/spec/hl7.fhir.core/1.0.2/package/
PROTO_GENERATOR=$ROOT_PATH/bazel-bin/java/ProtoGenerator
PROTO_GENERATOR=$ROOT_PATH/bazel-bin/java/StructureDefinitionTransformer
DESCRIPTOR_OUTPUT_PATH=$ROOT_PATH/testdata/dstu2/descriptors/

# TODO: Consider using the Bundle-types and Bundle-resources to avoid explicitly listing them.
PRIMITIVES="base64Binary boolean code date dateTime decimal id instant integer markdown oid positiveInt string time unsignedInt uri uuid xhtml"
DATATYPES="Address Age Annotation Attachment CodeableConcept Coding ContactPoint Count Distance Duration Extension HumanName Identifier Meta Money Period Quantity Range Ratio SampledData Signature SimpleQuantity Timing"
METADATATYPES="BackboneElement Element ElementDefinition Narrative"
RESOURCES="Account AllergyIntolerance Appointment AppointmentResponse AuditEvent Basic Binary BodySite Bundle CarePlan Claim ClaimResponse ClinicalImpression Communication CommunicationRequest Composition ConceptMap Condition Conformance Contract Coverage DataElement DetectedIssue Device DeviceComponent DeviceMetric DeviceUseRequest DeviceUseStatement DiagnosticOrder DiagnosticReport DocumentManifest DocumentReference DomainResource EligibilityRequest EligibilityResponse Encounter EnrollmentRequest EnrollmentResponse EpisodeOfCare ExplanationOfBenefit FamilyMemberHistory Flag Goal Group HealthcareService ImagingObjectSelection ImagingStudy Immunization ImmunizationRecommendation ImplementationGuide List Location Media Medication MedicationAdministration MedicationDispense MedicationOrder MedicationStatement MessageHeader NamingSystem NutritionOrder Observation OperationDefinition OperationOutcome Order OrderResponse Organization Parameters Patient PaymentNotice PaymentReconciliation Person Practitioner Procedure ProcedureRequest ProcessRequest ProcessResponse Provenance Questionnaire QuestionnaireResponse ReferralRequest RelatedPerson Resource RiskAssessment Schedule SearchParameter Slot Specimen StructureDefinition Subscription Substance SupplyDelivery SupplyRequest TestScript ValueSet VisionPrescription"

FHIR_PACKAGE_INFO=$ROOT_PATH/testdata/dstu2/fhir_package_info.prototxt
OUTPUT_PATH=$ROOT_PATH/proto/dstu2

INPUT_PATH=$ROOT_PATH/spec/hl7.fhir.core/1.0.2/package

COMMON_FLAGS="\
  --emit_proto \
  --emit_descriptors \
  --dstu2_struct_def_zip $FHIR_STRUCT_DEF_ZIP \
  --package_info $FHIR_PACKAGE_INFO \
  --output_directory $OUTPUT_PATH \
  --descriptor_output_directory $DESCRIPTOR_OUTPUT_PATH"

# Generate datatypes.proto.
$PROTO_GENERATOR \
  $COMMON_FLAGS \
  --output_name datatypes \
  $(for i in $PRIMITIVES $DATATYPES; do echo "$INPUT_PATH/StructureDefinition-${i}.json "; done)

if [ $? -eq 0 ]
then
  echo -e "\n//End of auto-generated messages.\n" >> $OUTPUT_PATH/datatypes.proto
  # Add manually generated messages.
  cat $(dirname $0)/dstu2/reference_proto.txt >> $OUTPUT_PATH/datatypes.proto
  cat $(dirname $0)/dstu2/codes_proto.txt >> $OUTPUT_PATH/datatypes.proto
fi

# Generate metadatatypes.proto.
$PROTO_GENERATOR \
  $COMMON_FLAGS \
  --output_name metadatatypes \
  $(for i in $METADATATYPES; do echo "$INPUT_PATH/StructureDefinition-${i}.json "; done)

# Generate resources.proto.
$PROTO_GENERATOR \
  $COMMON_FLAGS \
  --output_name resources \
  $(for i in $RESOURCES; do echo "$INPUT_PATH/StructureDefinition-${i}.json "; done)
