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

import com.google.common.io.Files;
import com.google.fhir.common.JsonFormat.Parser;
import com.google.fhir.common.ResourceUtils;
import com.google.fhir.r4.core.ContainedResource;
import com.google.protobuf.Message;
import java.io.IOException;

/**
 * This example reads FHIR resources in json format, one message per file, and emits corresponding
 * .prototxt files. It is mainly used to generate testdata for JsonFormatTest.
 */
public class JsonToProtoMain {

  public static void main(String[] argv) throws IOException {
    JsonParserArgs args = new JsonParserArgs(argv);
    Parser fhirParser = Parser.newBuilder().withDefaultTimeZone(args.getDefaultTimezone()).build();

    // Process the input files one by one.
    for (JsonParserArgs.InputOutputFilePair entry : args.getInputOutputFilePairs()) {
      // We parse as a ContainedResource, because we don't know what type of resource this is.
      System.out.println("Processing " + entry.input + "...");
      ContainedResource.Builder builder = ContainedResource.newBuilder();
      String input = Files.asCharSource(entry.input, UTF_8).read();
      fhirParser.merge(input, builder);

      // Extract and print the parsed resource.
      Message parsed = ResourceUtils.getContainedResource(builder.build());
      Files.asCharSink(entry.output, UTF_8).write(parsed.toString());
    }
  }
}
