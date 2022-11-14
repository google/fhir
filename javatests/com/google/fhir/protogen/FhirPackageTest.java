//    Copyright 2020 Google LLC
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

import static com.google.common.collect.Streams.stream;
import static com.google.common.truth.Truth.assertThat;
import static java.nio.charset.StandardCharsets.UTF_8;
import static junit.framework.Assert.assertTrue;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertThrows;

import com.google.common.collect.ImmutableList;
import com.google.common.testing.EqualsTester;
import com.google.fhir.common.InvalidFhirException;
import com.google.fhir.proto.Annotations.FhirVersion;
import com.google.fhir.proto.PackageInfo;
import com.google.fhir.proto.PackageInfo.FileSplittingBehavior;
import com.google.fhir.proto.PackageInfo.License;
import com.google.fhir.r4.core.CodeSystem;
import com.google.fhir.r4.core.Id;
import com.google.fhir.r4.core.SearchParameter;
import com.google.fhir.r4.core.StructureDefinition;
import com.google.fhir.r4.core.Uri;
import com.google.fhir.r4.core.ValueSet;
import com.google.testing.junit.testparameterinjector.TestParameter;
import com.google.testing.junit.testparameterinjector.TestParameterInjector;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.channels.Channels;
import java.nio.charset.Charset;
import java.util.List;
import java.util.Optional;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;
import org.apache.commons.compress.archivers.tar.TarArchiveEntry;
import org.apache.commons.compress.archivers.tar.TarArchiveOutputStream;
import org.apache.commons.compress.compressors.gzip.GzipCompressorOutputStream;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(TestParameterInjector.class)
public final class FhirPackageTest {
  /** A test data file's name and its contents. */
  private static class PackageFile {
    protected String fileName;
    protected String fileContents;
  }

  /**
   * Creates a temporary ZIP file for a FHIR package.
   *
   * <p>A FHIR package consists of a collection of JSON files and a package info prototxt file.
   *
   * @param fileName The filename prefix used for the temporary file created.
   * @param files The collection of files that are packaged into the temporary ZIP file.
   * @return The absolute path of the file that is created.
   */
  private static String createFhirPackageInfoZip(String fileName, List<PackageFile> files)
      throws IOException {
    File f = File.createTempFile(fileName, ".zip");
    try (ZipOutputStream out = new ZipOutputStream(new FileOutputStream(f))) {
      for (PackageFile file : files) {
        ZipEntry e = new ZipEntry(file.fileName);
        out.putNextEntry(e);
        byte[] data = file.fileContents.getBytes(Charset.forName(UTF_8.name()));
        out.write(data, 0, data.length);
        out.closeEntry();
      }
    }
    return f.getAbsolutePath();
  }

  @Test
  public void equality() throws IOException, InvalidFhirException {
    PackageInfo fooPackage =
        PackageInfo.newBuilder()
            .setProtoPackage("google.foo")
            .setFhirVersion(FhirVersion.R4)
            .build();
    PackageInfo barPackage =
        PackageInfo.newBuilder()
            .setProtoPackage("google.bar")
            .setFhirVersion(FhirVersion.R4)
            .build();
    new EqualsTester()
        .addEqualityGroup(
            FhirPackage.load(
                createFhirPackageInfoZip("foo_package", ImmutableList.of()), fooPackage),
            FhirPackage.load(
                createFhirPackageInfoZip("foo_package", ImmutableList.of()), fooPackage))
        .addEqualityGroup(
            FhirPackage.load(
                createFhirPackageInfoZip("bar_package", ImmutableList.of()), barPackage),
            FhirPackage.load(
                createFhirPackageInfoZip("bar_package", ImmutableList.of()), barPackage))
        .testEquals();
  }

  @Test
  public void isCorePackage_withCorePackage() {
    PackageInfo packageInfo =
        PackageInfo.newBuilder()
            .setProtoPackage("google.fhir.r4.core")
            .setFhirVersion(FhirVersion.R4)
            .build();

    assertThat(FhirPackage.isCorePackage(packageInfo)).isTrue();
  }

  @Test
  public void isCorePackage_withNonCorePackage() {
    PackageInfo packageInfo =
        PackageInfo.newBuilder()
            .setProtoPackage("my.custom.package")
            .setFhirVersion(FhirVersion.R4)
            .build();

    assertThat(FhirPackage.isCorePackage(packageInfo)).isFalse();
  }

  @Test
  public void isCorePackage_withLoadedCorePackage() throws IOException, InvalidFhirException {
    ImmutableList<PackageFile> files =
        ImmutableList.of(
            new PackageFile() {
              {
                fileName = "foo_package_info.prototxt";
                fileContents =
                    "proto_package: \"google.foo\""
                        + "\njava_proto_package: \"com.google.foo\""
                        + "\nfhir_version: R4"
                        + "\nlicense: APACHE"
                        + "\nlicense_date: \"2019\""
                        + "\nlocal_contained_resource: true"
                        + "\nfile_splitting_behavior: SPLIT_RESOURCES";
              }
            });
    String zipFile = createFhirPackageInfoZip("foo_package", files);
    FhirPackage fhirPackage = FhirPackage.load(zipFile);

    assertThat(fhirPackage.isCorePackage()).isFalse();
  }

  @Test
  public void isCorePackage_withLoadedNonCorePackage() throws IOException, InvalidFhirException {
    ImmutableList<PackageFile> files =
        ImmutableList.of(
            new PackageFile() {
              {
                fileName = "foo_package_info.textproto";
                fileContents =
                    "proto_package: \"google.fhir.r4.core\""
                        + "\njava_proto_package: \"com.google.fhir.r4.core\""
                        + "\nfhir_version: R4"
                        + "\nlicense: APACHE"
                        + "\nlicense_date: \"2019\""
                        + "\nlocal_contained_resource: true"
                        + "\nfile_splitting_behavior: SPLIT_RESOURCES";
              }
            });
    String zipFile = createFhirPackageInfoZip("foo_package", files);
    FhirPackage fhirPackage = FhirPackage.load(zipFile);

    assertThat(fhirPackage.isCorePackage()).isTrue();
  }

  @Test
  public void load_moreThanOnePackageInfo() throws IOException {
    ImmutableList<PackageFile> files =
        ImmutableList.of(
            new PackageFile() {
              {
                fileName = "foo_package_info.prototxt";
                fileContents =
                    "proto_package: \"google.foo\""
                        + "\njava_proto_package: \"com.google.foo\""
                        + "\nfhir_version: R4"
                        + "\nlicense: APACHE"
                        + "\nlicense_date: \"2019\""
                        + "\nlocal_contained_resource: true"
                        + "\nfile_splitting_behavior: SPLIT_RESOURCES";
              }
            },
            new PackageFile() {
              {
                fileName = "bar_package_info.prototxt";
                fileContents =
                    "proto_package: \"google.bar\""
                        + "\njava_proto_package: \"com.google.bar\""
                        + "\nfhir_version: R4"
                        + "\nlicense: APACHE"
                        + "\nlicense_date: \"2019\""
                        + "\nlocal_contained_resource: true"
                        + "\nfile_splitting_behavior: SPLIT_RESOURCES";
              }
            });
    String zipFile = createFhirPackageInfoZip("foo_package", files);

    IllegalArgumentException thrown =
        assertThrows(IllegalArgumentException.class, () -> FhirPackage.load(zipFile));

    assertThat(thrown)
        .hasMessageThat()
        .isEqualTo("Only one PackageInfo should be provided: " + zipFile);
  }

  @Test
  public void load_missingProtoPackageInPackageInfo() throws IOException {
    ImmutableList<PackageFile> files =
        ImmutableList.of(
            new PackageFile() {
              {
                fileName = "foo_package_info.prototxt";
                fileContents =
                    "java_proto_package: \"com.google.foo\""
                        + "\nfhir_version: R4"
                        + "\nlicense: APACHE"
                        + "\nlicense_date: \"2019\""
                        + "\nlocal_contained_resource: true"
                        + "\nfile_splitting_behavior: SPLIT_RESOURCES";
              }
            });
    String zipFile = createFhirPackageInfoZip("foo_package", files);

    IllegalArgumentException thrown =
        assertThrows(IllegalArgumentException.class, () -> FhirPackage.load(zipFile));

    assertThat(thrown)
        .hasMessageThat()
        .isEqualTo("When PackageInfo is provided, must specify `proto_package`.");
  }

  @Test
  public void load_unknownFhirVersion() throws IOException {
    ImmutableList<PackageFile> files =
        ImmutableList.of(
            new PackageFile() {
              {
                fileName = "foo_package_info.prototxt";
                fileContents =
                    "proto_package: \"google.foo\""
                        + "\njava_proto_package: \"com.google.foo\""
                        + "\nfhir_version: FHIR_VERSION_UNKNOWN"
                        + "\nlicense: APACHE"
                        + "\nlicense_date: \"2019\""
                        + "\nlocal_contained_resource: true"
                        + "\nfile_splitting_behavior: SPLIT_RESOURCES";
              }
            });
    String zipFile = createFhirPackageInfoZip("foo_package", files);

    IllegalArgumentException thrown =
        assertThrows(IllegalArgumentException.class, () -> FhirPackage.load(zipFile));

    assertThat(thrown)
        .hasMessageThat()
        .isEqualTo("When PackageInfo is provided, must specify `fhir_version`.");
  }

  @Test
  public void load_noPackageInfo() throws IOException, InvalidFhirException {

    String zipFile =
        createFhirPackageInfoZip(
            "foo_package",
            ImmutableList.of(
                new PackageFile() {
                  {
                    fileName = "bar.json";
                    fileContents =
                        "{\"resourceType\":\"StructureDefinition\", \"url\":\"http://bar.com\"}";
                  }
                }));

    FhirPackage fhirPackage = FhirPackage.load(zipFile);

    assertThat(fhirPackage.packageInfo).isNull();
    assertThat(fhirPackage.structureDefinitions()).hasSize(1);
    assertThat(fhirPackage.codeSystems()).isEmpty();
    assertThat(fhirPackage.valueSets()).isEmpty();
    assertThat(fhirPackage.searchParameters()).isEmpty();
  }

  private static final PackageFile VALID_PACKAGE_INFO =
      new PackageFile() {
        {
          fileName = "foo_package_info.prototxt";
          fileContents =
              "proto_package: \"google.foo\""
                  + "\njava_proto_package: \"com.google.foo\""
                  + "\nfhir_version: R4"
                  + "\nlicense: APACHE"
                  + "\nlicense_date: \"2019\""
                  + "\nlocal_contained_resource: true"
                  + "\nfile_splitting_behavior: SPLIT_RESOURCES";
        }
      };

  private enum LoadCase {
    UNDEFINED_RESOURCE(
        /*fileContents=*/ "{\"noResourceTypeField\":\"Foo\"}",
        /*structureDefinitionsCount=*/ 0,
        /*codeSystemsCount=*/ 0,
        /*valueSetsCount=*/ 0,
        /*searchParametersCount=*/ 0),
    UNHANDLED_RESOURCE_TYPE(
        /*fileContents=*/ "{\"resourceType\":\"Foo\", \"url\":\"http://foo.com\"}",
        /*structureDefinitionsCount=*/ 0,
        /*codeSystemsCount=*/ 0,
        /*valueSetsCount=*/ 0,
        /*searchParametersCount=*/ 0),
    VALUE_SET(
        /*fileContents=*/ "{\"resourceType\":\"ValueSet\", \"url\":\"http://foo.com\"}",
        /*structureDefinitionsCount=*/ 0,
        /*codeSystemsCount=*/ 0,
        /*valueSetsCount=*/ 1,
        /*searchParametersCount=*/ 0),
    CODE_SYSTEM(
        /*fileContents=*/ "{\"resourceType\":\"CodeSystem\", \"url\":\"http://foo.com\"}",
        /*structureDefinitionsCount=*/ 0,
        /*codeSystemsCount=*/ 1,
        /*valueSetsCount=*/ 0,
        /*searchParametersCount=*/ 0),
    STRUCTURE_DEFINITION(
        /*fileContents=*/ "{\"resourceType\":\"StructureDefinition\", \"url\":\"http://foo.com\"}",
        /*structureDefinitionsCount=*/ 1,
        /*codeSystemsCount=*/ 0,
        /*valueSetsCount=*/ 0,
        /*searchParametersCount=*/ 0),
    SEARCH_PARAMETER(
        /*fileContents=*/ "{\"resourceType\":\"SearchParameter\", \"url\":\"http://foo.com\"}",
        /*structureDefinitionsCount=*/ 0,
        /*codeSystemsCount=*/ 0,
        /*valueSetsCount=*/ 0,
        /*searchParametersCount=*/ 1),
    BUNDLE_WITH_EACH(
        /*fileContents=*/ "{\"resourceType\":\"Bundle\", \"entry\": [{\"resource\":"
            + " {\"resourceType\":\"ValueSet\", \"url\":\"http://foo.com/valueset\"}},"
            + " {\"resource\": {\"resourceType\":\"CodeSystem\""
            + ", \"url\":\"http://foo.com/codesystem\"}}, {\"resource\":"
            + " {\"resourceType\":\"StructureDefinition\""
            + ", \"url\":\"http://foo.com/strucdef\"}}, {\"resource\":"
            + " {\"resourceType\":\"SearchParameter\""
            + ", \"url\":\"http://foo.com/searchparam\"}}]}",
        /*structureDefinitionsCount=*/ 1,
        /*codeSystemsCount=*/ 1,
        /*valueSetsCount=*/ 1,
        /*searchParametersCount=*/ 1),
    BUNDLE_WITH_NO_ENTRIES(
        /*fileContents=*/ "{\"resourceType\":\"Bundle\"}",
        /*structureDefinitionsCount=*/ 0,
        /*codeSystemsCount=*/ 0,
        /*valueSetsCount=*/ 0,
        /*searchParametersCount=*/ 0),
    BUNDLE_WITH_INVALID_ENTRIES(
        /*fileContents=*/ "{\"resourceType\":\"Bundle\", \"entry\":\"Not an array\"}",
        /*structureDefinitionsCount=*/ 0,
        /*codeSystemsCount=*/ 0,
        /*valueSetsCount=*/ 0,
        /*searchParametersCount=*/ 0);

    final String fileContents;
    final int structureDefinitionsCount;
    final int codeSystemsCount;
    final int valueSetsCount;
    final int searchParametersCount;

    LoadCase(
        String fileContents,
        int structureDefinitionsCount,
        int codeSystemsCount,
        int valueSetsCount,
        int searchParametersCount) {
      this.fileContents = fileContents;
      this.structureDefinitionsCount = structureDefinitionsCount;
      this.codeSystemsCount = codeSystemsCount;
      this.valueSetsCount = valueSetsCount;
      this.searchParametersCount = searchParametersCount;
    }
  }

  @Test
  public void load_success(@TestParameter LoadCase loadCase)
      throws IOException, InvalidFhirException {
    ImmutableList<PackageFile> files =
        ImmutableList.of(
            VALID_PACKAGE_INFO,
            new PackageFile() {
              {
                fileName = "bar.json";
                fileContents = loadCase.fileContents;
              }
            });
    String zipFile = createFhirPackageInfoZip("foo_package", files);

    FhirPackage fhirPackage = FhirPackage.load(zipFile);

    assertThat(fhirPackage.packageInfo.getProtoPackage()).isEqualTo("google.foo");
    assertThat(fhirPackage.structureDefinitions()).hasSize(loadCase.structureDefinitionsCount);
    assertThat(fhirPackage.codeSystems()).hasSize(loadCase.codeSystemsCount);
    assertThat(fhirPackage.valueSets()).hasSize(loadCase.valueSetsCount);
    assertThat(fhirPackage.searchParameters()).hasSize(loadCase.searchParametersCount);
  }

  @Test
  public void load_withPackageInfo() throws IOException, InvalidFhirException {
    ImmutableList<PackageFile> files =
        ImmutableList.of(
            new PackageFile() {
              {
                fileName = "foo_package_info.prototxt";
                fileContents =
                    "proto_package: \"google.bar\""
                        + "\njava_proto_package: \"com.google.bar\""
                        + "\nfhir_version: R4"
                        + "\nlicense: APACHE"
                        + "\nlicense_date: \"2019\""
                        + "\nlocal_contained_resource: true"
                        + "\nfile_splitting_behavior: SPLIT_RESOURCES";
              }
            },
            new PackageFile() {
              {
                fileName = "bar.json";
                fileContents = "{\"resourceType\":\"ValueSet\", \"url\":\"http://foo.com\"}";
              }
            });
    String zipFile = createFhirPackageInfoZip("foo_package", files);

    PackageInfo packageInfo =
        PackageInfo.newBuilder()
            .setProtoPackage("google.foo")
            .setJavaProtoPackage("com.google.foo")
            .setFhirVersion(FhirVersion.R4)
            .setLicense(License.APACHE)
            .setLicenseDate("2019")
            .setLocalContainedResource(true)
            .setFileSplittingBehavior(FileSplittingBehavior.SPLIT_RESOURCES)
            .build();
    FhirPackage fhirPackage = FhirPackage.load(zipFile, packageInfo);

    // Note that it ignores the PackageInfo ("google.bar") provided in the ZIP.
    assertThat(fhirPackage.packageInfo.getProtoPackage()).isEqualTo("google.foo");
    assertThat(fhirPackage.structureDefinitions()).isEmpty();
    assertThat(fhirPackage.codeSystems()).isEmpty();
    assertThat(fhirPackage.valueSets()).hasSize(1);
    assertThat(fhirPackage.searchParameters()).isEmpty();
  }

  @Test
  public void load_withTarGzFile(@TestParameter({".tar.gz", ".tgz"}) String tarFileExtension)
      throws IOException, InvalidFhirException {
    File tarGzFile = File.createTempFile("foo", tarFileExtension);
    try (TarArchiveOutputStream tOut =
        new TarArchiveOutputStream(
            new GzipCompressorOutputStream(
                new BufferedOutputStream(
                    Channels.newOutputStream(
                        new FileOutputStream(tarGzFile.getAbsolutePath()).getChannel()))))) {
      TarArchiveEntry t = new TarArchiveEntry(new File("foo.json"));
      byte[] content =
          "{\"resourceType\":\"StructureDefinition\", \"url\":\"http://foo.com\"}"
              .getBytes(Charset.forName(UTF_8.name()));
      t.setSize(content.length);
      tOut.putArchiveEntry(t);
      tOut.write(content);
      tOut.closeArchiveEntry();
    }

    FhirPackage fhirPackage = FhirPackage.load(tarGzFile.getAbsolutePath());

    assertThat(fhirPackage.packageInfo).isNull();
    assertThat(fhirPackage.structureDefinitions()).hasSize(1);
    assertThat(fhirPackage.codeSystems()).isEmpty();
    assertThat(fhirPackage.valueSets()).isEmpty();
    assertThat(fhirPackage.searchParameters()).isEmpty();
  }

  @Test
  public void load_withUnsupportedFileType() throws IOException, InvalidFhirException {
    File unsupportedFile = File.createTempFile("foo", ".notsupported");

    IllegalArgumentException thrown =
        assertThrows(
            IllegalArgumentException.class,
            () -> FhirPackage.load(unsupportedFile.getAbsolutePath()));

    assertThat(thrown)
        .hasMessageThat()
        .isEqualTo(
            "`archiveFilePath` must end with '.zip', 'tar.gz' or '.tgz': "
                + unsupportedFile.getAbsolutePath());
  }

  @Test
  public void filter() throws IOException, InvalidFhirException {
    ImmutableList<PackageFile> files =
        ImmutableList.of(
            VALID_PACKAGE_INFO,
            new PackageFile() {
              {
                fileName = "foo.json";
                fileContents =
                    "{\"resourceType\":\"StructureDefinition\", \"url\":\"http://foo.com\","
                        + " \"id\":\"Foo\"}";
              }
            },
            new PackageFile() {
              {
                fileName = "bar.json";
                fileContents =
                    "{\"resourceType\":\"StructureDefinition\", \"url\":\"http://bar.com\","
                        + " \"id\":\"Bar\"}";
              }
            });
    String zipFile = createFhirPackageInfoZip("foo_package", files);

    FhirPackage fhirPackage = FhirPackage.load(zipFile);

    assertTrue(
        stream(fhirPackage.structureDefinitions().iterator())
            .anyMatch(def -> def.getId().getValue().equals("Foo")));
    FhirPackage filteredPackage =
        fhirPackage.filterResources(def -> !def.getId().getValue().equals("Foo"));
    assertFalse(
        stream(filteredPackage.structureDefinitions().iterator())
            .anyMatch(def -> def.getId().getValue().equals("Foo")));
    // Only "Bar" remains.
    assertThat(filteredPackage.structureDefinitions()).hasSize(1);
  }

  @Test
  public void getStructureDefinition_doesNotExist_emptyOptional()
      throws IOException, InvalidFhirException {
    ImmutableList<PackageFile> files =
        ImmutableList.of(
            VALID_PACKAGE_INFO,
            new PackageFile() {
              {
                fileName = "foo.json";
                fileContents =
                    "{\"resourceType\":\"StructureDefinition\", \"url\":\"http://foo.com\","
                        + " \"id\":\"Foo\"}";
              }
            });
    String zipFile = createFhirPackageInfoZip("foo_package", files);
    FhirPackage fhirPackage = FhirPackage.load(zipFile);

    assertThat(fhirPackage.getStructureDefinition("http://bar.com")).isEqualTo(Optional.empty());
  }

  @Test
  public void getStructureDefinition_exists() throws IOException, InvalidFhirException {
    ImmutableList<PackageFile> files =
        ImmutableList.of(
            VALID_PACKAGE_INFO,
            new PackageFile() {
              {
                fileName = "foo.json";
                fileContents =
                    "{\"resourceType\":\"StructureDefinition\", \"url\":\"http://foo.com\","
                        + " \"id\":\"Foo\"}";
              }
            });
    String zipFile = createFhirPackageInfoZip("foo_package", files);
    FhirPackage fhirPackage = FhirPackage.load(zipFile);

    assertThat(fhirPackage.getStructureDefinition("http://foo.com"))
        .isEqualTo(
            Optional.of(
                StructureDefinition.newBuilder()
                    .setUrl(Uri.newBuilder().setValue("http://foo.com"))
                    .setId(Id.newBuilder().setValue("Foo"))
                    .build()));
  }

  @Test
  public void getSearchParameter_doesNotExist_emptyOptional()
      throws IOException, InvalidFhirException {
    ImmutableList<PackageFile> files =
        ImmutableList.of(
            VALID_PACKAGE_INFO,
            new PackageFile() {
              {
                fileName = "foo.json";
                fileContents =
                    "{\"resourceType\":\"SearchParameter\", \"url\":\"http://foo.com\","
                        + " \"id\":\"Foo\"}";
              }
            });
    String zipFile = createFhirPackageInfoZip("foo_package", files);
    FhirPackage fhirPackage = FhirPackage.load(zipFile);

    assertThat(fhirPackage.getSearchParameter("http://bar.com")).isEqualTo(Optional.empty());
  }

  @Test
  public void getSearchParameter_exists() throws IOException, InvalidFhirException {
    ImmutableList<PackageFile> files =
        ImmutableList.of(
            VALID_PACKAGE_INFO,
            new PackageFile() {
              {
                fileName = "foo.json";
                fileContents =
                    "{\"resourceType\":\"SearchParameter\", \"url\":\"http://foo.com\","
                        + " \"id\":\"Foo\"}";
              }
            });
    String zipFile = createFhirPackageInfoZip("foo_package", files);
    FhirPackage fhirPackage = FhirPackage.load(zipFile);

    assertThat(fhirPackage.getSearchParameter("http://foo.com"))
        .isEqualTo(
            Optional.of(
                SearchParameter.newBuilder()
                    .setUrl(Uri.newBuilder().setValue("http://foo.com"))
                    .setId(Id.newBuilder().setValue("Foo"))
                    .build()));
  }

  @Test
  public void getCodeSystem_doesNotExist_emptyOptional() throws IOException, InvalidFhirException {
    ImmutableList<PackageFile> files =
        ImmutableList.of(
            VALID_PACKAGE_INFO,
            new PackageFile() {
              {
                fileName = "foo.json";
                fileContents =
                    "{\"resourceType\":\"CodeSystem\", \"url\":\"http://foo.com\","
                        + " \"id\":\"Foo\"}";
              }
            });
    String zipFile = createFhirPackageInfoZip("foo_package", files);
    FhirPackage fhirPackage = FhirPackage.load(zipFile);

    assertThat(fhirPackage.getCodeSystem("http://bar.com")).isEqualTo(Optional.empty());
  }

  @Test
  public void getCodeSystem_exists() throws IOException, InvalidFhirException {
    ImmutableList<PackageFile> files =
        ImmutableList.of(
            VALID_PACKAGE_INFO,
            new PackageFile() {
              {
                fileName = "foo.json";
                fileContents =
                    "{\"resourceType\":\"CodeSystem\", \"url\":\"http://foo.com\","
                        + " \"id\":\"Foo\"}";
              }
            });
    String zipFile = createFhirPackageInfoZip("foo_package", files);
    FhirPackage fhirPackage = FhirPackage.load(zipFile);

    assertThat(fhirPackage.getCodeSystem("http://foo.com"))
        .isEqualTo(
            Optional.of(
                CodeSystem.newBuilder()
                    .setUrl(Uri.newBuilder().setValue("http://foo.com"))
                    .setId(Id.newBuilder().setValue("Foo"))
                    .build()));
  }

  @Test
  public void getValueSet_doesNotExist_emptyOptional() throws IOException, InvalidFhirException {
    ImmutableList<PackageFile> files =
        ImmutableList.of(
            VALID_PACKAGE_INFO,
            new PackageFile() {
              {
                fileName = "foo.json";
                fileContents =
                    "{\"resourceType\":\"ValueSet\", \"url\":\"http://foo.com\","
                        + " \"id\":\"Foo\"}";
              }
            });
    String zipFile = createFhirPackageInfoZip("foo_package", files);
    FhirPackage fhirPackage = FhirPackage.load(zipFile);

    assertThat(fhirPackage.getValueSet("http://bar.com")).isEqualTo(Optional.empty());
  }

  @Test
  public void getValueSet_exists() throws IOException, InvalidFhirException {
    ImmutableList<PackageFile> files =
        ImmutableList.of(
            VALID_PACKAGE_INFO,
            new PackageFile() {
              {
                fileName = "foo.json";
                fileContents =
                    "{\"resourceType\":\"ValueSet\", \"url\":\"http://foo.com\","
                        + " \"id\":\"Foo\"}";
              }
            });
    String zipFile = createFhirPackageInfoZip("foo_package", files);
    FhirPackage fhirPackage = FhirPackage.load(zipFile);

    assertThat(fhirPackage.getValueSet("http://foo.com"))
        .isEqualTo(
            Optional.of(
                ValueSet.newBuilder()
                    .setUrl(Uri.newBuilder().setValue("http://foo.com"))
                    .setId(Id.newBuilder().setValue("Foo"))
                    .build()));
  }
}
