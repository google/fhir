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
import com.google.fhir.common.JsonFormat.Parser;
import com.google.fhir.common.ResourceUtils;
import com.google.fhir.r4.core.ContainedResource;
import com.google.protobuf.Message;
import com.google.protobuf.util.JsonFormat;
import com.google.protobuf.util.JsonFormat.Printer;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.HashMap;
import java.util.Map;

/**
 * This example reads FHIR resources in standard ndjson format, containing one message per line, and
 * multiple messages per file. It emits protobuf ndjson files, suitable for loading into a database
 * like BigQuery. At a high level, the main difference between the input and output formats is that
 * proto-json can be mapped to a standard db schema unchanged, whereas FHIR json can not. For a more
 * detailed explanation of the differences, see the main FHIR protobuf documentation.
 */
public class ConvertNdJsonForBigQueryMain {

  public static void main(String[] argv) throws IOException {
    JsonParserArgs args = new JsonParserArgs(argv);
    Parser fhirParser = Parser.newBuilder().withDefaultTimeZone(args.getDefaultTimezone()).build();
    Printer protoPrinter = JsonFormat.printer().omittingInsignificantWhitespace();
    GsonFactory gsonFactory = new GsonFactory();

    // Process the input files one by one, and count the number of processed resources.
    Map<String, Integer> counts = new HashMap<>();
    for (JsonParserArgs.InputOutputFilePair entry : args.getInputOutputFilePairs()) {
      System.out.println("Processing " + entry.input + "...");
      BufferedReader input = Files.newBufferedReader(Paths.get(entry.input.toString()), UTF_8);
      BufferedWriter output = Files.newBufferedWriter(Paths.get(entry.output.toString()), UTF_8);
      TableSchema schema = null;
      for (String line = input.readLine(); line != null; line = input.readLine()) {
        // We parse as a ContainedResource, because we don't know what type of resource this is.
        ContainedResource.Builder builder = ContainedResource.newBuilder();
        fhirParser.merge(line, builder);
        // Extract and print the (one) parsed resource.
        Message parsed = ResourceUtils.getContainedResource(builder.build());
        if (schema == null) {
          // Generate a schema for this file. Note that we do this purely based on a single message,
          // which could potentially cause issues with extensions.
          schema = BigQuerySchema.fromDescriptor(parsed.getDescriptorForType());
        }
        protoPrinter.appendTo(parsed, output);
        output.newLine();
        // Count the number of parsed resources.
        String resourceType = parsed.getDescriptorForType().getName();
        int count = counts.containsKey(resourceType) ? counts.get(resourceType) : 0;
        counts.put(resourceType, count + 1);
      }
      output.close();
      if (schema != null) {
        String filename = Paths.get(entry.output.toString() + ".schema.json").toString();
        System.out.println("Writing schema to " + filename + "...");
        com.google.common.io.Files.asCharSink(new File(filename), StandardCharsets.UTF_8)
            .write(gsonFactory.toPrettyString(schema.getFields()));
      }
    }
    System.out.println(
        "Processed "
            + args.getInputOutputFilePairs().size()
            + " input files. Total number of resources:");
    for (Map.Entry<String, Integer> count : counts.entrySet()) {
      System.out.println(count.getKey() + ": " + count.getValue());
    }
  }
}
