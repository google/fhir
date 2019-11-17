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

import com.google.api.client.json.gson.GsonFactory;
import com.google.api.services.bigquery.model.TableSchema;
import com.google.fhir.common.BigQuerySchema;
import com.google.fhir.common.JsonFormat;
import com.google.fhir.common.JsonFormat.Parser;
import com.google.fhir.common.JsonFormat.Printer;
import com.google.fhir.common.ResourceUtils;
import com.google.fhir.r4.core.Bundle;
import com.google.protobuf.Message;
import java.io.BufferedWriter;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.HashMap;
import java.util.Map;

/**
 * This example splits a set of FHIR bundles into individual resources, saved as ndjson files. The
 * first argument is assumed to be the target output directory, and each subsequent argument is an
 * input file.
 */
public class SplitBundleMain {

  public static void main(String[] args) throws IOException {
    Parser fhirParser = com.google.fhir.common.JsonFormat.getParser();
    Printer fhirPrinter = JsonFormat.getPrinter().omittingInsignificantWhitespace();
    Printer analyticPrinter =
        JsonFormat.getPrinter().omittingInsignificantWhitespace().forAnalytics();

    // Process the input files one by one, and count the number of processed resources.
    Map<String, Integer> counts = new HashMap<>();
    // We create one file per output resource type.
    Map<String, BufferedWriter> fhirOutput = new HashMap<>();
    Map<String, BufferedWriter> analyticOutput = new HashMap<>();
    // We create one schema per output resource type.
    Map<String, TableSchema> schema = new HashMap<>();

    String outputDir = args[0];
    for (int i = 1; i < args.length; i++) {
      String file = args[i];
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
        if (!fhirOutput.containsKey(resourceType)) {
          fhirOutput.put(
              resourceType,
              Files.newBufferedWriter(Paths.get(outputDir, resourceType + ".fhir.ndjson"), UTF_8));
          analyticOutput.put(
              resourceType,
              Files.newBufferedWriter(
                  Paths.get(outputDir, resourceType + ".analytic.ndjson"), UTF_8));
        }
        if (!schema.containsKey(resourceType)) {
          // Generate a schema for this type.
          schema.put(resourceType, BigQuerySchema.fromDescriptor(resource.getDescriptorForType()));
        }
        BufferedWriter resourceOutput = fhirOutput.get(resourceType);
        fhirPrinter.appendTo(resource, resourceOutput);
        resourceOutput.newLine();

        BufferedWriter analyticResourceOutput = analyticOutput.get(resourceType);
        analyticPrinter.appendTo(resource, analyticResourceOutput);
        analyticResourceOutput.newLine();
      }
    }
    for (BufferedWriter writer : fhirOutput.values()) {
      writer.close();
    }
    for (BufferedWriter writer : analyticOutput.values()) {
      writer.close();
    }
    // Write the schemas to disk.
    GsonFactory gsonFactory = new GsonFactory();
    for (String resourceType : schema.keySet()) {
      String filename = Paths.get(outputDir, resourceType + ".schema.json").toString();
      com.google.common.io.Files.asCharSink(new File(filename), StandardCharsets.UTF_8)
          .write(gsonFactory.toPrettyString(schema.get(resourceType).getFields()));
    }
    System.out.println("Processed " + args.length + " input files. Total number of resources:");
    for (Map.Entry<String, Integer> count : counts.entrySet()) {
      System.out.println(count.getKey() + ": " + count.getValue());
    }
  }
}
