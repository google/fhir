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

import com.beust.jcommander.JCommander;
import com.beust.jcommander.Parameter;
import com.beust.jcommander.ParameterException;
import java.io.File;
import java.time.ZoneId;
import java.util.ArrayList;
import java.util.List;

/** Common arguments for examples parsing json-formatted FHIR resource inputs. */
public class JsonParserArgs {

  @Parameter(
    names = {"--input_filename_suffix"},
    description = "Suffix stripped from input filenames"
  )
  private String inputFilenameSuffix = ".json";

  @Parameter(
    names = {"--output_directory"},
    description = "Directory where generated output will be saved"
  )
  private String outputDirectory = ".";

  @Parameter(
    names = {"--output_filename_suffix"},
    description = "Suffix appended to output filenames"
  )
  private String outputFilenameSuffix = ".prototxt";

  @Parameter(
    names = {"--default_timezone"},
    description = "Defailt timezone for the json parser"
  )
  private String defaultTimezone = "Australia/Sydney";

  // Each non-flag argument is assumed to be an input file.
  @Parameter(description = "List of input files")
  private List<String> inputFiles = new ArrayList<>();

  /** Create a JsonParserArgs, initialized from command-line arguments. */
  public JsonParserArgs(String[] argv) {
    JCommander jcommander = new JCommander(this);
    try {
      jcommander.parse(argv);
    } catch (ParameterException exception) {
      System.err.printf("Invalid usage: %s\n", exception.getMessage());
      jcommander.usage();
      System.exit(1);
    }
  }

  static class InputOutputFilePair {
    public final File input;
    public final File output;

    public InputOutputFilePair(File input, File output) {
      this.input = input;
      this.output = output;
    }
  }

  /** Return list of input filenames, along with the corresponding desired output filenames. */
  public List<InputOutputFilePair> getInputOutputFilePairs() {
    List<InputOutputFilePair> values = new ArrayList<>();
    for (String file : inputFiles) {
      File inputFile = new File(file);
      String outputFilename = inputFile.getName();
      if (outputFilename.endsWith(inputFilenameSuffix)) {
        outputFilename =
            outputFilename.substring(0, outputFilename.lastIndexOf(inputFilenameSuffix));
      }
      outputFilename = outputFilename + outputFilenameSuffix;
      File outputFile = new File(outputDirectory, outputFilename);
      values.add(new InputOutputFilePair(inputFile, outputFile));
    }
    return values;
  }

  public ZoneId getDefaultTimezone() {
    return ZoneId.of(defaultTimezone);
  }
}
