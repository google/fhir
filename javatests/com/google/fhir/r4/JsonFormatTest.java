//    Copyright 2018 Google Inc.
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//        https://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.

package com.google.fhir.r4;

import com.google.common.io.Files;
import com.google.fhir.common.InvalidFhirException;
import com.google.fhir.r4.core.Account;
import com.google.fhir.r4.core.ActivityDefinition;
import com.google.fhir.r4.core.AdverseEvent;
import com.google.fhir.r4.core.AllergyIntolerance;
import com.google.fhir.r4.core.Appointment;
import com.google.fhir.r4.core.AppointmentResponse;
import com.google.fhir.r4.core.AuditEvent;
import com.google.fhir.r4.core.Basic;
import com.google.fhir.r4.core.Binary;
import com.google.fhir.r4.core.BiologicallyDerivedProduct;
import com.google.fhir.r4.core.BodyStructure;
import com.google.fhir.r4.core.Bundle;
import com.google.fhir.r4.core.CapabilityStatement;
import com.google.fhir.r4.core.CarePlan;
import com.google.fhir.r4.core.CareTeam;
import com.google.fhir.r4.core.CatalogEntry;
import com.google.fhir.r4.core.ChargeItem;
import com.google.fhir.r4.core.ChargeItemDefinition;
import com.google.fhir.r4.core.Claim;
import com.google.fhir.r4.core.ClaimResponse;
import com.google.fhir.r4.core.ClinicalImpression;
import com.google.fhir.r4.core.Communication;
import com.google.fhir.r4.core.CommunicationRequest;
import com.google.fhir.r4.core.CompartmentDefinition;
import com.google.fhir.r4.core.Composition;
import com.google.fhir.r4.core.Condition;
import com.google.fhir.r4.core.Consent;
import com.google.fhir.r4.core.Contract;
import com.google.fhir.r4.core.Coverage;
import com.google.fhir.r4.core.CoverageEligibilityRequest;
import com.google.fhir.r4.core.CoverageEligibilityResponse;
import com.google.fhir.r4.core.DetectedIssue;
import com.google.fhir.r4.core.Device;
import com.google.fhir.r4.core.DeviceDefinition;
import com.google.fhir.r4.core.DeviceMetric;
import com.google.fhir.r4.core.DeviceRequest;
import com.google.fhir.r4.core.DeviceUseStatement;
import com.google.fhir.r4.core.DiagnosticReport;
import com.google.fhir.r4.core.DocumentManifest;
import com.google.fhir.r4.core.DocumentReference;
import com.google.fhir.r4.core.EffectEvidenceSynthesis;
import com.google.fhir.r4.core.Encounter;
import com.google.fhir.r4.core.Endpoint;
import com.google.fhir.r4.core.EnrollmentRequest;
import com.google.fhir.r4.core.EnrollmentResponse;
import com.google.fhir.r4.core.EpisodeOfCare;
import com.google.fhir.r4.core.EventDefinition;
import com.google.fhir.r4.core.Evidence;
import com.google.fhir.r4.core.EvidenceVariable;
import com.google.fhir.r4.core.ExampleScenario;
import com.google.fhir.r4.core.ExplanationOfBenefit;
import com.google.fhir.r4.core.FamilyMemberHistory;
import com.google.fhir.r4.core.Flag;
import com.google.fhir.r4.core.Goal;
import com.google.fhir.r4.core.GraphDefinition;
import com.google.fhir.r4.core.Group;
import com.google.fhir.r4.core.GuidanceResponse;
import com.google.fhir.r4.core.HealthcareService;
import com.google.fhir.r4.core.ImagingStudy;
import com.google.fhir.r4.core.Immunization;
import com.google.fhir.r4.core.ImmunizationEvaluation;
import com.google.fhir.r4.core.ImmunizationRecommendation;
import com.google.fhir.r4.core.ImplementationGuide;
import com.google.fhir.r4.core.InsurancePlan;
import com.google.fhir.r4.core.Invoice;
import com.google.fhir.r4.core.Library;
import com.google.fhir.r4.core.Linkage;
import com.google.fhir.r4.core.List;
import com.google.fhir.r4.core.Location;
import com.google.fhir.r4.core.Measure;
import com.google.fhir.r4.core.MeasureReport;
import com.google.fhir.r4.core.Media;
import com.google.fhir.r4.core.Medication;
import com.google.fhir.r4.core.MedicationAdministration;
import com.google.fhir.r4.core.MedicationDispense;
import com.google.fhir.r4.core.MedicationKnowledge;
import com.google.fhir.r4.core.MedicationRequest;
import com.google.fhir.r4.core.MedicationStatement;
import com.google.fhir.r4.core.MedicinalProduct;
import com.google.fhir.r4.core.MedicinalProductAuthorization;
import com.google.fhir.r4.core.MedicinalProductContraindication;
import com.google.fhir.r4.core.MedicinalProductIndication;
import com.google.fhir.r4.core.MedicinalProductIngredient;
import com.google.fhir.r4.core.MedicinalProductInteraction;
import com.google.fhir.r4.core.MedicinalProductManufactured;
import com.google.fhir.r4.core.MedicinalProductPackaged;
import com.google.fhir.r4.core.MedicinalProductPharmaceutical;
import com.google.fhir.r4.core.MedicinalProductUndesirableEffect;
import com.google.fhir.r4.core.MessageDefinition;
import com.google.fhir.r4.core.MessageHeader;
import com.google.fhir.r4.core.MolecularSequence;
import com.google.fhir.r4.core.NamingSystem;
import com.google.fhir.r4.core.NutritionOrder;
import com.google.fhir.r4.core.Observation;
import com.google.fhir.r4.core.ObservationDefinition;
import com.google.fhir.r4.core.OperationDefinition;
import com.google.fhir.r4.core.OperationOutcome;
import com.google.fhir.r4.core.Organization;
import com.google.fhir.r4.core.OrganizationAffiliation;
import com.google.fhir.r4.core.Patient;
import com.google.fhir.r4.core.PaymentNotice;
import com.google.fhir.r4.core.PaymentReconciliation;
import com.google.fhir.r4.core.Person;
import com.google.fhir.r4.core.PlanDefinition;
import com.google.fhir.r4.core.Practitioner;
import com.google.fhir.r4.core.PractitionerRole;
import com.google.fhir.r4.core.Procedure;
import com.google.fhir.r4.core.Provenance;
import com.google.fhir.r4.core.Questionnaire;
import com.google.fhir.r4.core.QuestionnaireResponse;
import com.google.fhir.r4.core.RelatedPerson;
import com.google.fhir.r4.core.RequestGroup;
import com.google.fhir.r4.core.ResearchDefinition;
import com.google.fhir.r4.core.ResearchElementDefinition;
import com.google.fhir.r4.core.ResearchStudy;
import com.google.fhir.r4.core.ResearchSubject;
import com.google.fhir.r4.core.RiskAssessment;
import com.google.fhir.r4.core.RiskEvidenceSynthesis;
import com.google.fhir.r4.core.Schedule;
import com.google.fhir.r4.core.ServiceRequest;
import com.google.fhir.r4.core.Slot;
import com.google.fhir.r4.core.Specimen;
import com.google.fhir.r4.core.SpecimenDefinition;
import com.google.fhir.r4.core.StructureMap;
import com.google.fhir.r4.core.Subscription;
import com.google.fhir.r4.core.Substance;
import com.google.fhir.r4.core.SubstanceSpecification;
import com.google.fhir.r4.core.SupplyDelivery;
import com.google.fhir.r4.core.SupplyRequest;
import com.google.fhir.r4.core.Task;
import com.google.fhir.r4.core.TerminologyCapabilities;
import com.google.fhir.r4.core.TestReport;
import com.google.fhir.r4.core.TestScript;
import com.google.fhir.r4.core.VerificationResult;
import com.google.fhir.r4.core.VisionPrescription;
import com.google.fhir.testing.JsonFormatTestBase;
import com.google.protobuf.Message;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit tests for {@link JsonFormat}. */
@RunWith(JUnit4.class)
public class JsonFormatTest extends JsonFormatTestBase {

  private static final boolean GENERATE_GOLDEN = false;

  public JsonFormatTest() {
    super("r4", "hl7.fhir.r4.examples/4.0.1");
  }

  @Before
  public void setUp() throws IOException, InvalidFhirException {
    setUpParser();
  }

  private void generateProtoTxt(String[] fileNames, Message.Builder type)
      throws InvalidFhirException, IOException {
    for (String fileName : fileNames) {
      Message.Builder builder = type.clone();
      try {
        parseToProto(fileName, builder);
      } catch (Exception e) {
        throw new InvalidFhirException("Failed parsing " + fileName, e);
      }
      File file = new File("/tmp/examples/" + fileName + ".prototxt");
      Files.asCharSink(file, StandardCharsets.UTF_8).write(builder.toString());
    }
  }

  private void testOrGenerate(String[] fileNames, Message.Builder type)
      throws InvalidFhirException, IOException {
    if (GENERATE_GOLDEN) {
      generateProtoTxt(fileNames, type);
    } else {
      for (String file : fileNames) {
        testPair(file, type);
      }
    }
  }

  /** Test the analytics output format. */
  @Test
  public void convertForAnalytics() throws Exception {
    testConvertForAnalytics("Composition-example", Composition.newBuilder());
    testConvertForAnalytics("Encounter-home", Encounter.newBuilder());
    testConvertForAnalytics("Observation-example-genetics-1", Observation.newBuilder());
    testConvertForAnalytics("Patient-example", Patient.newBuilder());
  }

  @Test
  public void testAccount() throws Exception {
    String[] files = {"Account-ewg", "Account-example"};
    testOrGenerate(files, Account.newBuilder());
  }

  @Test
  public void testActivityDefinition() throws Exception {
    String[] files = {
      "ActivityDefinition-administer-zika-virus-exposure-assessment",
      "ActivityDefinition-blood-tubes-supply",
      "ActivityDefinition-citalopramPrescription",
      "ActivityDefinition-heart-valve-replacement",
      "ActivityDefinition-provide-mosquito-prevention-advice",
      "ActivityDefinition-referralPrimaryCareMentalHealth-initial",
      "ActivityDefinition-referralPrimaryCareMentalHealth",
      "ActivityDefinition-serum-dengue-virus-igm",
      "ActivityDefinition-serum-zika-dengue-virus-igm"
    };
    testOrGenerate(files, ActivityDefinition.newBuilder());
  }

  @Test
  public void testAdverseEvent() throws Exception {
    String[] files = {"AdverseEvent-example"};
    testOrGenerate(files, AdverseEvent.newBuilder());
  }

  @Test
  public void testAllergyIntolerance() throws Exception {
    String[] files = {
      "AllergyIntolerance-example",
      "AllergyIntolerance-fishallergy",
      "AllergyIntolerance-medication",
      "AllergyIntolerance-nka",
      "AllergyIntolerance-nkda",
      "AllergyIntolerance-nkla"
    };
    testOrGenerate(files, AllergyIntolerance.newBuilder());
  }

  @Test
  public void testAppointment() throws Exception {
    String[] files = {"Appointment-2docs", "Appointment-example", "Appointment-examplereq"};
    testOrGenerate(files, Appointment.newBuilder());
  }

  @Test
  public void testAppointmentResponse() throws Exception {
    String[] files = {"AppointmentResponse-example", "AppointmentResponse-exampleresp"};
    testOrGenerate(files, AppointmentResponse.newBuilder());
  }

  @Test
  public void testAuditEvent() throws Exception {
    String[] files = {
      "AuditEvent-example-disclosure",
      "AuditEvent-example-error",
      "AuditEvent-example",
      "AuditEvent-example-login",
      "AuditEvent-example-logout",
      "AuditEvent-example-media",
      "AuditEvent-example-pixQuery",
      "AuditEvent-example-rest",
      "AuditEvent-example-search"
    };
    testOrGenerate(files, AuditEvent.newBuilder());
  }

  @Test
  public void testBasic() throws Exception {
    String[] files = {"Basic-basic-example-narrative", "Basic-classModel", "Basic-referral"};
    testOrGenerate(files, Basic.newBuilder());
  }

  @Test
  public void testBinary() throws Exception {
    String[] files = {"Binary-example", "Binary-f006"};
    testOrGenerate(files, Binary.newBuilder());
  }

  @Test
  public void testBiologicallyDerivedProduct() throws Exception {
    String[] files = {"BiologicallyDerivedProduct-example"};
    testOrGenerate(files, BiologicallyDerivedProduct.newBuilder());
  }

  @Test
  public void testBodyStructure() throws Exception {
    String[] files = {"BodyStructure-fetus", "BodyStructure-skin-patch", "BodyStructure-tumor"};
    testOrGenerate(files, BodyStructure.newBuilder());
  }

  // Split bundles into several tests to enable better sharding.
  @Test
  public void testBundlePt1() throws Exception {
    String[] files = {
      "Bundle-101",
      "Bundle-10bb101f-a121-4264-a920-67be9cb82c74",
      "Bundle-3a0707d3-549e-4467-b8b8-5a2ab3800efe",
      "Bundle-3ad0687e-f477-468c-afd5-fcc2bf897808",
      "Bundle-3ad0687e-f477-468c-afd5-fcc2bf897809",
      "Bundle-3ad0687e-f477-468c-afd5-fcc2bf897819",
      "Bundle-72ac8493-52ac-41bd-8d5d-7258c289b5ea",
      "Bundle-b0a5e4277-83c4-4adb-87e2-e3efe3369b6f",
      "Bundle-b248b1b2-1686-4b94-9936-37d7a5f94b51",
    };
    testOrGenerate(files, Bundle.newBuilder());
  }

  @Test
  public void testBundlePt2() throws Exception {
    String[] files = {
      "Bundle-bundle-example",
      "Bundle-bundle-references",
      "Bundle-bundle-request-medsallergies",
      "Bundle-bundle-request-simplesummary",
      "Bundle-bundle-response",
      "Bundle-bundle-response-medsallergies",
      "Bundle-bundle-response-simplesummary",
      "Bundle-bundle-search-warning",
      "Bundle-bundle-transaction",
    };
    testOrGenerate(files, Bundle.newBuilder());
  }

  @Test
  public void testBundlePt4() throws Exception {
    testOrGenerate(new String[] {"Bundle-conceptmaps", "Bundle-dataelements"}, Bundle.newBuilder());
  }
  //   TODO: These tests don't seem to ever finish for some reason - while other large
  //   files finish in at most a couple seconds, these time out even at 15 minutes.
  //   Seems to be a problem with the printer.  Figure out why, and reneable these tests.

  @Test
  public void testBundlePt3() throws Exception {
    String[] files = {
      "Bundle-dg2",
      "Bundle-extensions",
      "Bundle-externals",
      "Bundle-f001",
      "Bundle-f202",
      "Bundle-father",
      "Bundle-ghp",
      "Bundle-hla-1",
      "Bundle-lipids",
      "Bundle-lri-example",
      "Bundle-micro",
      "Bundle-profiles-others",
      "Bundle-registry",
      "Bundle-report",
      "Bundle-searchParams",
      "Bundle-terminologies",
      "Bundle-types",
      "Bundle-ussg-fht",
      "Bundle-valueset-expansions",
      "Bundle-xds"
    };
    testOrGenerate(files, Bundle.newBuilder());
  }

  @Test
  public void testResourcesBundle() throws Exception {
    String[] files = {
      "Bundle-resources",
    };
    testOrGenerate(files, Bundle.newBuilder());
  }

  @Test
  public void testValuesetsBundle() throws Exception {
    String[] files = {
      "Bundle-valuesets",
    };
    testOrGenerate(files, Bundle.newBuilder());
  }

  @Test
  public void testV2ValuesetsBundle() throws Exception {
    String[] files = {
      "Bundle-v2-valuesets",
    };
    testOrGenerate(files, Bundle.newBuilder());
  }

  @Test
  public void testV3ValuesetsBundle() throws Exception {
    String[] files = {
      "Bundle-v3-valuesets",
    };
    testOrGenerate(files, Bundle.newBuilder());
  }

  @Test
  public void testCapabilityStatement() throws Exception {
    String[] files = {
      "CapabilityStatement-base2",
      "CapabilityStatement-base",
      "CapabilityStatement-example",
      "CapabilityStatement-knowledge-repository",
      "CapabilityStatement-measure-processor",
      "CapabilityStatement-messagedefinition",
      "CapabilityStatement-phr",
      "CapabilityStatement-terminology-server"
    };
    testOrGenerate(files, CapabilityStatement.newBuilder());
  }

  @Test
  public void testCarePlan() throws Exception {
    String[] files = {
      "CarePlan-example",
      "CarePlan-f001",
      "CarePlan-f002",
      "CarePlan-f003",
      "CarePlan-f201",
      "CarePlan-f202",
      "CarePlan-f203",
      "CarePlan-gpvisit",
      "CarePlan-integrate",
      "CarePlan-obesity-narrative",
      "CarePlan-preg"
    };
    testOrGenerate(files, CarePlan.newBuilder());
  }

  @Test
  public void testCareTeam() throws Exception {
    String[] files = {"CareTeam-example"};
    testOrGenerate(files, CareTeam.newBuilder());
  }

  @Test
  public void testCatalogEntry() throws Exception {
    String[] files = {"CatalogEntry-example"};
    testOrGenerate(files, CatalogEntry.newBuilder());
  }

  @Test
  public void testChargeItem() throws Exception {
    String[] files = {"ChargeItem-example"};
    testOrGenerate(files, ChargeItem.newBuilder());
  }

  @Test
  public void testChargeItemDefinition() throws Exception {
    String[] files = {"ChargeItemDefinition-device", "ChargeItemDefinition-ebm"};
    testOrGenerate(files, ChargeItemDefinition.newBuilder());
  }

  @Test
  public void testClaim() throws Exception {
    String[] files = {
      "Claim-100150",
      "Claim-100151",
      "Claim-100152",
      "Claim-100153",
      "Claim-100154",
      "Claim-100155",
      "Claim-100156",
      "Claim-660150",
      "Claim-660151",
      "Claim-660152",
      "Claim-760150",
      "Claim-760151",
      "Claim-760152",
      "Claim-860150",
      "Claim-960150",
      "Claim-960151",
      "Claim-MED-00050"
    };
    testOrGenerate(files, Claim.newBuilder());
  }

  @Test
  public void testClaimResponse() throws Exception {
    String[] files = {
      "ClaimResponse-R3500",
      "ClaimResponse-R3501",
      "ClaimResponse-R3502",
      "ClaimResponse-R3503",
      "ClaimResponse-UR3503"
    };
    testOrGenerate(files, ClaimResponse.newBuilder());
  }

  @Test
  public void testClinicalImpression() throws Exception {
    String[] files = {"ClinicalImpression-example"};
    testOrGenerate(files, ClinicalImpression.newBuilder());
  }

  @Test
  public void testCommunication() throws Exception {
    String[] files = {
      "Communication-example", "Communication-fm-attachment", "Communication-fm-solicited"
    };
    testOrGenerate(files, Communication.newBuilder());
  }

  @Test
  public void testCommunicationRequest() throws Exception {
    String[] files = {"CommunicationRequest-example", "CommunicationRequest-fm-solicit"};
    testOrGenerate(files, CommunicationRequest.newBuilder());
  }

  @Test
  public void testCompartmentDefinition() throws Exception {
    String[] files = {
      "CompartmentDefinition-device",
      "CompartmentDefinition-encounter",
      "CompartmentDefinition-example",
      "CompartmentDefinition-patient",
      "CompartmentDefinition-practitioner",
      "CompartmentDefinition-relatedPerson"
    };
    testOrGenerate(files, CompartmentDefinition.newBuilder());
  }

  @Test
  public void testComposition() throws Exception {
    String[] files = {"Composition-example", "Composition-example-mixed"};
    testOrGenerate(files, Composition.newBuilder());
  }

  @Test
  public void testCondition() throws Exception {
    String[] files = {
      "Condition-example2",
      "Condition-example",
      "Condition-f001",
      "Condition-f002",
      "Condition-f003",
      "Condition-f201",
      "Condition-f202",
      "Condition-f203",
      "Condition-f204",
      "Condition-f205",
      "Condition-family-history",
      "Condition-stroke"
    };
    testOrGenerate(files, Condition.newBuilder());
  }

  @Test
  public void testConsent() throws Exception {
    String[] files = {
      "Consent-consent-example-basic",
      "Consent-consent-example-Emergency",
      "Consent-consent-example-grantor",
      "Consent-consent-example-notAuthor",
      "Consent-consent-example-notOrg",
      "Consent-consent-example-notThem",
      "Consent-consent-example-notThis",
      "Consent-consent-example-notTime",
      "Consent-consent-example-Out",
      "Consent-consent-example-pkb",
      "Consent-consent-example-signature",
      "Consent-consent-example-smartonfhir"
    };
    testOrGenerate(files, Consent.newBuilder());
  }

  @Test
  public void testContract() throws Exception {
    String[] files = {
      "Contract-C-123",
      "Contract-C-2121",
      "Contract-INS-101",
      "Contract-pcd-example-notAuthor",
      "Contract-pcd-example-notLabs",
      "Contract-pcd-example-notOrg",
      "Contract-pcd-example-notThem",
      "Contract-pcd-example-notThis"
    };
    testOrGenerate(files, Contract.newBuilder());
  }

  @Test
  public void testCoverage() throws Exception {
    String[] files = {"Coverage-7546D", "Coverage-7547E", "Coverage-9876B1", "Coverage-SP1234"};
    testOrGenerate(files, Coverage.newBuilder());
  }

  @Test
  public void testCoverageEligibilityRequest() throws Exception {
    String[] files = {"CoverageEligibilityRequest-52345", "CoverageEligibilityRequest-52346"};
    testOrGenerate(files, CoverageEligibilityRequest.newBuilder());
  }

  @Test
  public void testCoverageEligibilityResponse() throws Exception {
    String[] files = {
      "CoverageEligibilityResponse-E2500",
      "CoverageEligibilityResponse-E2501",
      "CoverageEligibilityResponse-E2502",
      "CoverageEligibilityResponse-E2503"
    };
    testOrGenerate(files, CoverageEligibilityResponse.newBuilder());
  }

  @Test
  public void testDetectedIssue() throws Exception {
    String[] files = {
      "DetectedIssue-allergy", "DetectedIssue-ddi", "DetectedIssue-duplicate", "DetectedIssue-lab"
    };
    testOrGenerate(files, DetectedIssue.newBuilder());
  }

  @Test
  public void testDevice() throws Exception {
    String[] files = {"Device-example", "Device-f001"};
    testOrGenerate(files, Device.newBuilder());
  }

  @Test
  public void testDeviceDefinition() throws Exception {
    String[] files = {"DeviceDefinition-example"};
    testOrGenerate(files, DeviceDefinition.newBuilder());
  }

  @Test
  public void testDeviceMetric() throws Exception {
    String[] files = {"DeviceMetric-example"};
    testOrGenerate(files, DeviceMetric.newBuilder());
  }

  @Test
  public void testDeviceRequest() throws Exception {
    String[] files = {
      "DeviceRequest-example",
      "DeviceRequest-insulinpump",
      "DeviceRequest-left-lens",
      "DeviceRequest-right-lens"
    };
    testOrGenerate(files, DeviceRequest.newBuilder());
  }

  @Test
  public void testDeviceUseStatement() throws Exception {
    String[] files = {"DeviceUseStatement-example"};
    testOrGenerate(files, DeviceUseStatement.newBuilder());
  }

  @Test
  public void testDiagnosticReport() throws Exception {
    String[] files = {
      "DiagnosticReport-102",
      "DiagnosticReport-example-pgx",
      "DiagnosticReport-f201",
      "DiagnosticReport-gingival-mass",
      "DiagnosticReport-pap",
      "DiagnosticReport-ultrasound"
    };
    testOrGenerate(files, DiagnosticReport.newBuilder());
  }

  @Test
  public void testDocumentManifest() throws Exception {
    String[] files = {"DocumentManifest-654789", "DocumentManifest-example"};
    testOrGenerate(files, DocumentManifest.newBuilder());
  }

  @Test
  public void testDocumentReference() throws Exception {
    String[] files = {"DocumentReference-example"};
    testOrGenerate(files, DocumentReference.newBuilder());
  }

  @Test
  public void testEffectEvidenceSynthesis() throws Exception {
    String[] files = {"EffectEvidenceSynthesis-example"};
    testOrGenerate(files, EffectEvidenceSynthesis.newBuilder());
  }

  @Test
  public void testEncounter() throws Exception {
    String[] files = {
      "Encounter-emerg",
      "Encounter-example",
      "Encounter-f001",
      "Encounter-f002",
      "Encounter-f003",
      "Encounter-f201",
      "Encounter-f202",
      "Encounter-f203",
      "Encounter-home",
      "Encounter-xcda"
    };
    testOrGenerate(files, Encounter.newBuilder());
  }

  @Test
  public void testEndpoint() throws Exception {
    String[] files = {
      "Endpoint-direct-endpoint",
      "Endpoint-example-iid",
      "Endpoint-example",
      "Endpoint-example-wadors"
    };
    testOrGenerate(files, Endpoint.newBuilder());
  }

  @Test
  public void testEnrollmentRequest() throws Exception {
    String[] files = {"EnrollmentRequest-22345"};
    testOrGenerate(files, EnrollmentRequest.newBuilder());
  }

  @Test
  public void testEnrollmentResponse() throws Exception {
    String[] files = {"EnrollmentResponse-ER2500"};
    testOrGenerate(files, EnrollmentResponse.newBuilder());
  }

  @Test
  public void testEpisodeOfCare() throws Exception {
    String[] files = {"EpisodeOfCare-example"};
    testOrGenerate(files, EpisodeOfCare.newBuilder());
  }

  @Test
  public void testEventDefinition() throws Exception {
    String[] files = {"EventDefinition-example"};
    testOrGenerate(files, EventDefinition.newBuilder());
  }

  @Test
  public void testEvidence() throws Exception {
    String[] files = {"Evidence-example"};
    testOrGenerate(files, Evidence.newBuilder());
  }

  @Test
  public void testEvidenceVariable() throws Exception {
    String[] files = {"EvidenceVariable-example"};
    testOrGenerate(files, EvidenceVariable.newBuilder());
  }

  @Test
  public void testExampleScenario() throws Exception {
    String[] files = {"ExampleScenario-example"};
    testOrGenerate(files, ExampleScenario.newBuilder());
  }

  @Test
  public void testExplanationOfBenefit() throws Exception {
    String[] files = {"ExplanationOfBenefit-EB3500", "ExplanationOfBenefit-EB3501"};
    testOrGenerate(files, ExplanationOfBenefit.newBuilder());
  }

  @Test
  public void testFamilyMemberHistory() throws Exception {
    String[] files = {"FamilyMemberHistory-father", "FamilyMemberHistory-mother"};
    testOrGenerate(files, FamilyMemberHistory.newBuilder());
  }

  @Test
  public void testFlag() throws Exception {
    String[] files = {"Flag-example-encounter", "Flag-example"};
    testOrGenerate(files, Flag.newBuilder());
  }

  @Test
  public void testGoal() throws Exception {
    String[] files = {"Goal-example", "Goal-stop-smoking"};
    testOrGenerate(files, Goal.newBuilder());
  }

  @Test
  public void testGraphDefinition() throws Exception {
    String[] files = {"GraphDefinition-example"};
    testOrGenerate(files, GraphDefinition.newBuilder());
  }

  @Test
  public void testGroup() throws Exception {
    String[] files = {"Group-101", "Group-102", "Group-example-patientlist", "Group-herd1"};
    testOrGenerate(files, Group.newBuilder());
  }

  @Test
  public void testGuidanceResponse() throws Exception {
    String[] files = {"GuidanceResponse-example"};
    testOrGenerate(files, GuidanceResponse.newBuilder());
  }

  @Test
  public void testHealthcareService() throws Exception {
    String[] files = {"HealthcareService-example"};
    testOrGenerate(files, HealthcareService.newBuilder());
  }

  @Test
  public void testImagingStudy() throws Exception {
    String[] files = {"ImagingStudy-example", "ImagingStudy-example-xr"};
    testOrGenerate(files, ImagingStudy.newBuilder());
  }

  @Test
  public void testImmunization() throws Exception {
    String[] files = {
      "Immunization-example",
      "Immunization-historical",
      "Immunization-notGiven",
      "Immunization-protocol",
      "Immunization-subpotent"
    };
    testOrGenerate(files, Immunization.newBuilder());
  }

  @Test
  public void testImmunizationEvaluation() throws Exception {
    String[] files = {"ImmunizationEvaluation-example", "ImmunizationEvaluation-notValid"};
    testOrGenerate(files, ImmunizationEvaluation.newBuilder());
  }

  @Test
  public void testImmunizationRecommendation() throws Exception {
    String[] files = {"ImmunizationRecommendation-example"};
    testOrGenerate(files, ImmunizationRecommendation.newBuilder());
  }

  @Test
  public void testImplementationGuide() throws Exception {
    // "ImplementationGuide-fhir" and "ig-r4"do not parse because it contains a reference to an
    // invalid resource
    // https://gforge.hl7.org/gf/project/fhir/tracker/?action=TrackerItemEdit&tracker_item_id=22489
    String[] files = {"ImplementationGuide-example"};
    testOrGenerate(files, ImplementationGuide.newBuilder());
  }

  @Test
  public void testInsurancePlan() throws Exception {
    String[] files = {"InsurancePlan-example"};
    testOrGenerate(files, InsurancePlan.newBuilder());
  }

  @Test
  public void testInvoice() throws Exception {
    String[] files = {"Invoice-example"};
    testOrGenerate(files, Invoice.newBuilder());
  }

  @Test
  public void testLibrary() throws Exception {
    String[] files = {
      "Library-composition-example",
      "Library-example",
      "Library-hiv-indicators",
      "Library-library-cms146-example",
      "Library-library-exclusive-breastfeeding-cds-logic",
      "Library-library-exclusive-breastfeeding-cqm-logic",
      "Library-library-fhir-helpers",
      "Library-library-fhir-helpers-predecessor",
      "Library-library-fhir-model-definition",
      "Library-library-quick-model-definition",
      "Library-omtk-logic",
      "Library-omtk-modelinfo",
      "Library-opioidcds-common",
      "Library-opioidcds-recommendation-04",
      "Library-opioidcds-recommendation-05",
      "Library-opioidcds-recommendation-07",
      "Library-opioidcds-recommendation-08",
      "Library-opioidcds-recommendation-10",
      "Library-opioidcds-recommendation-11",
      "Library-suiciderisk-orderset-logic",
      "Library-zika-virus-intervention-logic"
    };
    testOrGenerate(files, Library.newBuilder());
  }

  @Test
  public void testLinkage() throws Exception {
    String[] files = {"Linkage-example"};
    testOrGenerate(files, Linkage.newBuilder());
  }

  @Test
  public void testList() throws Exception {
    String[] files = {
      "List-current-allergies",
      "List-example-double-cousin-relationship",
      "List-example-empty",
      "List-example",
      "List-example-simple-empty",
      "List-f201",
      "List-genetic",
      "List-long",
      "List-med-list",
      "List-prognosis"
    };
    testOrGenerate(files, List.newBuilder());
  }

  @Test
  public void testLocation() throws Exception {
    String[] files = {
      "Location-1", "Location-2", "Location-amb", "Location-hl7", "Location-ph", "Location-ukp"
    };
    testOrGenerate(files, Location.newBuilder());
  }

  @Test
  public void testMeasure() throws Exception {
    String[] files = {
      "Measure-component-a-example",
      "Measure-component-b-example",
      "Measure-composite-example",
      "Measure-hiv-indicators",
      "Measure-measure-cms146-example",
      "Measure-measure-exclusive-breastfeeding",
      "Measure-measure-predecessor-example"
    };
    testOrGenerate(files, Measure.newBuilder());
  }

  @Test
  public void testMeasureReport() throws Exception {
    String[] files = {
      "MeasureReport-hiv-indicators",
      "MeasureReport-measurereport-cms146-cat1-example",
      "MeasureReport-measurereport-cms146-cat2-example",
      "MeasureReport-measurereport-cms146-cat3-example"
    };
    testOrGenerate(files, MeasureReport.newBuilder());
  }

  @Test
  public void testMedia() throws Exception {
    String[] files = {
      "Media-1.2.840.11361907579238403408700.3.1.04.19970327150033",
      "Media-example",
      "Media-sound",
      "Media-xray"
    };
    testOrGenerate(files, Media.newBuilder());
  }

  @Test
  public void testMedication() throws Exception {
    String[] files = {
      "Medication-med0301",
      "Medication-med0302",
      "Medication-med0303",
      "Medication-med0304",
      "Medication-med0305",
      "Medication-med0306",
      "Medication-med0307",
      "Medication-med0308",
      "Medication-med0309",
      "Medication-med0310",
      "Medication-med0311",
      "Medication-med0312",
      "Medication-med0313",
      "Medication-med0314",
      "Medication-med0315",
      "Medication-med0316",
      "Medication-med0317",
      "Medication-med0318",
      "Medication-med0319",
      "Medication-med0320",
      "Medication-med0321",
      "Medication-medexample015",
      "Medication-medicationexample1"
    };
    testOrGenerate(files, Medication.newBuilder());
  }

  @Test
  public void testMedicationAdministration() throws Exception {
    String[] files = {
      "MedicationAdministration-medadmin0301",
      "MedicationAdministration-medadmin0302",
      "MedicationAdministration-medadmin0303",
      "MedicationAdministration-medadmin0304",
      "MedicationAdministration-medadmin0305",
      "MedicationAdministration-medadmin0306",
      "MedicationAdministration-medadmin0307",
      "MedicationAdministration-medadmin0308",
      "MedicationAdministration-medadmin0309",
      "MedicationAdministration-medadmin0310",
      "MedicationAdministration-medadmin0311",
      "MedicationAdministration-medadmin0312",
      "MedicationAdministration-medadmin0313",
      "MedicationAdministration-medadminexample03"
    };
    testOrGenerate(files, MedicationAdministration.newBuilder());
  }

  @Test
  public void testMedicationDispense() throws Exception {
    String[] files = {
      "MedicationDispense-meddisp008",
      "MedicationDispense-meddisp0301",
      "MedicationDispense-meddisp0302",
      "MedicationDispense-meddisp0303",
      "MedicationDispense-meddisp0304",
      "MedicationDispense-meddisp0305",
      "MedicationDispense-meddisp0306",
      "MedicationDispense-meddisp0307",
      "MedicationDispense-meddisp0308",
      "MedicationDispense-meddisp0309",
      "MedicationDispense-meddisp0310",
      "MedicationDispense-meddisp0311",
      "MedicationDispense-meddisp0312",
      "MedicationDispense-meddisp0313",
      "MedicationDispense-meddisp0314",
      "MedicationDispense-meddisp0315",
      "MedicationDispense-meddisp0316",
      "MedicationDispense-meddisp0317",
      "MedicationDispense-meddisp0318",
      "MedicationDispense-meddisp0319",
      "MedicationDispense-meddisp0320",
      "MedicationDispense-meddisp0321",
      "MedicationDispense-meddisp0322",
      "MedicationDispense-meddisp0324",
      "MedicationDispense-meddisp0325",
      "MedicationDispense-meddisp0326",
      "MedicationDispense-meddisp0327",
      "MedicationDispense-meddisp0328",
      "MedicationDispense-meddisp0329",
      "MedicationDispense-meddisp0330",
      "MedicationDispense-meddisp0331"
    };
    testOrGenerate(files, MedicationDispense.newBuilder());
  }

  @Test
  public void testMedicationKnowledge() throws Exception {
    String[] files = {"MedicationKnowledge-example"};
    testOrGenerate(files, MedicationKnowledge.newBuilder());
  }

  @Test
  public void testMedicationRequest() throws Exception {
    String[] files = {
      "MedicationRequest-medrx002",
      "MedicationRequest-medrx0301",
      "MedicationRequest-medrx0302",
      "MedicationRequest-medrx0303",
      "MedicationRequest-medrx0304",
      "MedicationRequest-medrx0305",
      "MedicationRequest-medrx0306",
      "MedicationRequest-medrx0307",
      "MedicationRequest-medrx0308",
      "MedicationRequest-medrx0309",
      "MedicationRequest-medrx0310",
      "MedicationRequest-medrx0311",
      "MedicationRequest-medrx0312",
      "MedicationRequest-medrx0313",
      "MedicationRequest-medrx0314",
      "MedicationRequest-medrx0315",
      "MedicationRequest-medrx0316",
      "MedicationRequest-medrx0317",
      "MedicationRequest-medrx0318",
      "MedicationRequest-medrx0319",
      "MedicationRequest-medrx0320",
      "MedicationRequest-medrx0321",
      "MedicationRequest-medrx0322",
      "MedicationRequest-medrx0323",
      "MedicationRequest-medrx0324",
      "MedicationRequest-medrx0325",
      "MedicationRequest-medrx0326",
      "MedicationRequest-medrx0327",
      "MedicationRequest-medrx0328",
      "MedicationRequest-medrx0329",
      "MedicationRequest-medrx0330",
      "MedicationRequest-medrx0331",
      "MedicationRequest-medrx0332",
      "MedicationRequest-medrx0333",
      "MedicationRequest-medrx0334",
      "MedicationRequest-medrx0335",
      "MedicationRequest-medrx0336",
      "MedicationRequest-medrx0337",
      "MedicationRequest-medrx0338",
      "MedicationRequest-medrx0339"
    };
    testOrGenerate(files, MedicationRequest.newBuilder());
  }

  @Test
  public void testMedicationStatement() throws Exception {
    String[] files = {
      "MedicationStatement-example001",
      "MedicationStatement-example002",
      "MedicationStatement-example003",
      "MedicationStatement-example004",
      "MedicationStatement-example005",
      "MedicationStatement-example006",
      "MedicationStatement-example007"
    };
    testOrGenerate(files, MedicationStatement.newBuilder());
  }

  @Test
  public void testMedicinalProduct() throws Exception {
    String[] files = {"MedicinalProduct-example"};
    testOrGenerate(files, MedicinalProduct.newBuilder());
  }

  @Test
  public void testMedicinalProductAuthorization() throws Exception {
    String[] files = {"MedicinalProductAuthorization-example"};
    testOrGenerate(files, MedicinalProductAuthorization.newBuilder());
  }

  @Test
  public void testMedicinalProductContraindication() throws Exception {
    String[] files = {"MedicinalProductContraindication-example"};
    testOrGenerate(files, MedicinalProductContraindication.newBuilder());
  }

  @Test
  public void testMedicinalProductIndication() throws Exception {
    String[] files = {"MedicinalProductIndication-example"};
    testOrGenerate(files, MedicinalProductIndication.newBuilder());
  }

  @Test
  public void testMedicinalProductIngredient() throws Exception {
    String[] files = {"MedicinalProductIngredient-example"};
    testOrGenerate(files, MedicinalProductIngredient.newBuilder());
  }

  @Test
  public void testMedicinalProductInteraction() throws Exception {
    String[] files = {"MedicinalProductInteraction-example"};
    testOrGenerate(files, MedicinalProductInteraction.newBuilder());
  }

  @Test
  public void testMedicinalProductManufactured() throws Exception {
    String[] files = {"MedicinalProductManufactured-example"};
    testOrGenerate(files, MedicinalProductManufactured.newBuilder());
  }

  @Test
  public void testMedicinalProductPackaged() throws Exception {
    String[] files = {"MedicinalProductPackaged-example"};
    testOrGenerate(files, MedicinalProductPackaged.newBuilder());
  }

  @Test
  public void testMedicinalProductPharmaceutical() throws Exception {
    String[] files = {"MedicinalProductPharmaceutical-example"};
    testOrGenerate(files, MedicinalProductPharmaceutical.newBuilder());
  }

  @Test
  public void testMedicinalProductUndesirableEffect() throws Exception {
    String[] files = {"MedicinalProductUndesirableEffect-example"};
    testOrGenerate(files, MedicinalProductUndesirableEffect.newBuilder());
  }

  @Test
  public void testMessageDefinition() throws Exception {
    String[] files = {
      "MessageDefinition-example",
      "MessageDefinition-patient-link-notification",
      "MessageDefinition-patient-link-response"
    };
    testOrGenerate(files, MessageDefinition.newBuilder());
  }

  @Test
  public void testMessageHeader() throws Exception {
    String[] files = {"MessageHeader-1cbdfb97-5859-48a4-8301-d54eab818d68"};
    testOrGenerate(files, MessageHeader.newBuilder());
  }

  @Test
  public void testMolecularSequence() throws Exception {
    String[] files = {
      "MolecularSequence-breastcancer",
      "MolecularSequence-coord-0-base",
      "MolecularSequence-coord-1-base",
      "MolecularSequence-example",
      "MolecularSequence-example-pgx-1",
      "MolecularSequence-example-pgx-2",
      "MolecularSequence-example-TPMT-one",
      "MolecularSequence-example-TPMT-two",
      "MolecularSequence-fda-example",
      "MolecularSequence-fda-vcf-comparison",
      "MolecularSequence-fda-vcfeval-comparison",
      "MolecularSequence-graphic-example-1",
      "MolecularSequence-graphic-example-2",
      "MolecularSequence-graphic-example-3",
      "MolecularSequence-graphic-example-4",
      "MolecularSequence-graphic-example-5",
      "MolecularSequence-sequence-complex-variant"
    };
    testOrGenerate(files, MolecularSequence.newBuilder());
  }

  @Test
  public void testNamingSystem() throws Exception {
    String[] files = {"NamingSystem-example-id", "NamingSystem-example"};
    testOrGenerate(files, NamingSystem.newBuilder());
  }

  @Test
  public void testNutritionOrder() throws Exception {
    String[] files = {
      "NutritionOrder-cardiacdiet",
      "NutritionOrder-diabeticdiet",
      "NutritionOrder-diabeticsupplement",
      "NutritionOrder-energysupplement",
      "NutritionOrder-enteralbolus",
      "NutritionOrder-enteralcontinuous",
      "NutritionOrder-fiberrestricteddiet",
      "NutritionOrder-infantenteral",
      "NutritionOrder-proteinsupplement",
      "NutritionOrder-pureeddiet",
      "NutritionOrder-pureeddiet-simple",
      "NutritionOrder-renaldiet",
      "NutritionOrder-texturemodified"
    };
    testOrGenerate(files, NutritionOrder.newBuilder());
  }

  @Test
  public void testObservation() throws Exception {
    String[] files = {
      "Observation-10minute-apgar-score",
      "Observation-1minute-apgar-score",
      "Observation-20minute-apgar-score",
      "Observation-2minute-apgar-score",
      "Observation-5minute-apgar-score",
      "Observation-656",
      "Observation-abdo-tender",
      "Observation-alcohol-type",
      "Observation-bgpanel",
      "Observation-bloodgroup",
      "Observation-blood-pressure-cancel",
      "Observation-blood-pressure-dar",
      "Observation-blood-pressure",
      "Observation-bmd",
      "Observation-bmi",
      "Observation-bmi-using-related",
      "Observation-body-height",
      "Observation-body-length",
      "Observation-body-temperature",
      "Observation-clinical-gender",
      "Observation-date-lastmp",
      "Observation-decimal",
      "Observation-ekg",
      "Observation-example-diplotype1",
      "Observation-example-genetics-1",
      "Observation-example-genetics-2",
      "Observation-example-genetics-3",
      "Observation-example-genetics-4",
      "Observation-example-genetics-5",
      "Observation-example-genetics-brcapat",
      "Observation-example-haplotype1",
      "Observation-example-haplotype2",
      "Observation-example",
      "Observation-example-phenotype",
      "Observation-example-TPMT-diplotype",
      "Observation-example-TPMT-haplotype-one",
      "Observation-example-TPMT-haplotype-two",
      "Observation-eye-color",
      "Observation-f001",
      "Observation-f002",
      "Observation-f003",
      "Observation-f004",
      "Observation-f005",
      "Observation-f202",
      "Observation-f203",
      "Observation-f204",
      "Observation-f205",
      "Observation-f206",
      "Observation-gcs-qa",
      "Observation-glasgow",
      "Observation-head-circumference",
      "Observation-heart-rate",
      "Observation-herd1",
      "Observation-map-sitting",
      "Observation-mbp",
      "Observation-respiratory-rate",
      "Observation-rhstatus",
      "Observation-satO2",
      "Observation-secondsmoke",
      "Observation-trachcare",
      "Observation-unsat",
      "Observation-vitals-panel",
      "Observation-vomiting",
      "Observation-vp-oyster"
    };
    testOrGenerate(files, Observation.newBuilder());
  }

  @Test
  public void testObservationDefinition() throws Exception {
    String[] files = {"ObservationDefinition-example"};
    testOrGenerate(files, ObservationDefinition.newBuilder());
  }

  @Test
  public void testOperationDefinition() throws Exception {
    String[] files = {
      "OperationDefinition-ActivityDefinition-apply",
      "OperationDefinition-ActivityDefinition-data-requirements",
      "OperationDefinition-CapabilityStatement-conforms",
      "OperationDefinition-CapabilityStatement-implements",
      "OperationDefinition-CapabilityStatement-subset",
      "OperationDefinition-CapabilityStatement-versions",
      "OperationDefinition-ChargeItemDefinition-apply",
      "OperationDefinition-Claim-submit",
      "OperationDefinition-CodeSystem-find-matches",
      "OperationDefinition-CodeSystem-lookup",
      "OperationDefinition-CodeSystem-subsumes",
      "OperationDefinition-CodeSystem-validate-code",
      "OperationDefinition-Composition-document",
      "OperationDefinition-ConceptMap-closure",
      "OperationDefinition-ConceptMap-translate",
      "OperationDefinition-CoverageEligibilityRequest-submit",
      "OperationDefinition-Encounter-everything",
      "OperationDefinition-example",
      "OperationDefinition-Group-everything",
      "OperationDefinition-Library-data-requirements",
      "OperationDefinition-List-find",
      "OperationDefinition-Measure-care-gaps",
      "OperationDefinition-Measure-collect-data",
      "OperationDefinition-Measure-data-requirements",
      "OperationDefinition-Measure-evaluate-measure",
      "OperationDefinition-Measure-submit-data",
      "OperationDefinition-MedicinalProduct-everything",
      "OperationDefinition-MessageHeader-process-message",
      "OperationDefinition-NamingSystem-preferred-id",
      "OperationDefinition-Observation-lastn",
      "OperationDefinition-Observation-stats",
      "OperationDefinition-Patient-everything",
      "OperationDefinition-Patient-match",
      "OperationDefinition-PlanDefinition-apply",
      "OperationDefinition-PlanDefinition-data-requirements",
      "OperationDefinition-Resource-convert",
      "OperationDefinition-Resource-graph",
      "OperationDefinition-Resource-graphql",
      "OperationDefinition-Resource-meta-add",
      "OperationDefinition-Resource-meta-delete",
      "OperationDefinition-Resource-meta",
      "OperationDefinition-Resource-validate",
      "OperationDefinition-StructureDefinition-questionnaire",
      "OperationDefinition-StructureDefinition-snapshot",
      "OperationDefinition-StructureMap-transform",
      "OperationDefinition-ValueSet-expand",
      "OperationDefinition-ValueSet-validate-code"
    };
    testOrGenerate(files, OperationDefinition.newBuilder());
  }

  @Test
  public void testOperationOutcome() throws Exception {
    String[] files = {
      "OperationOutcome-101",
      "OperationOutcome-allok",
      "OperationOutcome-break-the-glass",
      "OperationOutcome-exception",
      "OperationOutcome-searchfail",
      "OperationOutcome-validationfail"
    };
    testOrGenerate(files, OperationOutcome.newBuilder());
  }

  @Test
  public void testOrganization() throws Exception {
    String[] files = {
      "Organization-1832473e-2fe0-452d-abe9-3cdb9879522f",
      "Organization-1",
      "Organization-2.16.840.1.113883.19.5",
      "Organization-2",
      "Organization-3",
      "Organization-f001",
      "Organization-f002",
      "Organization-f003",
      "Organization-f201",
      "Organization-f203",
      "Organization-hl7",
      "Organization-hl7pay",
      "Organization-mmanu"
    };
    testOrGenerate(files, Organization.newBuilder());
  }

  @Test
  public void testOrganizationAffiliation() throws Exception {
    String[] files = {
      "OrganizationAffiliation-example",
      "OrganizationAffiliation-orgrole1",
      "OrganizationAffiliation-orgrole2"
    };
    testOrGenerate(files, OrganizationAffiliation.newBuilder());
  }

  @Test
  public void testPatient() throws Exception {
    String[] files = {
      "Patient-animal",
      "Patient-ch-example",
      "Patient-dicom",
      "Patient-example",
      "Patient-f001",
      "Patient-f201",
      "Patient-genetics-example1",
      "Patient-glossy",
      "Patient-ihe-pcd",
      "Patient-infant-fetal",
      "Patient-infant-mom",
      "Patient-infant-twin-1",
      "Patient-infant-twin-2",
      "Patient-mom",
      "Patient-newborn",
      "Patient-pat1",
      "Patient-pat2",
      "Patient-pat3",
      "Patient-pat4",
      "Patient-proband",
      "Patient-xcda",
      "Patient-xds"
    };
    testOrGenerate(files, Patient.newBuilder());
  }

  @Test
  public void testPaymentNotice() throws Exception {
    String[] files = {"PaymentNotice-77654"};
    testOrGenerate(files, PaymentNotice.newBuilder());
  }

  @Test
  public void testPaymentReconciliation() throws Exception {
    String[] files = {"PaymentReconciliation-ER2500"};
    testOrGenerate(files, PaymentReconciliation.newBuilder());
  }

  @Test
  public void testPerson() throws Exception {
    String[] files = {"Person-example", "Person-f002", "Person-grahame", "Person-pd", "Person-pp"};
    testOrGenerate(files, Person.newBuilder());
  }

  @Test
  public void testPlanDefinition() throws Exception {
    String[] files = {
      "PlanDefinition-chlamydia-screening-intervention",
      "PlanDefinition-example-cardiology-os",
      "PlanDefinition-exclusive-breastfeeding-intervention-01",
      "PlanDefinition-exclusive-breastfeeding-intervention-02",
      "PlanDefinition-exclusive-breastfeeding-intervention-03",
      "PlanDefinition-exclusive-breastfeeding-intervention-04",
      "PlanDefinition-KDN5",
      "PlanDefinition-low-suicide-risk-order-set",
      "PlanDefinition-opioidcds-04",
      "PlanDefinition-opioidcds-05",
      "PlanDefinition-opioidcds-07",
      "PlanDefinition-opioidcds-08",
      "PlanDefinition-opioidcds-10",
      "PlanDefinition-opioidcds-11",
      "PlanDefinition-options-example",
      "PlanDefinition-protocol-example",
      "PlanDefinition-zika-virus-intervention-initial",
      "PlanDefinition-zika-virus-intervention"
    };
    testOrGenerate(files, PlanDefinition.newBuilder());
  }

  @Test
  public void testPractitioner() throws Exception {
    String[] files = {
      "Practitioner-example",
      "Practitioner-f001",
      "Practitioner-f002",
      "Practitioner-f003",
      "Practitioner-f004",
      "Practitioner-f005",
      "Practitioner-f006",
      "Practitioner-f007",
      "Practitioner-f201",
      "Practitioner-f202",
      "Practitioner-f203",
      "Practitioner-f204",
      "Practitioner-xcda1",
      "Practitioner-xcda-author"
    };
    testOrGenerate(files, Practitioner.newBuilder());
  }

  @Test
  public void testPractitionerRole() throws Exception {
    String[] files = {"PractitionerRole-example"};
    testOrGenerate(files, PractitionerRole.newBuilder());
  }

  @Test
  public void testProcedure() throws Exception {
    String[] files = {
      "Procedure-ambulation",
      "Procedure-appendectomy-narrative",
      "Procedure-biopsy",
      "Procedure-colon-biopsy",
      "Procedure-colonoscopy",
      "Procedure-education",
      "Procedure-example-implant",
      "Procedure-example",
      "Procedure-f001",
      "Procedure-f002",
      "Procedure-f003",
      "Procedure-f004",
      "Procedure-f201",
      "Procedure-HCBS",
      "Procedure-ob",
      "Procedure-physical-therapy"
    };
    testOrGenerate(files, Procedure.newBuilder());
  }

  @Test
  public void testProvenance() throws Exception {
    String[] files = {
      "Provenance-consent-signature",
      "Provenance-example-biocompute-object",
      "Provenance-example-cwl",
      "Provenance-example",
      "Provenance-signature"
    };
    testOrGenerate(files, Provenance.newBuilder());
  }

  @Test
  public void testQuestionnaire() throws Exception {
    String[] files = {
      "Questionnaire-3141",
      "Questionnaire-bb",
      "Questionnaire-f201",
      "Questionnaire-gcs",
      "Questionnaire-phq-9-questionnaire",
      "Questionnaire-qs1",
      "Questionnaire-zika-virus-exposure-assessment"
    };
    testOrGenerate(files, Questionnaire.newBuilder());
  }

  @Test
  public void testQuestionnaireResponse() throws Exception {
    String[] files = {
      "QuestionnaireResponse-3141",
      "QuestionnaireResponse-bb",
      "QuestionnaireResponse-f201",
      "QuestionnaireResponse-gcs",
      "QuestionnaireResponse-ussg-fht-answers"
    };
    testOrGenerate(files, QuestionnaireResponse.newBuilder());
  }

  @Test
  public void testRelatedPerson() throws Exception {
    String[] files = {
      "RelatedPerson-benedicte",
      "RelatedPerson-f001",
      "RelatedPerson-f002",
      "RelatedPerson-newborn-mom",
      "RelatedPerson-peter"
    };
    testOrGenerate(files, RelatedPerson.newBuilder());
  }

  @Test
  public void testRequestGroup() throws Exception {
    String[] files = {"RequestGroup-example", "RequestGroup-kdn5-example"};
    testOrGenerate(files, RequestGroup.newBuilder());
  }

  @Test
  public void testResearchDefinition() throws Exception {
    String[] files = {"ResearchDefinition-example"};
    testOrGenerate(files, ResearchDefinition.newBuilder());
  }

  @Test
  public void testResearchElementDefinition() throws Exception {
    String[] files = {"ResearchElementDefinition-example"};
    testOrGenerate(files, ResearchElementDefinition.newBuilder());
  }

  @Test
  public void testResearchStudy() throws Exception {
    String[] files = {"ResearchStudy-example"};
    testOrGenerate(files, ResearchStudy.newBuilder());
  }

  @Test
  public void testResearchSubject() throws Exception {
    String[] files = {"ResearchSubject-example"};
    testOrGenerate(files, ResearchSubject.newBuilder());
  }

  @Test
  public void testRiskAssessment() throws Exception {
    String[] files = {
      "RiskAssessment-breastcancer-risk",
      "RiskAssessment-cardiac",
      "RiskAssessment-genetic",
      "RiskAssessment-population",
      "RiskAssessment-prognosis",
      "RiskAssessment-riskexample"
    };
    testOrGenerate(files, RiskAssessment.newBuilder());
  }

  @Test
  public void testRiskEvidenceSynthesis() throws Exception {
    String[] files = {"RiskEvidenceSynthesis-example"};
    testOrGenerate(files, RiskEvidenceSynthesis.newBuilder());
  }

  @Test
  public void testSchedule() throws Exception {
    String[] files = {"Schedule-example", "Schedule-exampleloc1", "Schedule-exampleloc2"};
    testOrGenerate(files, Schedule.newBuilder());
  }

  @Test
  public void testServiceRequest() throws Exception {
    String[] files = {
      "ServiceRequest-ambulation",
      "ServiceRequest-appendectomy-narrative",
      "ServiceRequest-benchpress",
      "ServiceRequest-colon-biopsy",
      "ServiceRequest-colonoscopy",
      "ServiceRequest-di",
      "ServiceRequest-do-not-turn",
      "ServiceRequest-education",
      "ServiceRequest-example-implant",
      "ServiceRequest-example",
      "ServiceRequest-example-pgx",
      "ServiceRequest-ft4",
      "ServiceRequest-lipid",
      "ServiceRequest-myringotomy",
      "ServiceRequest-ob",
      "ServiceRequest-og-example1",
      "ServiceRequest-physical-therapy",
      "ServiceRequest-physiotherapy",
      "ServiceRequest-subrequest",
      "ServiceRequest-vent"
    };
    testOrGenerate(files, ServiceRequest.newBuilder());
  }

  @Test
  public void testSlot() throws Exception {
    String[] files = {"Slot-1", "Slot-2", "Slot-3", "Slot-example"};
    testOrGenerate(files, Slot.newBuilder());
  }

  @Test
  public void testSpecimen() throws Exception {
    String[] files = {
      "Specimen-101",
      "Specimen-isolate",
      "Specimen-pooled-serum",
      "Specimen-sst",
      "Specimen-vma-urine"
    };
    testOrGenerate(files, Specimen.newBuilder());
  }

  @Test
  public void testSpecimenDefinition() throws Exception {
    String[] files = {"SpecimenDefinition-2364"};
    testOrGenerate(files, SpecimenDefinition.newBuilder());
  }

  @Test
  public void testStructureMap() throws Exception {
    String[] files = {"StructureMap-example", "StructureMap-supplyrequest-transform"};
    testOrGenerate(files, StructureMap.newBuilder());
  }

  @Test
  public void testSubscription() throws Exception {
    String[] files = {"Subscription-example-error", "Subscription-example"};
    testOrGenerate(files, Subscription.newBuilder());
  }

  @Test
  public void testSubstance() throws Exception {
    String[] files = {
      "Substance-example",
      "Substance-f201",
      "Substance-f202",
      "Substance-f203",
      "Substance-f204",
      "Substance-f205"
    };
    testOrGenerate(files, Substance.newBuilder());
  }

  @Test
  public void testSubstanceSpecification() throws Exception {
    String[] files = {"SubstanceSpecification-example"};
    testOrGenerate(files, SubstanceSpecification.newBuilder());
  }

  @Test
  public void testSupplyDelivery() throws Exception {
    String[] files = {"SupplyDelivery-pumpdelivery", "SupplyDelivery-simpledelivery"};
    testOrGenerate(files, SupplyDelivery.newBuilder());
  }

  @Test
  public void testSupplyRequest() throws Exception {
    String[] files = {"SupplyRequest-simpleorder"};
    testOrGenerate(files, SupplyRequest.newBuilder());
  }

  @Test
  public void testTask() throws Exception {
    String[] files = {
      "Task-example1",
      "Task-example2",
      "Task-example3",
      "Task-example4",
      "Task-example5",
      "Task-example6",
      "Task-fm-example1",
      "Task-fm-example2",
      "Task-fm-example3",
      "Task-fm-example4",
      "Task-fm-example5",
      "Task-fm-example6"
    };
    testOrGenerate(files, Task.newBuilder());
  }

  @Test
  public void testTerminologyCapabilities() throws Exception {
    String[] files = {"TerminologyCapabilities-example"};
    testOrGenerate(files, TerminologyCapabilities.newBuilder());
  }

  @Test
  public void testTestReport() throws Exception {
    String[] files = {"TestReport-testreport-example"};
    testOrGenerate(files, TestReport.newBuilder());
  }

  @Test
  public void testTestScript() throws Exception {
    String[] files = {
      "TestScript-testscript-example-history",
      "TestScript-testscript-example",
      "TestScript-testscript-example-multisystem",
      "TestScript-testscript-example-readtest",
      "TestScript-testscript-example-search",
      "TestScript-testscript-example-update"
    };
    testOrGenerate(files, TestScript.newBuilder());
  }

  @Test
  public void testVerificationResult() throws Exception {
    String[] files = {"VerificationResult-example"};
    testOrGenerate(files, VerificationResult.newBuilder());
  }

  @Test
  public void testVisionPrescription() throws Exception {
    String[] files = {"VisionPrescription-33123", "VisionPrescription-33124"};
    testOrGenerate(files, VisionPrescription.newBuilder());
  }
}
