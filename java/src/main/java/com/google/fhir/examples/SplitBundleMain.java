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

package com.google.fhir.examples;

import static java.nio.charset.StandardCharsets.UTF_8;

import com.google.fhir.stu3.JsonFormat.Parser;
import com.google.fhir.stu3.ResourceUtils;
import com.google.fhir.stu3.proto.Bundle;
import com.google.protobuf.Message;
import com.google.protobuf.util.JsonFormat.Printer;
import java.io.BufferedWriter;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.HashMap;
import java.util.Map;

/**
 * This example splits a set of FHIR bundles into individual resources, saved as ndjson files. Each
 * argument is assumed to be an input file.
 */
public class SplitBundleMain {

  public static void main(java.lang.String[] args) throws IOException {
    Parser fhirParser = com.google.fhir.stu3.JsonFormat.getParser();
    Printer protoPrinter =
        com.google.protobuf.util.JsonFormat.printer().omittingInsignificantWhitespace();

    // Process the input files one by one, and count the number of processed resources.
    Map<String, Integer> counts = new HashMap<>();
    // We create one file per output resource type.
    Map<String, BufferedWriter> output = new HashMap<>();
    for (String file : args) {
      System.out.println("Processing " + file + "...");
      String input = new String(Files.readAllBytes(Paths.get(file)), UTF_8);

      // Parse the input bundle.
      Bundle.Builder builder = Bundle.newBuilder();
      fhirParser.merge(input, builder);

      // Some FHIR implementations use absolute urls for references, such as urn:uuid:<identifier>,
      // we'd like to resolve them to for example Patient/<identifier> instead. Here we do it in an
      // ad-hoc way, creating a map of full url to relative reference, and then apply that mapping
      // directly to the input string. It's fragile and slow, but enough for an example application.
      // For more details on resolving references in bundles, see
      // https://www.hl7.org/fhir/bundle.html#references
      Bundle bundle = ResourceUtils.resolveBundleReferences(builder.build());

      // Split the bundle.
      for (Bundle.Entry entry : bundle.getEntryList()) {
        Message resource = ResourceUtils.getContainedResource(entry.getResource());
        String resourceType = ResourceUtils.getResourceType(resource);
        int count = counts.containsKey(resourceType) ? counts.get(resourceType) : 0;
        counts.put(resourceType, count + 1);
        if (!output.containsKey(resourceType)) {
          output.put(
              resourceType, Files.newBufferedWriter(Paths.get(resourceType + ".ndjson"), UTF_8));
        }
        BufferedWriter resourceOutput = output.get(resourceType);
        protoPrinter.appendTo(resource, resourceOutput);
        resourceOutput.newLine();
      }
    }
    for (BufferedWriter writer : output.values()) {
      writer.close();
    }
    System.out.println("Processed " + args.length + " input files. Total number of resources:");
    for (Map.Entry<String, Integer> count : counts.entrySet()) {
      System.out.println(count.getKey() + ": " + count.getValue());
    }
  }
}
