//    Copyright 2022 Google LLC
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

import static com.google.common.truth.Truth.assertThat;
import static java.nio.charset.StandardCharsets.UTF_8;
import static org.junit.Assert.assertThrows;

import com.google.common.collect.ImmutableList;
import com.google.fhir.r4.core.CodeSystem;
import com.google.fhir.r4.core.SearchParameter;
import com.google.fhir.r4.core.StructureDefinition;
import com.google.fhir.r4.core.Uri;
import com.google.fhir.r4.core.ValueSet;
import java.io.File;
import java.io.FileOutputStream;
import java.nio.charset.Charset;
import java.util.List;
import java.util.Optional;
import java.util.UUID;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public final class FhirPackageManagerTest {
  /** A test data file's name and its contents. */
  private static class PackageFile {
    PackageFile(String fileName, String fileContents) {
      this.fileName = fileName;
      this.fileContents = fileContents;
    }

    protected final String fileName;
    protected final String fileContents;
  }

  /**
   * Loads a FHIR package with the provided `files`.
   *
   * <p>The filename of the backing ZIP is randomly generated.
   */
  FhirPackage loadFhirPackage(List<PackageFile> files) throws Exception {
    File f = File.createTempFile(UUID.randomUUID().toString(), ".zip");
    try (ZipOutputStream out = new ZipOutputStream(new FileOutputStream(f))) {
      for (PackageFile file : files) {
        ZipEntry e = new ZipEntry(file.fileName);
        out.putNextEntry(e);
        byte[] data = file.fileContents.getBytes(Charset.forName(UTF_8.name()));
        out.write(data, 0, data.length);
        out.closeEntry();
      }
    }
    return FhirPackage.load(f.getAbsolutePath());
  }

  private static final PackageFile VALID_PACKAGE_INFO =
      new PackageFile(
          /*fileName=*/ "foo_package_info.textproto",
          /*fileContents=*/ "proto_package: \"google.foo\""
              + "\njava_proto_package: \"com.google.foo\""
              + "\nfhir_version: R4"
              + "\nlicense: APACHE"
              + "\nlicense_date: \"2022\""
              + "\nlocal_contained_resource: true"
              + "\nfile_splitting_behavior: SPLIT_RESOURCES");

  @Test
  public void addPackage_duplicatePackage_throws() throws Exception {
    FhirPackage fhirPackage = loadFhirPackage(ImmutableList.of(VALID_PACKAGE_INFO));
    FhirPackageManager manager = new FhirPackageManager();
    manager.addPackage(fhirPackage);

    assertThat(assertThrows(IllegalArgumentException.class, () -> manager.addPackage(fhirPackage)))
        .hasMessageThat()
        .isEqualTo("Already managing package: google.foo");
  }

  @Test
  public void addPackage_withStructureDefinition_canRetrieve() throws Exception {
    FhirPackage fhirPackage =
        loadFhirPackage(
            ImmutableList.of(
                VALID_PACKAGE_INFO,
                new PackageFile(
                    /*fileName=*/ "foo.json",
                    /*fileContents=*/ "{\"resourceType\":\"StructureDefinition\","
                        + " \"url\":\"http://foo.com\"}")));
    FhirPackageManager manager = new FhirPackageManager();
    manager.addPackage(fhirPackage);

    assertThat(manager.getStructureDefinition("http://foo.com"))
        .isEqualTo(
            Optional.of(
                StructureDefinition.newBuilder()
                    .setUrl(Uri.newBuilder().setValue("http://foo.com"))
                    .build()));
    assertThat(manager.getStructureDefinition("http://bar.com")).isEqualTo(Optional.empty());
  }

  @Test
  public void addPackage_withSearchParameter_canRetrieve() throws Exception {
    FhirPackage fhirPackage =
        loadFhirPackage(
            ImmutableList.of(
                VALID_PACKAGE_INFO,
                new PackageFile(
                    /*fileName=*/ "foo.json",
                    /*fileContents=*/ "{\"resourceType\":\"SearchParameter\","
                        + " \"url\":\"http://foo.com\"}")));
    FhirPackageManager manager = new FhirPackageManager();
    manager.addPackage(fhirPackage);

    assertThat(manager.getSearchParameter("http://foo.com"))
        .isEqualTo(
            Optional.of(
                SearchParameter.newBuilder()
                    .setUrl(Uri.newBuilder().setValue("http://foo.com"))
                    .build()));
    assertThat(manager.getSearchParameter("http://bar.com")).isEqualTo(Optional.empty());
  }

  @Test
  public void addPackage_withCodeSystem_canRetrieve() throws Exception {
    FhirPackage fhirPackage =
        loadFhirPackage(
            ImmutableList.of(
                VALID_PACKAGE_INFO,
                new PackageFile(
                    /*fileName=*/ "foo.json",
                    /*fileContents=*/ "{\"resourceType\":\"CodeSystem\","
                        + " \"url\":\"http://foo.com\"}")));
    FhirPackageManager manager = new FhirPackageManager();
    manager.addPackage(fhirPackage);

    assertThat(manager.getCodeSystem("http://foo.com"))
        .isEqualTo(
            Optional.of(
                CodeSystem.newBuilder()
                    .setUrl(Uri.newBuilder().setValue("http://foo.com"))
                    .build()));
    assertThat(manager.getCodeSystem("http://bar.com")).isEqualTo(Optional.empty());
  }

  @Test
  public void addPackage_withValueSet_canRetrieve() throws Exception {
    FhirPackage fhirPackage =
        loadFhirPackage(
            ImmutableList.of(
                VALID_PACKAGE_INFO,
                new PackageFile(
                    /*fileName=*/ "foo.json",
                    /*fileContents=*/ "{\"resourceType\":\"ValueSet\","
                        + " \"url\":\"http://foo.com\"}")));
    FhirPackageManager manager = new FhirPackageManager();
    manager.addPackage(fhirPackage);

    assertThat(manager.getValueSet("http://foo.com"))
        .isEqualTo(
            Optional.of(
                ValueSet.newBuilder().setUrl(Uri.newBuilder().setValue("http://foo.com")).build()));
    assertThat(manager.getValueSet("http://bar.com")).isEqualTo(Optional.empty());
  }

  @Test
  public void getResource_withNoPackages_returnsEmpty() throws Exception {
    FhirPackageManager manager = new FhirPackageManager();

    assertThat(manager.getValueSet("http://foo.com")).isEqualTo(Optional.empty());
  }

  @Test
  public void getResource_acrossMultiplePackages_canRetrieve() throws Exception {
    FhirPackage fhirPackage1 =
        loadFhirPackage(
            ImmutableList.of(
                VALID_PACKAGE_INFO,
                new PackageFile(
                    /*fileName=*/ "foo.json",
                    /*fileContents=*/ "{\"resourceType\":\"ValueSet\","
                        + " \"url\":\"http://foo.com\"}")));
    PackageFile packageFile2 =
        new PackageFile(
            /*fileName=*/ "bar_package_info.textproto",
            /*fileContents=*/ "proto_package: \"google.bar\""
                + "\njava_proto_package: \"com.google.bar\""
                + "\nfhir_version: R4"
                + "\nlicense: APACHE"
                + "\nlicense_date: \"2022\""
                + "\nlocal_contained_resource: true"
                + "\nfile_splitting_behavior: SPLIT_RESOURCES");
    FhirPackage fhirPackage2 =
        loadFhirPackage(
            ImmutableList.of(
                packageFile2,
                new PackageFile(
                    /*fileName=*/ "bar.json",
                    /*fileContents=*/ "{\"resourceType\":\"ValueSet\","
                        + " \"url\":\"http://bar.com\"}")));
    FhirPackageManager manager = new FhirPackageManager();
    manager.addPackage(fhirPackage1);
    manager.addPackage(fhirPackage2);

    assertThat(manager.getValueSet("http://bar.com"))
        .isEqualTo(
            Optional.of(
                ValueSet.newBuilder().setUrl(Uri.newBuilder().setValue("http://bar.com")).build()));
    assertThat(manager.getValueSet("http://foo.com"))
        .isEqualTo(
            Optional.of(
                ValueSet.newBuilder().setUrl(Uri.newBuilder().setValue("http://foo.com")).build()));
  }
}
