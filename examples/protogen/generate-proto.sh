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

ROOT_PATH=../..
INPUT_PATH=$ROOT_PATH/testdata/stu3/structure_definitions
EXTENSION_PATH=$ROOT_PATH/testdata/stu3/extensions
PROTO_GENERATOR=$ROOT_PATH/bazel-bin/java/ProtoGenerator
OUTPUT_PATH=.
MANUAL_ADDITIONS_ROOT=.

while getopts ":io:" opt; do
  case ${opt} in
    i )
      INPUT_PATH=$OPTARG
      ;;
    o )
      OUTPUT_PATH=$OPTARG
      ;;
    \? )
      echo "Invalid option: $OPTARG" 1>&2
      ;;
    : )
      echo "Invalid option: $OPTARG requires an argument" 1>&2
      ;;
  esac
done
shift $((OPTIND -1))

# Build the binary.
bazel build //java:ProtoGenerator

PRIMITIVES="Base64Binary Boolean Code Date DateTime Decimal Id Instant Integer Markdown Oid PositiveInt String Time UnsignedInt Uri Uuid Xhtml"
DATATYPES="Address Age Annotation Attachment CodeableConcept Coding ContactPoint Count Distance Dosage Duration HumanName Identifier Meta Money Period Quantity Range Ratio SampledData Signature SimpleQuantity Timing"
METADATATYPES="BackboneElement ContactDetail Contributor DataRequirement Element ElementDefinition Narrative ParameterDefinition RelatedArtifact Resource TriggerDefinition UsageContext"
RESOURCETYPES="Account ActivityDefinition AdverseEvent AllergyIntolerance Appointment AppointmentResponse AuditEvent Basic Binary BodySite Bundle CapabilityStatement CarePlan CareTeam ChargeItem Claim ClaimResponse ClinicalImpression CodeSystem Communication CommunicationRequest CompartmentDefinition Composition ConceptMap Condition Consent Contract Coverage DataElement DetectedIssue Device DeviceComponent DeviceMetric DeviceRequest DeviceUseStatement DiagnosticReport DocumentManifest DocumentReference EligibilityRequest EligibilityResponse Encounter Endpoint EnrollmentRequest EnrollmentResponse EpisodeOfCare ExpansionProfile ExplanationOfBenefit FamilyMemberHistory Flag Goal GraphDefinition Group GuidanceResponse HealthcareService ImagingManifest ImagingStudy Immunization ImmunizationRecommendation ImplementationGuide Library Linkage List Location Measure MeasureReport Media Medication MedicationAdministration MedicationDispense MedicationRequest MedicationStatement MessageDefinition MessageHeader NamingSystem NutritionOrder Observation OperationDefinition OperationOutcome Organization Parameters Patient PaymentNotice PaymentReconciliation Person PlanDefinition Practitioner PractitionerRole Procedure ProcedureRequest ProcessRequest ProcessResponse Provenance Questionnaire QuestionnaireResponse ReferralRequest RelatedPerson RequestGroup ResearchStudy ResearchSubject RiskAssessment Schedule SearchParameter Sequence ServiceDefinition Slot Specimen StructureDefinition StructureMap Subscription Substance SupplyDelivery SupplyRequest Task TestReport TestScript ValueSet VisionPrescription"
PROFILES="observation-genetics ../extensions/extension-observation-geneticsdnasequencevariantname"
EXTENSIONS="extension-elementdefinition-bindingname extension-structuredefinition-explicit-type-name extension-structuredefinition-regex"

# generate datatypes.proto
$PROTO_GENERATOR \
  --emit_proto --output_directory $OUTPUT_PATH \
  $(for i in $DATATYPES; do echo " --known_types $INPUT_PATH/${i,,}.profile.json "; done) \
  --output_filename datatypes.proto \
  $(for i in $PRIMITIVES $DATATYPES; do echo "$INPUT_PATH/${i,,}.profile.json"; done)
# Some datatypes are manually generated.
# These include:
# * FHIR-defined valueset codes
# * Proto for Reference, which allows more structure than FHIR spec provides.
# * Extension, which has a field order discrepancy between spec and test data.
# TODO(nickgeorge): generate Extension proto with custom ordering.
# TODO(sundberg): generate codes.proto
echo -e "\n//End of auto-generated messages.\n" >> $OUTPUT_PATH/datatypes.proto
cat $MANUAL_ADDITIONS_ROOT/extension_proto.txt >> $OUTPUT_PATH/datatypes.proto
cat $MANUAL_ADDITIONS_ROOT/reference_proto.txt >> $OUTPUT_PATH/datatypes.proto
cat $MANUAL_ADDITIONS_ROOT/codes_proto.txt >> $OUTPUT_PATH/datatypes.proto

# generate metadatatypes.proto
$PROTO_GENERATOR \
  --emit_proto --output_directory $OUTPUT_PATH \
  --output_filename metadatatypes.proto \
  $(for i in $METADATATYPES; do echo "$INPUT_PATH/${i,,}.profile.json"; done)

# generate resources.proto
$PROTO_GENERATOR \
  --emit_proto --include_contained_resource \
  --include_metadatatypes \
  $(for i in $EXTENSIONS; do echo " --known_types $EXTENSION_PATH/${i}.json "; done) \
  $(for i in $DATATYPES; do echo " --known_types $INPUT_PATH/${i,,}.profile.json "; done) \
  $(for i in $PROFILES; do echo " --known_types $INPUT_PATH/${i}.profile.json "; done) \
  --output_directory $OUTPUT_PATH --output_filename resources.proto \
  $(for i in $RESOURCETYPES; do echo "$INPUT_PATH/${i,,}.profile.json"; done)

# generate profiles.proto
$PROTO_GENERATOR \
  --emit_proto --include_resources \
  --include_metadatatypes \
  $(for i in $EXTENSIONS; do echo " --known_types $EXTENSION_PATH/${i}.json "; done) \
  $(for i in $DATATYPES; do echo " --known_types $INPUT_PATH/${i,,}.profile.json "; done) \
  $(for i in $PROFILES; do echo " --known_types $INPUT_PATH/${i}.profile.json "; done) \
  --output_directory $OUTPUT_PATH --output_filename profiles.proto \
  $(for i in $PROFILES; do echo "$INPUT_PATH/${i,,}.profile.json"; done)

# generate extensions
# TODO(nickgeorge): have smarter importing logic for profiles so that all
# extensions don't have to get lumped into a single file.
$PROTO_GENERATOR \
  --emit_proto --output_directory $OUTPUT_PATH \
  $(for i in $EXTENSIONS; do echo " --known_types $EXTENSION_PATH/${i}.json "; done) \
  $(for i in $DATATYPES; do echo " --known_types $INPUT_PATH/${i,,}.profile.json "; done) \
  $(for i in $PROFILES; do echo " --known_types $INPUT_PATH/${i}.profile.json "; done) \
  --output_filename extensions.proto \
  $(for i in $EXTENSIONS; do echo "$EXTENSION_PATH/${i,,}.json"; done)
