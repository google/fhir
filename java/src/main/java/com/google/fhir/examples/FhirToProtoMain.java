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
import com.google.fhir.stu3.proto.ContainedResource;
import com.google.protobuf.Message;
import com.google.protobuf.util.JsonFormat.Printer;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;

/**
 * This example reads FHIR resources in standard ndjson format, and emits protobuf ndjson files,
 * suitable for loading into a database like BigQuery.
 */
public class FhirToProtoMain {

  public static void main(java.lang.String[] args) throws IOException {
    // Each argument is assumed to be an input file.
    Parser fhirParser = com.google.fhir.stu3.JsonFormat.getParser();
    Printer protoPrinter =
        com.google.protobuf.util.JsonFormat.printer().omittingInsignificantWhitespace();

    // Process the input files one by one, and count the number of processed resources.
    Map<String, Integer> counts = new HashMap<>();
    for (String file : args) {
      System.out.println("Processing " + file + "...");
      BufferedReader input = Files.newBufferedReader(Paths.get(file));
      BufferedWriter output = Files.newBufferedWriter(Paths.get(file + ".out"), UTF_8);
      for (String line = input.readLine(); line != null; line = input.readLine()) {
        // We parse as a ContainedResource, because we don't know what type of resource this is.
        ContainedResource.Builder builder = ContainedResource.newBuilder();
        fhirParser.merge(line, builder);
        // Extract and print the (one) parsed field.
        Collection<Object> values = builder.getAllFields().values();
        if (values.size() != 1) {
          System.out.println("Unexpected parse result for line: " + line);
        } else {
          Message parsed = (Message) values.iterator().next();
          protoPrinter.appendTo(parsed, output);
          output.newLine();
          // Count the number of parsed resources.
          String resourceType = parsed.getDescriptorForType().getName();
          int count = counts.containsKey(resourceType) ? counts.get(resourceType) : 0;
          counts.put(resourceType, count + 1);
        }
      }
      output.close();
    }
    System.out.println("Processed " + args.length + " input files. Total number of resources:");
    for (Map.Entry<String, Integer> count : counts.entrySet()) {
      System.out.println(count.getKey() + ": " + count.getValue());
    }
  }
}
