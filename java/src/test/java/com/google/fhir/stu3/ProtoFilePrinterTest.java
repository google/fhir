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

package com.google.fhir.stu3;

import static com.google.common.truth.Truth.assertWithMessage;

import com.google.common.base.CaseFormat;
import com.google.common.base.Splitter;
import com.google.common.collect.ImmutableSet;
import com.google.common.collect.Iterators;
import com.google.common.collect.PeekingIterator;
import com.google.common.io.Files;
import com.google.devtools.build.runfiles.Runfiles;
import com.google.fhir.stu3.proto.Annotations;
import com.google.fhir.stu3.proto.ContactDetail;
import com.google.fhir.stu3.proto.ContainedResource;
import com.google.fhir.stu3.proto.Extension;
import com.google.fhir.stu3.proto.StructureDefinition;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Descriptors.FileDescriptor;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.SortedMap;
import java.util.TreeMap;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit tests for {@link ProtoFilePrinter}. */
@RunWith(JUnit4.class)
public final class ProtoFilePrinterTest {

  private JsonFormat.Parser jsonParser;
  private ProtoGenerator protoGenerator;
  private ProtoFilePrinter protoPrinter;
  private Runfiles runfiles;

  /** Read and parse the specified StructureDefinition. */
  private StructureDefinition readProfile(String relativePath) throws IOException {
    File file =
        new File(runfiles.rlocation("com_google_fhir/testdata/stu3/" + relativePath));
    String json = Files.asCharSource(file, StandardCharsets.UTF_8).read();
    StructureDefinition.Builder builder = StructureDefinition.newBuilder();
    jsonParser.merge(json, builder);
    return builder.build();
  }

  /**
   * Read the expected golden output for a specific message, either from the .proto file, or from a
   * file in the testdata directory.
   */
  private String readGolden(String messageName) throws IOException {
    String filename = CaseFormat.UPPER_CAMEL.to(CaseFormat.LOWER_UNDERSCORE, messageName);

    // Use the actual .proto file as golden.
    File file =
        new File(runfiles.rlocation("com_google_fhir/proto/stu3/" + filename + ".proto"));
    return Files.asCharSource(file, StandardCharsets.UTF_8).read();
  }

  /** Collapse comments spread across multiple lines into single lines. */
  private SortedMap<java.lang.Integer, String> collapseComments(
      SortedMap<java.lang.Integer, String> input) {
    TreeMap<java.lang.Integer, String> result = new TreeMap<>();
    PeekingIterator<Map.Entry<java.lang.Integer, String>> iter =
        Iterators.peekingIterator(input.entrySet().iterator());
    while (iter.hasNext()) {
      Map.Entry<java.lang.Integer, String> current = iter.next();
      result.put(current.getKey(), current.getValue().trim());
      if (current.getValue().trim().startsWith("//")) {
        while (iter.hasNext() && iter.peek().getValue().trim().startsWith("//")) {
          // Merge.
          result.put(
              current.getKey(),
              result.get(current.getKey()) + iter.next().getValue().trim().substring(2));
        }
      }
    }
    return result;
  }

  /**
   * Collapse statements spread across multiple lines into single lines. These statements are
   * typically field definitions, along with annotations, and may be well over the maximum line
   * length allowed by the style guide.
   */
  private SortedMap<java.lang.Integer, String> collapseStatements(
      SortedMap<java.lang.Integer, String> input) {
    TreeMap<java.lang.Integer, String> result = new TreeMap<>();
    Iterator<Map.Entry<java.lang.Integer, String>> iter = input.entrySet().iterator();
    while (iter.hasNext()) {
      Map.Entry<java.lang.Integer, String> current = iter.next();
      String value = current.getValue();
      if (!value.trim().isEmpty() && !value.trim().startsWith("//")) {
        // Merge until we see the closing ';'
        while (!value.endsWith(";") && iter.hasNext()) {
          String next = iter.next().getValue().trim();
          if (!next.isEmpty()) {
            value = value + (value.endsWith("[") || next.startsWith("]") ? "" : " ") + next;
          }
        }
      }
      if (value.contains("End of auto-generated messages.")) {
        return result;
      }
      if (!value.isEmpty()) {
        result.put(current.getKey(), value);
      }
    }
    return result;
  }

  private SortedMap<java.lang.Integer, String> splitIntoLines(String text) {
    TreeMap<java.lang.Integer, String> result = new TreeMap<>();
    for (String line : Splitter.on('\n').split(text)) {
      result.put(result.size() + 1, line);
    }
    return result;
  }

  /**
   * Compare two .proto files, line by line, ignoring differences that may have been caused by
   * clang-format.
   */
  private void assertEqualsIgnoreClangFormat(String golden, String test) {
    Iterator<Map.Entry<java.lang.Integer, String>> goldenIter =
        collapseStatements(collapseComments(splitIntoLines(golden))).entrySet().iterator();
    Iterator<Map.Entry<java.lang.Integer, String>> testIter =
        collapseStatements(collapseComments(splitIntoLines(test))).entrySet().iterator();
    while (goldenIter.hasNext() && testIter.hasNext()) {
      Map.Entry<java.lang.Integer, String> goldenEntry = goldenIter.next();
      Map.Entry<java.lang.Integer, String> testEntry = testIter.next();
      assertWithMessage(
              "Test line "
                  + testEntry.getKey()
                  + " does not match golden line "
                  + goldenEntry.getKey())
          .that(testEntry.getValue())
          .isEqualTo(goldenEntry.getValue());
    }
  }

  private static final ImmutableSet<String> TYPES_TO_IGNORE =
      ImmutableSet.of("Extension", "Reference", "ReferenceId");

  private List<StructureDefinition> getResourcesInFile(FileDescriptor compiled) throws IOException {
    List<StructureDefinition> resourceDefinitions = new ArrayList<>();
    for (Descriptor message : compiled.getMessageTypes()) {
      if (!TYPES_TO_IGNORE.contains(message.getName())
          && !message.getOptions().hasExtension(Annotations.fhirValuesetUrl)) {
        String relativePath =
            "structure_definitions/" + message.getName().toLowerCase() + ".profile.json";
        resourceDefinitions.add(readProfile(relativePath));
      }
    }
    return resourceDefinitions;
  }

  @Before
  public void setUp() throws IOException {
    String packageName = "google.fhir.stu3.proto";
    jsonParser = JsonFormat.getParser();
    runfiles = Runfiles.create();
    protoGenerator = new ProtoGenerator(packageName, "proto/stu3");
    protoPrinter = new ProtoFilePrinter().withApacheLicense();
  }

  // TODO(sundberg): Test the FHIR code types.

  /** Test generating datatypes.proto. */
  @Test
  public void generateDataTypes() throws Exception {
    List<StructureDefinition> resourceDefinitions =
        getResourcesInFile(Extension.getDescriptor().getFile());
    FileDescriptorProto descriptor = protoGenerator.generateFileDescriptor(resourceDefinitions);
    String generated = protoPrinter.print(descriptor);
    String golden = readGolden("datatypes");
    assertEqualsIgnoreClangFormat(golden, generated);
  }

  /** Test generating metadatatypes.proto. */
  @Test
  public void generateMetadataTypes() throws Exception {
    List<StructureDefinition> resourceDefinitions =
        getResourcesInFile(ContactDetail.getDescriptor().getFile());
    FileDescriptorProto descriptor = protoGenerator.generateFileDescriptor(resourceDefinitions);
    String generated = protoPrinter.print(descriptor);
    String golden = readGolden("metadatatypes");
    assertEqualsIgnoreClangFormat(golden, generated);
  }

  /** Test generating resources.proto. */
  @Test
  public void generateResources() throws Exception {
    List<StructureDefinition> resourceDefinitions = new ArrayList<>();
    for (FieldDescriptor resource : ContainedResource.getDescriptor().getFields()) {
      String relativePath =
          "structure_definitions/"
              + resource.getMessageType().getName().toLowerCase()
              + ".profile.json";
      resourceDefinitions.add(readProfile(relativePath));
    }
    FileDescriptorProto descriptor = protoGenerator.generateFileDescriptor(resourceDefinitions);
    descriptor = protoGenerator.addContainedResource(descriptor);
    descriptor =
        descriptor
            .toBuilder()
            .addDependency("proto/stu3/metadatatypes.proto")
            .build();
    String generated = protoPrinter.print(descriptor);
    String golden = readGolden("resources");
    assertEqualsIgnoreClangFormat(golden, generated);
  }

  // Test generating a few of the extension protos.

  /** Test generating elementdefinition_binding_name.proto. */
  @Test
  public void generateElementDefinitionExtensions() throws Exception {
    String[] extensionNames = {
      "extension-elementdefinition-bindingname",
      "extension-structuredefinition-explicit-type-name",
      "extension-structuredefinition-regex"
    };
    List<StructureDefinition> extensionDefinitions = new ArrayList<>();
    for (String extensionName : extensionNames) {
      extensionDefinitions.add(readProfile("extensions/" + extensionName + ".json"));
    }
    FileDescriptorProto descriptor = protoGenerator.generateFileDescriptor(extensionDefinitions);
    String generated = protoPrinter.print(descriptor);
    String golden = readGolden("extensions");
    assertEqualsIgnoreClangFormat(golden, generated);
  }
}
