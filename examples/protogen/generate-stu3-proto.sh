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

ROOT_PATH=$(dirname $0)/../..
INPUT_PATH=$ROOT_PATH/spec/hl7.fhir.core/3.0.1/package/
PROTO_GENERATOR=$ROOT_PATH/bazel-bin/java/ProtoGenerator

OUTPUT_PATH="$(dirname $0)/../../proto/stu3/"
DESCRIPTOR_OUTPUT_PATH="$(dirname $0)/../../testdata/stu3/descriptors/"

PRIMITIVES="base64Binary boolean code date dateTime decimal id instant integer markdown oid positiveInt string time unsignedInt uri uuid xhtml"
DATATYPES="Address Age Annotation Attachment CodeableConcept Coding ContactPoint Count Distance Dosage Duration HumanName Identifier Meta Money Period Quantity Range Ratio SampledData Signature SimpleQuantity Timing"
METADATATYPES="BackboneElement ContactDetail Contributor DataRequirement Element ElementDefinition Narrative ParameterDefinition RelatedArtifact TriggerDefinition UsageContext"
RESOURCETYPES="Account ActivityDefinition AdverseEvent AllergyIntolerance Appointment AppointmentResponse AuditEvent Basic Binary BodySite Bundle CapabilityStatement CarePlan CareTeam ChargeItem Claim ClaimResponse ClinicalImpression CodeSystem Communication CommunicationRequest CompartmentDefinition Composition ConceptMap Condition Consent Contract Coverage DataElement DetectedIssue Device DeviceComponent DeviceMetric DeviceRequest DeviceUseStatement DiagnosticReport DocumentManifest DocumentReference EligibilityRequest EligibilityResponse Encounter Endpoint EnrollmentRequest EnrollmentResponse EpisodeOfCare ExpansionProfile ExplanationOfBenefit FamilyMemberHistory Flag Goal GraphDefinition Group GuidanceResponse HealthcareService ImagingManifest ImagingStudy Immunization ImmunizationRecommendation ImplementationGuide Library Linkage List Location Measure MeasureReport Media Medication MedicationAdministration MedicationDispense MedicationRequest MedicationStatement MessageDefinition MessageHeader NamingSystem NutritionOrder Observation OperationDefinition OperationOutcome Organization Parameters Patient PaymentNotice PaymentReconciliation Person PlanDefinition Practitioner PractitionerRole Procedure ProcedureRequest ProcessRequest ProcessResponse Provenance Questionnaire QuestionnaireResponse ReferralRequest RelatedPerson RequestGroup ResearchStudy ResearchSubject RiskAssessment Schedule SearchParameter Sequence ServiceDefinition Slot Specimen StructureDefinition StructureMap Subscription Substance SupplyDelivery SupplyRequest Task TestReport TestScript ValueSet VisionPrescription Resource DomainResource"
# NOTE: ConsentDirective dropped from profiles due to
# https://gforge.hl7.org/gf/project/fhir/tracker/?action=TrackerItemEdit&tracker_item_id=19263
PROFILES="bmi bodyheight bodylength bodytemp bodyweight bp cholesterol clinicaldocument devicemetricobservation diagnosticreport-genetics elementdefinition-de familymemberhistory-genetic hdlcholesterol headcircum heartrate hlaresult ldlcholesterol lipidprofile MetadataResource observation-genetics oxygensat procedurerequest-genetics resprate shareablecodesystem shareablevalueset triglyceride vitalsigns vitalspanel"
EXTENSIONS="11179-de-administrative-status 11179-de-change-description 11179-de-classification-or-context 11179-de-contact-address 11179-de-document-reference 11179-de-effective-period 11179-de-is-data-element-concept 11179-de-registry-org 11179-de-submitter-org 11179-objectClass 11179-objectClassProperty 11179-permitted-value-conceptmap 11179-permitted-value-valueset allergyintolerance-certainty allergyintolerance-duration allergyintolerance-reasonRefuted allergyintolerance-resolutionAge allergyintolerance-substanceExposureRisk auditevent-Accession auditevent-Anonymized auditevent-Encrypted auditevent-Instance auditevent-MPPS auditevent-NumberOfInstances auditevent-ParticipantObjectContainsStudy auditevent-SOPClass birthPlace body-site-instance capabilitystatement-expectation capabilitystatement-prohibited capabilitystatement-search-parameter-combination capabilitystatement-supported-system capabilitystatement-websocket careplan-activity-title codesystem-author codesystem-comment codesystem-comments codesystem-conceptOrder codesystem-deprecated codesystem-effectiveDate codesystem-expirationDate codesystem-history codesystem-keyWord codesystem-label codesystem-map codesystem-ordinalValue codesystem-otherName codesystem-reference codesystem-replacedby codesystem-sourceReference codesystem-subsumes codesystem-trusted-expansion codesystem-usage codesystem-warning codesystem-workflowStatus coding-sctdescid communication-media communication-reasonNotPerformed communicationrequest-definition communicationrequest-orderedBy communicationrequest-reasonRejected communicationrequest-relevantHistory communicationrequest-supportingInfo composition-clindoc-otherConfidentiality concept-bidirectional condition-basedOn condition-criticality condition-definition condition-dueTo condition-occurredFollowing condition-outcome condition-partOf condition-ruledOut condition-targetBodySite consent-location consent-NotificationEndpoint consent-Witness cqif-calculatedValue cqif-citation cqif-condition cqif-cqlExpression cqif-fhirPathExpression cqif-guidanceencounterClass cqif-guidanceencounterType cqif-guidanceinitiatingOrganization cqif-guidanceinitiatingPerson cqif-guidancereceivingOrganization cqif-guidancereceivingPerson cqif-guidancerecipientLanguage cqif-guidancerecipientType cqif-guidancesystemUserLanguage cqif-guidancesystemUserTaskContext cqif-guidancesystemUserType cqif-initialValue cqif-library cqif-measureInfo cqif-optionCode cqif-qualityOfEvidence cqif-sourceValueSet cqif-strengthOfRecommendation data-absent-reason datadictionary device-din device-implant-status devicerequest-patientInstruction devicerequest-reasonRejected diagnosticReport-addendumOf diagnosticReport-extends DiagnosticReport-geneticsAnalysis  DiagnosticReport-geneticsAssessedCondition DiagnosticReport-geneticsFamilyMemberHistory diagnosticReport-locationPerformed diagnosticReport-replaces diagnosticReport-summaryOf elementdefinition-allowedUnits elementdefinition-bestpractice elementdefinition-bindingName elementdefinition-equivalence elementdefinition-identifier elementdefinition-inheritedExtensibleValueSet elementdefinition-isCommonBinding elementdefinition-maxValueSet elementdefinition-minValueSet elementdefinition-namespace elementdefinition-question elementdefinition-selector elementdefinition-translatable encounter-associatedEncounter encounter-modeOfArrival encounter-primaryDiagnosis encounter-reasonCancelled entryFormat event-definition event-notDone event-OnBehalfOf event-partOf event-performerRole event-reasonCode event-reasonReference family-member-history-genetics-observation family-member-history-genetics-parent family-member-history-genetics-sibling familymemberhistory-abatement familymemberhistory-patient-record familymemberhistory-severity familymemberhistory-type flag-detail flag-priority geolocation goal-acceptance goal-pertainsToGoal goal-reasonRejected goal-relationship hla-genotyping-results-allele-database hla-genotyping-results-glstring hla-genotyping-results-haploid hla-genotyping-results-method http-response-header humanname-assembly-order humanname-fathers-family humanname-mothers-family humanname-own-name humanname-own-prefix humanname-partner-name humanname-partner-prefix identifier-validDate implementationguide-page iso21090-AD-use iso21090-ADXP-additionalLocator iso21090-ADXP-buildingNumberSuffix iso21090-ADXP-careOf iso21090-ADXP-censusTract iso21090-ADXP-delimiter iso21090-ADXP-deliveryAddressLine iso21090-ADXP-deliveryInstallationArea iso21090-ADXP-deliveryInstallationQualifier iso21090-ADXP-deliveryInstallationType iso21090-ADXP-deliveryMode iso21090-ADXP-deliveryModeIdentifier iso21090-ADXP-direction iso21090-ADXP-houseNumber iso21090-ADXP-houseNumberNumeric iso21090-ADXP-postBox iso21090-ADXP-precinct iso21090-ADXP-streetAddressLine iso21090-ADXP-streetName iso21090-ADXP-streetNameBase iso21090-ADXP-streetNameType iso21090-ADXP-unitID iso21090-ADXP-unitType iso21090-CO-value iso21090-EN-qualifier iso21090-EN-representation iso21090-nullFlavor iso21090-preferred iso21090-SC-coding iso21090-TEL-address iso21090-uncertainty iso21090-uncertaintyType iso21090-verification location-alias location-distance mapSourcePublisher match-grade maxDecimalPlaces maxSize maxValue medication-usualRoute medicationdispense-validityPeriod medicationstatement-Prescriber messageheader-response-request mimeType minLength minValue observation-bodyPosition observation-delta observation-eventTiming observation-focal-subject observation-geneticsAlleleName observation-geneticsAllelicFrequency observation-geneticsAllelicState observation-geneticsAminoAcidChangeName observation-geneticsAminoAcidChangeType observation-geneticsCopyNumberEvent observation-geneticsDNARegionName observation-geneticsDNASequenceVariantName observation-geneticsDNASequenceVariantType observation-geneticsDNAVariantId observation-geneticsGene observation-geneticsGenomicSourceClass observation-geneticsInterpretation observation-geneticsPhaseSet observation-geneticsSequence observation-time-offset openEHR-administration openEHR-careplan openEHR-exposureDate openEHR-exposureDescription openEHR-exposureDuration openEHR-location openEHR-management openEHR-test operationoutcome-authority operationoutcome-detectedIssue operationoutcome-issue-source organization-alias organization-period organization-preferredContact patient-adoptionInfo patient-birthTime patient-cadavericDonor patient-citizenship patient-clinicalTrial patient-congregation patient-disability patient-importance patient-interpreterRequired patient-mothersMaidenName patient-nationality patient-religion pharmacy-core-doseType pharmacy-core-infuseOver pharmacy-core-maxDeliveryRate pharmacy-core-maxDeliveryVolume pharmacy-core-minDosePerPeriod pharmacy-core-rateGoal pharmacy-core-rateIncrement pharmacy-core-rateIncrementInterval pharmacy-core-refillsRemaining practitioner-animalSpecies practitioner-classification practitionerrole-primaryInd procedure-approachBodySite procedure-causedBy procedure-incisionDateTime procedure-method procedure-progressStatus procedure-schedule procedure-targetBodySite procedurerequest-approachBodySite procedurerequest-authorizedBy procedurerequest-geneticsItem procedurerequest-precondition procedurerequest-questionnaireRequest procedurerequest-reasonRefused procedurerequest-reasonRejected procedurerequest-targetBodySite questionnaire-allowedProfile questionnaire-allowedResource questionnaire-baseType questionnaire-choiceOrientation questionnaire-deMap questionnaire-displayCategory questionnaire-fhirType questionnaire-hidden questionnaire-itemControl questionnaire-lookupQuestionnaire questionnaire-maxOccurs questionnaire-minOccurs questionnaire-optionExclusive questionnaire-optionPrefix questionnaire-ordinalValue questionnaire-referenceFilter questionnaire-sourceStructureMap questionnaire-studyprotocolIdentifier questionnaire-supportLink questionnaire-targetStructureMap questionnaire-unit questionnaire-usageMode questionnaireresponse-author questionnaireresponse-note questionnaireresponse-reason questionnaireresponse-reviewer referralrequest-reasonRefused regex rendered-value rendering-markdown rendering-style rendering-styleSensitive rendering-xhtml resource-approvalDate resource-effectivePeriod resource-lastReviewDate specimen-collectionPriority specimen-isDryWeight specimen-processingTime specimen-sequenceNumber specimen-specialHandling structuredefinition-ancestor structuredefinition-annotation structuredefinition-ballot-status structuredefinition-category structuredefinition-display-hint structuredefinition-explicit-type-name structuredefinition-fmm-no-warnings structuredefinition-fmm structuredefinition-json-type structuredefinition-rdf-type structuredefinition-regex structuredefinition-summary structuredefinition-table-name structuredefinition-template-status structuredefinition-wg structuredefinition-xml-type task-candidateList task-replaces timing-daysOfCycle timing-exact translation usagecontext-group valueset-author valueset-caseSensitive valueset-comment valueset-comments valueset-conceptOrder valueset-definition valueset-effectiveDate valueset-expansionSource valueset-expirationDate valueset-history valueset-keyWord valueset-label valueset-map valueset-ordinalValue valueset-otherName valueset-reference valueset-sourceReference valueset-system valueset-systemName valueset-systemRef valueset-toocostly valueset-trusted-expansion valueset-unclosed valueset-usage valueset-warning valueset-workflowStatus"

MODIFIED_TYPES="StructureDefinition cqif-cqlExpression cqif-fhirPathExpression cqif-condition cqif-library familymemberhistory-genetic"

function get_location() {
  if [[ " ${MODIFIED_TYPES[@]} " =~ " $1 " ]]; then
    echo "$INPUT_PATH/../modified/StructureDefinition-${i}.json"
    return
  fi
  echo "$INPUT_PATH/StructureDefinition-$1.json"
}

FHIR_STRUCT_DEF_ZIP="$ROOT_PATH/bazel-genfiles/spec/fhir_stu3_structure_definitions.zip"

COMMON_FLAGS=" \
  --emit_proto \
  --emit_descriptors \
  --package_info $FHIR_PACKAGE_INFO \
  --r4_core_dep $FHIR_DEFINITION_ZIP \
  --output_directory $OUTPUT_PATH \
  --descriptor_output_directory $DESCRIPTOR_OUTPUT_PATH" \

# Build the binary.
bazel build //java:ProtoGenerator

if [ $? -ne 0 ]
then
 echo "Build Failed"
 exit 1;
fi

# generate datatypes.proto
$PROTO_GENERATOR \
  $COMMON_FLAGS \
  --output_name datatypes \
  $(for i in $PRIMITIVES $DATATYPES; do echo "$(get_location $i)"; done)
# Some datatypes are manually generated.
# These include:
# * FHIR-defined valueset codes
# * Proto for Reference, which allows more structure than FHIR spec provides.
# * Extension, which has a field order discrepancy between spec and test data.
# TODO: generate Extension proto with custom ordering.
# TODO: generate codes.proto
if [ $? -eq 0 ]
then
  echo -e "\n//End of auto-generated messages.\n" >> $OUTPUT_PATH/datatypes.proto
  cat "$(dirname $0)/stu3/datatypes_supplement_proto.txt" >> $OUTPUT_PATH/datatypes.proto
fi

# generate metadatatypes.proto
$PROTO_GENERATOR \
  $COMMON_FLAGS \
  --output_name metadatatypes \
  $(for i in $METADATATYPES; do echo "$(get_location $i)"; done)

# generate resources.proto
$PROTO_GENERATOR \
  $COMMON_FLAGS \
  --include_contained_resource \
  --output_name resources \
  $(for i in $RESOURCETYPES; do echo "$(get_location $i)"; done)

# generate profiles.proto
$PROTO_GENERATOR \
  $COMMON_FLAGS \
  --output_name profiles \
  $(for i in $PROFILES; do echo "$(get_location $i)"; done)

# generate extensions
$PROTO_GENERATOR \
  $COMMON_FLAGS \
  --output_name extensions \
  $(for i in $EXTENSIONS; do echo "$(get_location $i)"; done)
