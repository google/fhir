//    Copyright 2019 Google Inc.
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

import com.google.common.base.Ascii;
import com.google.common.base.CaseFormat;
import com.google.common.base.Splitter;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/** Common Utilites for proto generation. */
// TODO: make this package-private once ProtoGeneratorMain is moved
public final class GeneratorUtils {

  private GeneratorUtils() {}

  private static final Pattern WORD_BREAK_PATTERN = Pattern.compile("[^A-Za-z0-9]+([A-Za-z0-9])");
  private static final Pattern ACRONYM_PATTERN = Pattern.compile("([A-Z])([A-Z]+)(?![a-z])");

  public static String toFieldNameCase(String fieldName) {
    // Make sure the field name is snake case, as required by the proto style guide.
    String normalizedFieldName =
        CaseFormat.UPPER_CAMEL.to(CaseFormat.LOWER_UNDERSCORE, resolveAcronyms(fieldName));
    // TODO: add more normalization here if necessary.  I think this is enough for now.
    return normalizedFieldName;
  }

  // Converts a FHIR id strings to UpperCamelCasing for FieldTypes using a regex pattern that
  // considers hyphen, underscore and space to be word breaks.
  public static String toFieldTypeCase(String type) {
    String normalizedType = type;
    if (Character.isLowerCase(type.charAt(0))) {
      normalizedType = Ascii.toUpperCase(type.substring(0, 1)) + type.substring(1);
    }
    Matcher matcher = WORD_BREAK_PATTERN.matcher(normalizedType);
    StringBuffer typeBuilder = new StringBuffer();
    boolean foundMatch = false;
    while (matcher.find()) {
      foundMatch = true;
      matcher.appendReplacement(typeBuilder, Ascii.toUpperCase(matcher.group(1)));
    }
    return foundMatch ? matcher.appendTail(typeBuilder).toString() : normalizedType;
  }

  public static String resolveAcronyms(String input) {
    // Turn acronyms into single words, e.g., FHIRIsGreat -> FhirIsGreat, so that it ultimately
    // becomes FhirIsGreat instead of f_h_i_r_is_great
    Matcher matcher = ACRONYM_PATTERN.matcher(input);
    StringBuffer sb = new StringBuffer();
    while (matcher.find()) {
      matcher.appendReplacement(sb, matcher.group(1) + Ascii.toLowerCase(matcher.group(2)));
    }
    matcher.appendTail(sb);
    return sb.toString();
  }

  /** Struct representing a proto type and package. */
  public static class QualifiedType {
    final String type;
    final String packageName;

    QualifiedType(String type, String packageName) {
      this.type = type;
      this.packageName = packageName;
    }

    String toQualifiedTypeString() {
      return "." + packageName + "." + type;
    }

    String getName() {
      return nameFromQualifiedName(toQualifiedTypeString());
    }

    QualifiedType childType(String name) {
      return new QualifiedType(type + "." + name, packageName);
    }
  }

  public static String nameFromQualifiedName(String qualifiedName) {
    List<String> messageNameParts = Splitter.on('.').splitToList(qualifiedName);
    return messageNameParts.get(messageNameParts.size() - 1);
  }
}
