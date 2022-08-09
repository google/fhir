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

package com.google.fhir.protogen;


import com.google.common.base.Ascii;
import com.google.common.base.CharMatcher;
import com.google.common.base.Joiner;
import com.google.common.base.Splitter;
import com.google.common.base.Strings;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.Iterables;
import com.google.common.collect.Lists;
import com.google.common.escape.CharEscaperBuilder;
import com.google.common.escape.Escaper;
import com.google.fhir.proto.Annotations;
import com.google.fhir.proto.PackageInfo;
import com.google.fhir.proto.ProtoGeneratorAnnotations;
import com.google.protobuf.DescriptorProtos.DescriptorProto;
import com.google.protobuf.DescriptorProtos.EnumDescriptorProto;
import com.google.protobuf.DescriptorProtos.EnumOptions;
import com.google.protobuf.DescriptorProtos.EnumValueDescriptorProto;
import com.google.protobuf.DescriptorProtos.EnumValueOptions;
import com.google.protobuf.DescriptorProtos.FieldDescriptorProto;
import com.google.protobuf.DescriptorProtos.FieldOptions;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import com.google.protobuf.DescriptorProtos.FileOptions;
import com.google.protobuf.DescriptorProtos.MessageOptions;
import com.google.protobuf.Descriptors.FieldDescriptor;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Comparator;
import java.util.GregorianCalendar;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/** A utility to turn protocol message descriptors into .proto files. */
public class ProtoFilePrinter {

  /** Enum represting the proto Syntax */
  public static enum Syntax {
    PROTO2,
    PROTO3
  };

  private static final String APACHE_LICENSE =
      "//    Copyright %1$s Google Inc.\n"
          + "//\n"
          + "//    Licensed under the Apache License, Version 2.0 (the \"License\");\n"
          + "//    you may not use this file except in compliance with the License.\n"
          + "//    You may obtain a copy of the License at\n"
          + "//\n"
          + "//        https://www.apache.org/licenses/LICENSE-2.0\n"
          + "//\n"
          + "//    Unless required by applicable law or agreed to in writing, software\n"
          + "//    distributed under the License is distributed on an \"AS IS\" BASIS,\n"
          + "//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n"
          + "//    See the License for the specific language governing permissions and\n"
          + "//    limitations under the License.\n";

  // The proto package that contains the annotation definitions for proto options.
  static final String ANNOTATION_PACKAGE = "google.fhir.proto";

  private final PackageInfo packageInfo;

  private final Syntax syntax;

  // All Message options in this list will appear in this order, before any options not in this
  // list.  The remainder will be alphabetized.
  // This list exists largely to maintain backwards compatibility in ordering.
  private static final ImmutableList<String> LEGACY_MESSAGE_OPTIONS_ORDERING =
      ImmutableList.of(
          "google.fhir.proto.structure_definition_kind",
          "google.fhir.proto.is_abstract_type",
          "google.fhir.proto.value_regex",
          "google.fhir.proto.fhir_valueset_url",
          "google.fhir.proto.fhir_profile_base",
          "google.fhir.proto.fhir_structure_definition_url");

  // All Field options in this list will appear in this order, before any options not in this
  // list.  The remainder will be alphabetized.
  // This list exists largely to maintain backwards compatibility in ordering.
  private static final ImmutableList<String> LEGACY_FIELD_OPTIONS_ORDERING =
      ImmutableList.of(
          "google.fhir.proto.validation_requirement",
          "google.fhir.proto.fhir_inlined_extension_url",
          "google.fhir.proto.fhir_inlined_coding_system",
          "google.fhir.proto.fhir_inlined_coding_code",
          "google.fhir.proto.valid_reference_type",
          "google.fhir.proto.fhir_path_constraint");

  /** Creates a ProtoFilePrinter with default parameters. */
  public ProtoFilePrinter(PackageInfo packageInfo) {
    this(packageInfo, Syntax.PROTO3);
  }

  /** Creates a ProtoFilePrinter with default parameters. */
  public ProtoFilePrinter(PackageInfo packageInfo, Syntax syntax) {
    this.packageInfo = packageInfo;
    this.syntax = syntax;
  }

  /** Generate a .proto file corresponding to the provided FileDescriptorProto. */
  public String print(FileDescriptorProto fileDescriptor) {
    String fullyQualifiedPackageName = "." + fileDescriptor.getPackage();
    StringBuilder contents = new StringBuilder();
    if (packageInfo.getLicense() == PackageInfo.License.APACHE) {
      String licenseDate =
          packageInfo.getLicenseDate().isEmpty()
              ? ("" + new GregorianCalendar().get(Calendar.YEAR))
              : packageInfo.getLicenseDate();
      contents.append(String.format(APACHE_LICENSE, licenseDate)).append("\n");
    }
    contents.append(printHeader(fileDescriptor)).append("\n");
    contents.append(printImports(fileDescriptor)).append("\n");
    contents.append(printOptions(fileDescriptor, fullyQualifiedPackageName)).append("\n");
    for (DescriptorProto descriptor :
        ImmutableList.sortedCopyOf(
            Comparator.comparing(DescriptorProto::getName), fileDescriptor.getMessageTypeList())) {
      contents
          .append(printMessage(descriptor, fullyQualifiedPackageName, fullyQualifiedPackageName))
          .append("\n");
    }
    return contents.toString();
  }

  private String printHeader(FileDescriptorProto fileDescriptor) {
    StringBuilder header = new StringBuilder();
    if (fileDescriptor.hasSyntax()) {
      header.append("syntax = \"").append(fileDescriptor.getSyntax()).append("\";\n\n");
    }
    if (fileDescriptor.hasPackage()) {
      header.append("package ").append(fileDescriptor.getPackage()).append(";\n");
    }
    return header.toString();
  }

  private String printImports(FileDescriptorProto fileDescriptor) {
    final StringBuilder imports = new StringBuilder();
    fileDescriptor.getDependencyList().stream()
        .sorted()
        .forEach(dependency -> imports.append("import \"").append(dependency).append("\";\n"));
    return imports.toString();
  }

  private String printOptions(FileDescriptorProto fileDescriptor, String packageName) {
    StringBuilder options = new StringBuilder();
    FileOptions fileOptions = fileDescriptor.getOptions();
    if (fileOptions.hasJavaMultipleFiles()) {
      options
          .append("option java_multiple_files = ")
          .append(fileOptions.getJavaMultipleFiles())
          .append(";\n");
    }
    if (fileOptions.hasJavaPackage()) {
      options
          .append("option java_package = \"")
          .append(fileOptions.getJavaPackage())
          .append("\";\n");
    }
    if (fileOptions.hasExtension(Annotations.fhirVersion)) {
      options
          .append("option (")
          .append(getOptionsPackage(packageName))
          .append("fhir_version) = ")
          .append(fileOptions.getExtension(Annotations.fhirVersion))
          .append(";\n");
    }
    return options.toString();
  }

  private String printMessage(DescriptorProto descriptor, String typePrefix, String packageName) {
    // Get the name of this message.
    String messageName = descriptor.getName();
    String fullName = typePrefix + "." + messageName;

    // Use mutable options so we can clear {@link ProtoGeneratorAnnotation}s along the way.
    MessageOptions.Builder optionsBuilder = descriptor.getOptions().toBuilder();

    CharMatcher matcher = CharMatcher.is('.');
    String indent =
        Strings.repeat("  ", matcher.countIn(typePrefix) - matcher.countIn(packageName));
    StringBuilder message = new StringBuilder();

    if (optionsBuilder.hasExtension(ProtoGeneratorAnnotations.messageDescription)) {
      // Add the main documentation.
      message
          .append(indent)
          .append("// ")
          .append(
              optionsBuilder
                  .getExtension(ProtoGeneratorAnnotations.messageDescription)
                  .replaceAll("[\\n\\r]", "\n" + indent + "// "))
          .append("\n");
      optionsBuilder.clearExtension(ProtoGeneratorAnnotations.messageDescription);
    }

    // Start the main message.
    message.append(indent).append("message ").append(messageName).append(" {\n");

    String fieldIndent = indent + "  ";

    // Add options.
    List<String> extensions =
        addExtensions(
            "." + MessageOptions.getDescriptor().getFullName(),
            optionsBuilder.build().getAllFields(),
            packageName,
            fieldIndent,
            LEGACY_MESSAGE_OPTIONS_ORDERING);
    if (!extensions.isEmpty()) {
      Joiner.on(";\n").appendTo(message, extensions).append(";\n");
    }

    // Loop over the elements.
    Set<String> printedNestedTypeDefinitions = new HashSet<>();
    for (int i = 0; i < descriptor.getFieldCount(); i++) {
      FieldDescriptorProto field = descriptor.getField(i);
      // Use a mutable field in order to allow clearing protogenerator-only annotations.
      FieldDescriptorProto.Builder fieldBuilder = field.toBuilder();
      if (!field.hasOneofIndex()) {
        if (field.getOptions().hasExtension(ProtoGeneratorAnnotations.reservedReason)) {
          message
              .append(fieldIndent)
              .append("// ")
              .append(field.getOptions().getExtension(ProtoGeneratorAnnotations.reservedReason))
              .append("\n")
              .append(fieldIndent)
              .append("reserved ")
              .append(field.getNumber())
              .append(";\n\n");
          continue;
        }

        if (field.getOptions().hasExtension(ProtoGeneratorAnnotations.fieldDescription)) {
          // Add a comment describing the field.
          String description =
              field.getOptions().getExtension(ProtoGeneratorAnnotations.fieldDescription);
          message
              .append(fieldIndent)
              .append("// ")
              .append(description.replaceAll("[\\n\\r]", "\n" + indent + "// "))
              .append("\n");
          fieldBuilder
              .getOptionsBuilder()
              .clearExtension(ProtoGeneratorAnnotations.fieldDescription);
        }

        // Add nested types if necessary.
        message.append(
            maybePrintNestedType(
                descriptor, field, typePrefix, packageName, printedNestedTypeDefinitions));
        message.append(
            printField(
                fieldBuilder.build(), fullName, fieldIndent, packageName, /*inOneof=*/ false));
        if (i != descriptor.getFieldCount() - 1) {
          message.append("\n");
        }
      }
    }

    // Loop over the oneofs
    String oneofIndent = fieldIndent + "  ";
    for (int oneofIndex = 0; oneofIndex < descriptor.getOneofDeclCount(); oneofIndex++) {
      message
          .append(fieldIndent)
          .append("oneof ")
          .append(descriptor.getOneofDecl(oneofIndex).getName())
          .append(" {\n");
      // Loop over the elements.
      for (FieldDescriptorProto field : descriptor.getFieldList()) {
        if (field.getOneofIndex() == oneofIndex) {
          message.append(printField(field, fullName, oneofIndent, packageName, /*inOneof=*/ true));
        }
      }
      message.append(fieldIndent).append("}\n");
    }

    // Print any enums that weren't already printed (i.e., enums that aren't used by any fields)
    for (EnumDescriptorProto enumProto : descriptor.getEnumTypeList()) {
      String fullTypeName = fullName + "." + enumProto.getName();
      if (printedNestedTypeDefinitions.add(fullTypeName)) {
        message.append(printEnum(enumProto, fullTypeName, packageName));
      }
    }

    // Close the main message.
    message.append(indent).append("}\n");

    return message.toString();
  }

  private String printEnum(EnumDescriptorProto descriptor, String typePrefix, String packageName) {
    // Get the name of this message.
    String messageName = descriptor.getName();

    CharMatcher matcher = CharMatcher.is('.');
    String indent =
        Strings.repeat("  ", matcher.countIn(typePrefix) - matcher.countIn(packageName));
    StringBuilder message = new StringBuilder();

    // Start the enum.
    message.append(indent).append("enum ").append(messageName).append(" {\n");

    String fieldIndent = indent + "  ";

    List<String> enumExtensionStrings =
        addExtensions(
            "." + EnumOptions.getDescriptor().getFullName(),
            descriptor.getOptions().getAllFields(),
            packageName,
            fieldIndent,
            ImmutableList.of());
    if (!enumExtensionStrings.isEmpty()) {
      Joiner.on(";\n").appendTo(message, enumExtensionStrings).append(";\n\n");
    }

    // Loop over the elements.
    for (EnumValueDescriptorProto enumValue : descriptor.getValueList()) {
      message
          .append(fieldIndent)
          .append(enumValue.getName())
          .append(" = ")
          .append(enumValue.getNumber());

      List<String> enumValueExtensionStrings =
          addExtensions(
              "." + EnumValueOptions.getDescriptor().getFullName(),
              enumValue.getOptions().getAllFields(),
              packageName,
              "",
              ImmutableList.of());
      if (!enumValueExtensionStrings.isEmpty()) {
        message.append(" [").append(Joiner.on(", ").join(enumValueExtensionStrings)).append("]");
      }

      message.append(";\n");
    }

    // Close the enum.
    message.append(indent).append("}\n");

    return message.toString();
  }

  private String maybePrintNestedType(
      DescriptorProto descriptor,
      FieldDescriptorProto field,
      String typePrefix,
      String packageName,
      Set<String> printedNestedTypeDefinitions) {
    String prefix = typePrefix + "." + descriptor.getName();
    if (field.hasTypeName()
        && field.getTypeName().startsWith(prefix + ".")
        && !printedNestedTypeDefinitions.contains(field.getTypeName())) {
      List<String> typeNameParts = Splitter.on('.').splitToList(field.getTypeName());
      String typeName = Iterables.getLast(typeNameParts);
      if (field.getType() == FieldDescriptorProto.Type.TYPE_MESSAGE) {
        for (DescriptorProto nested : descriptor.getNestedTypeList()) {
          if (nested.getName().equals(typeName)) {
            printedNestedTypeDefinitions.add(field.getTypeName());
            return printMessage(nested, prefix, packageName);
          }
        }
      } else if (field.getType() == FieldDescriptorProto.Type.TYPE_ENUM) {
        for (EnumDescriptorProto nested : descriptor.getEnumTypeList()) {
          if (nested.getName().equals(typeName)) {
            printedNestedTypeDefinitions.add(field.getTypeName());
            return printEnum(nested, prefix, packageName);
          }
        }
      }
    }
    // The type wasn't defined in this scope, print nothing.
    return "";
  }

  private String printField(
      FieldDescriptorProto field,
      String containingType,
      String indent,
      String packageName,
      boolean inOneof) {
    StringBuilder message = new StringBuilder();
    message.append(indent);

    // Add the "repeated" or "optional" keywords, if necessary.
    if (field.getLabel() == FieldDescriptorProto.Label.LABEL_REPEATED) {
      message.append("repeated ");
    }
    if (!inOneof
        && field.getLabel() == FieldDescriptorProto.Label.LABEL_OPTIONAL
        && syntax == Syntax.PROTO2) {
      message.append("optional ");
    }

    // Add the type of the field.
    if ((field.getType() == FieldDescriptorProto.Type.TYPE_MESSAGE
            || field.getType() == FieldDescriptorProto.Type.TYPE_ENUM)
        && field.hasTypeName()) {
      List<String> typeNameParts = Splitter.on('.').splitToList(field.getTypeName());
      List<String> containingTypeParts = Splitter.on('.').splitToList(containingType);
      int numCommon = 0;
      while (numCommon < typeNameParts.size()
          && numCommon < containingTypeParts.size()
          && typeNameParts.get(numCommon).equals(containingTypeParts.get(numCommon))) {
        numCommon++;
      }

      // never drop the last token.
      // E.g., foo.baz.Quux on foo.baz.Quux (recursively) shortens to Quux.
      int tokensToDrop = numCommon == typeNameParts.size() ? numCommon - 1 : numCommon;

      // Make sure the first token that is not dropped in the shortened name is not a token
      // somewhere else in non-common part of the containing type.
      // E.g., if "abc.foo.baz.Quux" is a type used in "abc.foo.bar.baz.Bleh",
      // and we try to shorten it to baz.Quux, it will look for Quux on abc.foor.bar.baz,
      // and will error. In this case, just print the fully-qualified name.
      if (containingTypeParts
          .subList(numCommon, containingTypeParts.size())
          .contains(typeNameParts.get(tokensToDrop))) {
        tokensToDrop = 0;
      }

      // Since absolute namespaces start with ".", the first token is is empty (and thus common).
      // If this is the only common token, don't drop anything
      if (tokensToDrop > 1) {
        Joiner.on('.').appendTo(message, typeNameParts.subList(tokensToDrop, typeNameParts.size()));
      } else {
        message.append(field.getTypeName());
      }
    } else if (field.getType().toString().startsWith("TYPE_")) {
      message.append(Ascii.toLowerCase(field.getType().toString().substring(5)));
    } else {
      message.append("INVALID_TYPE");
    }

    // Add the name and field number.
    message.append(" ").append(field.getName()).append(" = ").append(field.getNumber());

    FieldOptions options = field.getOptions();

    List<String> extensionStrings =
        addExtensions(
            "." + FieldOptions.getDescriptor().getFullName(),
            options.getAllFields(),
            packageName,
            "",
            LEGACY_FIELD_OPTIONS_ORDERING);

    if (field.hasJsonName()) {
      int insertIndex = 0;
      // For legacy reasons, json_name appears before fhir_path_constraint.
      for (insertIndex = 0;
          insertIndex < extensionStrings.size()
              && !extensionStrings.get(insertIndex).contains("fhir_path_constraint");
          insertIndex++) {}

      extensionStrings.add(insertIndex, "json_name = \"" + field.getJsonName() + "\"");
    }

    if (!extensionStrings.isEmpty()) {
      message.append(" [").append(Joiner.on(", ").join(extensionStrings)).append("]");
    }

    return message.append(";\n").toString();
  }

  // For fhir options, fully type the package name if we are not writing to the same package
  // as the annotations
  private String getOptionsPackage(String packageName) {
    return packageName.equals("." + ANNOTATION_PACKAGE) ? "" : "." + ANNOTATION_PACKAGE + ".";
  }

  private static int getOptionPosition(FieldDescriptor messageOption, List<String> ordering) {
    return ordering.contains(messageOption.getFullName())
        ? ordering.indexOf(messageOption.getFullName())
        : ordering.size();
  }

  private List<Map.Entry<FieldDescriptor, Object>> sortOptions(
      Set<Map.Entry<FieldDescriptor, Object>> original, List<String> ordering) {
    List<Map.Entry<FieldDescriptor, Object>> sorted = new ArrayList<>(original);
    sorted.sort(
        (first, second) -> {
          int orderedDiff =
              getOptionPosition(first.getKey(), ordering)
                  - getOptionPosition(second.getKey(), ordering);
          return orderedDiff != 0
              ? orderedDiff
              : first.getKey().getName().compareTo(second.getKey().getName());
        });
    return sorted;
  }

  private List<String> addExtensions(
      String extendeeType,
      Map<FieldDescriptor, Object> allFields,
      String packageName,
      String fieldIndent,
      List<String> ordering) {
    List<String> extensionStrings = new ArrayList<>();
    for (Map.Entry<FieldDescriptor, Object> extension :
        sortOptions(allFields.entrySet(), ordering)) {
      FieldDescriptor extensionField = extension.getKey();
      Object extensionValue = extension.getValue();
      if (!extensionField.toProto().getExtendee().equals(extendeeType)) {
        continue;
      }
      boolean isStringType = extensionField.getType() == FieldDescriptor.Type.STRING;
      boolean isMessageType = extensionField.getType() == FieldDescriptor.Type.MESSAGE;

      @SuppressWarnings("unchecked")
      List<Object> extensionInstances =
          extensionField.isRepeated()
              ? (List<Object>) extensionValue
              : Lists.newArrayList(extensionValue);

      String optionPackage =
          packageName.equals(extensionField.getFile().getPackage())
              ? ""
              : ("." + extensionField.getFile().getPackage() + ".");

      Escaper escaper = new CharEscaperBuilder().addEscape('\\', "\\\\").toEscaper();

      for (Object extensionInstance : extensionInstances) {
        extensionStrings.add(
            fieldIndent
                + (extendeeType.equals("." + MessageOptions.getDescriptor().getFullName())
                        || extendeeType.equals("." + EnumOptions.getDescriptor().getFullName())
                    ? "option ("
                    : "(")
                + optionPackage
                + extensionField.getName()
                + ") = "
                + (isStringType ? "\"" : isMessageType ? "{\n" : "")
                + escaper.escape(
                    isMessageType
                        ? extensionInstance
                            .toString()
                            .replaceAll("(?m)^", fieldIndent + "  ")
                            .replace("\\", "") // remove additional backslash in messages
                        : extensionInstance.toString())
                + (isStringType ? "\"" : isMessageType ? fieldIndent + "}" : ""));
      }
    }
    return extensionStrings;
  }
}
