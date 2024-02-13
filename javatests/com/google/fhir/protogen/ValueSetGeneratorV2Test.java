//    Copyright 2023 Google Inc.
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

package com.google.fhir.protogen;

import static com.google.common.truth.extensions.proto.ProtoTruth.assertThat;
import static com.google.fhir.protogen.ProtoGeneratorTestUtils.cleaned;
import static com.google.fhir.protogen.ProtoGeneratorTestUtils.sorted;

import com.google.common.collect.ImmutableList;
import com.google.fhir.common.InvalidFhirException;
import com.google.fhir.proto.ProtogenConfig;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import com.google.testing.junit.testparameterinjector.TestParameter;
import com.google.testing.junit.testparameterinjector.TestParameterInjector;
import com.google.testing.junit.testparameterinjector.TestParameterValuesProvider;
import java.io.IOException;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(TestParameterInjector.class)
public final class ValueSetGeneratorV2Test {
  private static FhirPackage r4Package = null;
  private static FhirPackage r5Package = null;

  /**
   * Gets the R4 package. Lazy loaded to avoid questions of class/test/parameter initialization
   * order.
   */
  private static FhirPackage getR4Package() throws IOException, InvalidFhirException {
    if (r4Package == null) {
      r4Package =
          FhirPackage.load(
              "external/hl7.fhir.r4.core_4.0.1/file/hl7.fhir.r4.core@4.0.1.tgz",
              /* no packageInfo proto */ null,
              /* ignoreUnrecognizedFieldsAndCodes= */ true);
    }
    return r4Package;
  }

  /**
   * Gets the R5 package. Lazy loaded to avoid questions of class/test/parameter initialization
   * order.
   */
  private static FhirPackage getR5Package() throws IOException, InvalidFhirException {
    if (r5Package == null) {
      r5Package =
          FhirPackage.load(
              "external/hl7.fhir.r5.core_5.0.0/file/hl7.fhir.r5.core@5.0.0.tgz",
              /* no packageInfo proto */ null,
              /* ignoreUnrecognizedFieldsAndCodes= */ true);
    }
    return r5Package;
  }

  public static ProtogenConfig makeConfig(TestEnv testEnv) throws Exception {
    return ProtogenConfig.newBuilder()
        .setProtoPackage(testEnv.protoPackage)
        .setJavaProtoPackage(testEnv.javaProtoPackage)
        .setLegacyRetagging(true)
        .build();
  }

  static class TestEnv {
    // Inputs
    FhirPackage fhirPackage;
    String protoPackage;
    String javaProtoPackage;

    // Expected outputs
    FileDescriptorProto codeSystemFile;
    FileDescriptorProto valueSetFile;

    TestEnv(
        FhirPackage fhirPackage,
        String protoPackage,
        String javaProtoPackage,
        FileDescriptorProto codeSystemFile,
        FileDescriptorProto valueSetFile) {
      this.fhirPackage = fhirPackage;
      this.protoPackage = protoPackage;
      this.javaProtoPackage = javaProtoPackage;
      this.codeSystemFile = codeSystemFile;
      this.valueSetFile = valueSetFile;
    }
  }

  private static class TestEnvProvider extends TestParameterValuesProvider {
    @Override
    public List<TestEnv> provideValues(Context context) {
      try {
        return ImmutableList.of(
            new TestEnv(
                getR4Package(),
                "google.fhir.r4.core",
                "com.google.fhir.r4.core",
                com.google.fhir.r4.core.AbstractTypeCode.getDescriptor().getFile().toProto(),
                com.google.fhir.r4.core.BodyLengthUnitsValueSet.getDescriptor()
                    .getFile()
                    .toProto()),
            new TestEnv(
                getR5Package(),
                "google.fhir.r5.core",
                "com.google.fhir.r5.core",
                com.google.fhir.r5.core.AccountStatusCode.getDescriptor().getFile().toProto(),
                com.google.fhir.r5.core.BodyLengthUnitsValueSet.getDescriptor()
                    .getFile()
                    .toProto()));
      } catch (IOException | InvalidFhirException e) {
        throw new AssertionError(e);
      }
    }
  }

  @Test
  public void regressionTest_generateCodeSystems(
      @TestParameter(valuesProvider = TestEnvProvider.class) TestEnv testEnv) throws Exception {
    FileDescriptorProto codeSystemFile =
        new ValueSetGeneratorV2(testEnv.fhirPackage, makeConfig(testEnv)).makeCodeSystemFile();

    assertThat(sorted(cleaned(codeSystemFile))).isEqualTo(sorted(cleaned(testEnv.codeSystemFile)));
  }

  @Test
  public void regressionTest_generateValueSets(
      @TestParameter(valuesProvider = TestEnvProvider.class) TestEnv testEnv) throws Exception {
    FileDescriptorProto valueSetFile =
        new ValueSetGeneratorV2(testEnv.fhirPackage, makeConfig(testEnv)).makeValueSetFile();

    assertThat(sorted(cleaned(valueSetFile))).isEqualTo(sorted(cleaned(testEnv.valueSetFile)));
  }
}
