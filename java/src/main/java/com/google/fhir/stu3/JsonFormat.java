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

import com.google.common.base.CaseFormat;
import com.google.common.collect.ImmutableMap;
import com.google.fhir.stu3.google.PrimitiveHasNoValue;
import com.google.fhir.stu3.proto.Annotations;
import com.google.fhir.stu3.proto.Base64Binary;
import com.google.fhir.stu3.proto.Boolean;
import com.google.fhir.stu3.proto.Code;
import com.google.fhir.stu3.proto.ContainedResource;
import com.google.fhir.stu3.proto.Date;
import com.google.fhir.stu3.proto.DateTime;
import com.google.fhir.stu3.proto.Decimal;
import com.google.fhir.stu3.proto.Element;
import com.google.fhir.stu3.proto.Extension;
import com.google.fhir.stu3.proto.Id;
import com.google.fhir.stu3.proto.Instant;
import com.google.fhir.stu3.proto.Integer;
import com.google.fhir.stu3.proto.Markdown;
import com.google.fhir.stu3.proto.Oid;
import com.google.fhir.stu3.proto.PositiveInt;
import com.google.fhir.stu3.proto.ReferenceId;
import com.google.fhir.stu3.proto.Time;
import com.google.fhir.stu3.proto.UnsignedInt;
import com.google.fhir.stu3.proto.Uri;
import com.google.fhir.stu3.proto.Xhtml;
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonNull;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.google.gson.stream.JsonReader;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Message;
import com.google.protobuf.MessageOrBuilder;
import java.io.IOException;
import java.io.Reader;
import java.io.StringReader;
import java.time.ZoneId;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/** Parsers for FHIR data encoded as json or xml. */
public final class JsonFormat {

  private JsonFormat() {}

  private static final PrimitiveHasNoValue PRIMITIVE_HAS_NO_VALUE =
      PrimitiveHasNoValue.newBuilder().setValueBoolean(Boolean.newBuilder().setValue(true)).build();
  private static final ImmutableMap<String, FieldDescriptor> RESOURCE_TYPES = createResourceTypes();

  private static ImmutableMap<String, FieldDescriptor> createResourceTypes() {
    Map<String, FieldDescriptor> map = new HashMap<>();
    for (FieldDescriptor field : ContainedResource.getDescriptor().getFields()) {
      map.put(field.getMessageType().getName(), field);
    }
    return ImmutableMap.copyOf(map);
  }

  private static boolean isPrimitiveType(FieldDescriptor field) {
    return field.getType() == FieldDescriptor.Type.MESSAGE
        && AnnotationUtils.isPrimitiveType(field.getMessageType());
  }

  /**
   * Creates a {@link Printer} with default configurations. The default timezone is set to the local
   * system default.
   */
  public static Printer getPrinter() {
    return new Printer(false /*omittingInsignificantWhitespace*/, ZoneId.systemDefault(), false);
  }

  /** A Printer converts protobuf message to JSON format. */
  public static class Printer {
    private final boolean omittingInsignificantWhitespace;
    private final ZoneId defaultTimeZone;
    private final boolean forAnalytics;

    private Printer(
        boolean omittingInsignificantWhitespace, ZoneId defaultTimeZone, boolean forAnalytics) {
      this.omittingInsignificantWhitespace = omittingInsignificantWhitespace;
      this.defaultTimeZone = defaultTimeZone;
      this.forAnalytics = forAnalytics;
    }

    /**
     * Create a new {@link Printer} that will omit all insignificant whitespace in the JSON output.
     * This new Printer clones all other configurations from the current Printer. Insignificant
     * whitespace is defined by the JSON spec as whitespace that appear between JSON structural
     * elements:
     *
     * <pre>
     * ws = *(
     * %x20 /              ; Space
     * %x09 /              ; Horizontal tab
     * %x0A /              ; Line feed or New line
     * %x0D )              ; Carriage return
     * </pre>
     *
     * See <a href="https://tools.ietf.org/html/rfc7159">https://tools.ietf.org/html/rfc7159</a>
     * current {@link Printer}.
     */
    public Printer omittingInsignificantWhitespace() {
      return new Printer(true, defaultTimeZone, forAnalytics);
    }

    /*
     * Create a new {@link Printer} with a default timezone. Any Dates and DateTimes printed by
     * this printer which have values in this timezone will be printed without timezone qualifiers.
     * If the default timezone is null, a timezone will be emitted whenever allowed by the FHIR
     * standard, currently always in the form of a time offset.
     */
    public Printer withDefaultTimeZone(ZoneId defaultTimeZone) {
      return new Printer(omittingInsignificantWhitespace, defaultTimeZone, forAnalytics);
    }

    /*
     * Create a new {@link Printer} which formats the output in a manner suitable for SQL queries.
     * This follows the in-progress analytics spec defined at
     * https://github.com/rbrush/sql-on-fhir/blob/master/sql-on-fhir.md
     */
    public Printer forAnalytics() {
      return new Printer(omittingInsignificantWhitespace, defaultTimeZone, true);
    }

    /**
     * Converts a protobuf message to JSON format.
     *
     * @throws IOException if writing to the output fails.
     */
    public void appendTo(MessageOrBuilder message, Appendable output) throws IOException {
      new PrinterImpl(output, omittingInsignificantWhitespace, defaultTimeZone, forAnalytics)
          .print(message);
    }

    /** Converts a protobuf message to JSON format. */
    public String print(MessageOrBuilder message) throws IOException {
      StringBuilder builder = new StringBuilder();
      appendTo(message, builder);
      return builder.toString();
    }
  }

  /**
   * An interface for json formatting that can be used in combination with the
   * omittingInsignificantWhitespace() method.
   */
  interface TextGenerator {
    void indent();

    void outdent();

    void print(final CharSequence text) throws IOException;
  }

  /** Format the json without indentation */
  private static final class CompactTextGenerator implements TextGenerator {
    private final Appendable output;

    private CompactTextGenerator(final Appendable output) {
      this.output = output;
    }

    /** ignored by compact printer */
    @Override
    public void indent() {}

    /** ignored by compact printer */
    @Override
    public void outdent() {}

    /** Print text to the output stream. */
    @Override
    public void print(final CharSequence text) throws IOException {
      output.append(text);
    }
  }
  /** A TextGenerator adds indentation when writing formatted text. */
  private static final class PrettyTextGenerator implements TextGenerator {
    private final Appendable output;
    private final StringBuilder indent = new StringBuilder();
    private boolean atStartOfLine = true;

    private PrettyTextGenerator(final Appendable output) {
      this.output = output;
    }

    /**
     * Indent text by two spaces. After calling Indent(), two spaces will be inserted at the
     * beginning of each line of text. Indent() may be called multiple times to produce deeper
     * indents.
     */
    @Override
    public void indent() {
      indent.append("  ");
    }

    /** Reduces the current indent level by two spaces, or crashes if the indent level is zero. */
    @Override
    public void outdent() {
      final int length = indent.length();
      if (length < 2) {
        throw new IllegalArgumentException("Outdent() without matching Indent().");
      }
      indent.delete(length - 2, length);
    }

    /** Print text to the output stream. */
    @Override
    public void print(final CharSequence text) throws IOException {

      final int size = text.length();
      int pos = 0;

      for (int i = 0; i < size; i++) {
        if (text.charAt(i) == '\n') {
          write(text.subSequence(pos, i + 1));
          pos = i + 1;
          atStartOfLine = true;
        }
      }
      write(text.subSequence(pos, size));
    }

    private void write(final CharSequence data) throws IOException {
      if (data.length() == 0) {
        return;
      }
      if (atStartOfLine) {
        atStartOfLine = false;
        output.append(indent);
      }
      output.append(data);
    }
  }

  /** A Printer converts protobuf messages to JSON format. */
  private static final class PrinterImpl {
    private final TextGenerator generator;
    private final CharSequence blankOrSpace;
    private final CharSequence blankOrNewLine;
    private final ZoneId defaultTimeZone;
    private final boolean forAnalytics;

    PrinterImpl(
        Appendable jsonOutput,
        boolean omittingInsignificantWhitespace,
        ZoneId defaultTimeZone,
        boolean forAnalytics) {
      // json format related properties, determined by printerType
      if (omittingInsignificantWhitespace) {
        this.generator = new CompactTextGenerator(jsonOutput);
        this.blankOrSpace = "";
        this.blankOrNewLine = "";
      } else {
        this.generator = new PrettyTextGenerator(jsonOutput);
        this.blankOrSpace = " ";
        this.blankOrNewLine = "\n";
      }
      this.defaultTimeZone = defaultTimeZone;
      this.forAnalytics = forAnalytics;
    }

    void print(MessageOrBuilder message) throws IOException {
      WellKnownTypePrinter specialPrinter =
          wellKnownTypePrinters.get(message.getDescriptorForType().getFullName());
      if (specialPrinter != null) {
        specialPrinter.print(this, message);
      } else if (AnnotationUtils.isReference(message)) {
        printReference(message);
      } else {
        printMessage(message);
      }
    }

    private interface WellKnownTypePrinter {
      void print(PrinterImpl printer, MessageOrBuilder message) throws IOException;
    }

    private static final Map<String, WellKnownTypePrinter> wellKnownTypePrinters =
        buildWellKnownTypePrinters();

    private static Map<String, WellKnownTypePrinter> buildWellKnownTypePrinters() {
      Map<String, WellKnownTypePrinter> printers = new HashMap<>();
      // Special-case contained resources.
      WellKnownTypePrinter containedResourcesPrinter =
          new WellKnownTypePrinter() {
            @Override
            public void print(PrinterImpl printer, MessageOrBuilder message) throws IOException {
              printer.printContainedResource((ContainedResource) message);
            }
          };
      printers.put(ContainedResource.getDescriptor().getFullName(), containedResourcesPrinter);
      // Special-case extensions for analytics use.
      WellKnownTypePrinter extensionPrinter =
          new WellKnownTypePrinter() {
            @Override
            public void print(PrinterImpl printer, MessageOrBuilder message) throws IOException {
              printer.printExtension((Extension) message);
            }
          };
      printers.put(Extension.getDescriptor().getFullName(), extensionPrinter);

      return printers;
    }

    /** Prints a contained resource field. */
    private void printContainedResource(ContainedResource message) throws IOException {
      for (Map.Entry<FieldDescriptor, Object> field : message.getAllFields().entrySet()) {
        if (forAnalytics) {
          /* We print only the type of the contained resource here. */
          generator.print(
              "\""
                  + ((Message) field.getValue())
                      .getDescriptorForType()
                      .getOptions()
                      .getExtension(Annotations.fhirStructureDefinitionUrl)
                  + "\"");
        } else {
          /* Print the entire contained resource. */
          print((Message) field.getValue());
        }
      }
    }

    /** Prints an extension field. */
    private void printExtension(Extension extension) throws IOException {
      if (forAnalytics) {
        generator.print("\"" + extension.getUrl().getValue() + "\"");
      } else {
        printMessage(extension);
      }
    }

    /** Prints a reference field. */
    private void printReference(MessageOrBuilder reference) throws IOException {
      FieldDescriptor uri = reference.getDescriptorForType().findFieldByName("uri");
      if (reference.hasField(uri) || forAnalytics) {
        printMessage(reference);
      } else {
        // Restore the Uri field.
        String newUri = null;
        FieldDescriptor fragment = reference.getDescriptorForType().findFieldByName("fragment");
        if (reference.hasField(fragment)) {
          newUri =
              "#" + ((com.google.fhir.stu3.proto.String) reference.getField(fragment)).getValue();
        } else {
          for (Map.Entry<FieldDescriptor, Object> field : reference.getAllFields().entrySet()) {
            if (field.getKey().getContainingOneof() != null) {
              // Convert to CamelCase and strip out the trailing "Id"
              String type =
                  CaseFormat.LOWER_UNDERSCORE.to(CaseFormat.UPPER_CAMEL, field.getKey().getName());
              type = type.substring(0, type.length() - 2);
              ReferenceId refId = (ReferenceId) field.getValue();
              newUri = type + "/" + refId.getValue();
              if (refId.hasHistory()) {
                newUri = newUri + "/_history/" + refId.getHistory().getValue();
              }
            }
          }
        }
        if (newUri != null) {
          Message.Builder builder = ((Message) reference).toBuilder();
          builder.setField(
              uri, com.google.fhir.stu3.proto.String.newBuilder().setValue(newUri).build());
          printMessage(builder);
        } else {
          printMessage(reference);
        }
      }
    }

    private boolean maybeStartMessage(boolean printedField) throws IOException {
      if (!printedField) {
        generator.print("{" + blankOrNewLine);
        generator.indent();
      } else {
        // Add line-endings for the previous field.
        generator.print("," + blankOrNewLine);
      }
      return true;
    }

    private boolean maybePrintFieldSeparator(boolean printedElement) throws IOException {
      if (printedElement) {
        generator.print("," + blankOrNewLine);
      }
      return true;
    }

    /** Prints a regular message. */
    private void printMessage(MessageOrBuilder message) throws IOException {
      boolean printedField = false;

      if (AnnotationUtils.isResource(message.getDescriptorForType()) && !forAnalytics) {
        printedField = maybeStartMessage(printedField);
        generator.print("\"resourceType\": \"" + message.getDescriptorForType().getName() + "\"");
      }

      for (Map.Entry<FieldDescriptor, Object> field : message.getAllFields().entrySet()) {
        printedField = maybeStartMessage(printedField);
        String name = field.getKey().getJsonName();
        if (field.getKey().getOptions().getExtension(Annotations.isChoiceType) && !forAnalytics) {
          printChoiceField(field.getKey(), field.getValue());
        } else if (isPrimitiveType(field.getKey())) {
          printPrimitiveField(name, field.getKey(), field.getValue());
        } else {
          printMessageField(name, field.getKey(), field.getValue());
        }
      }

      if (printedField) {
        generator.print(blankOrNewLine);
        generator.outdent();
        generator.print("}");
      } else {
        generator.print("null");
      }
    }

    private void printChoiceField(FieldDescriptor field, Object value) throws IOException {
      Message message = (Message) value;
      if (message.getAllFields().size() != 1) {
        throw new IllegalArgumentException(
            "Invalid value for choice field " + field.getName() + ": " + message);
      }
      Map.Entry<FieldDescriptor, Object> entry =
          message.getAllFields().entrySet().iterator().next();
      String name =
          field.getJsonName()
              + CaseFormat.LOWER_CAMEL.to(CaseFormat.UPPER_CAMEL, entry.getKey().getJsonName());
      if (isPrimitiveType(entry.getKey())) {
        printPrimitiveField(name, entry.getKey(), entry.getValue());
      } else {
        printMessageField(name, entry.getKey(), entry.getValue());
      }
    }

    @SuppressWarnings("unchecked")
    private void printPrimitiveField(String name, FieldDescriptor field, Object value)
        throws IOException {
      boolean printedElement = false;
      if (field.isRepeated()) {
        boolean hasValue = false;
        boolean hasExtension = false;
        List<MessageOrBuilder> list = (List<MessageOrBuilder>) value;
        List<PrimitiveWrapper> wrappers = new ArrayList<>();
        List<MessageOrBuilder> elements = new ArrayList<>();
        for (MessageOrBuilder message : list) {
          PrimitiveWrapper wrapper = primitiveWrapperOf(message, defaultTimeZone);
          wrappers.add(wrapper);
          hasValue = hasValue || wrapper.hasValue();
          Element element = wrapper.getElement();
          elements.add(element != null ? element : Element.getDefaultInstance());
          hasExtension = hasExtension || (element != null);
        }
        if (hasValue) {
          generator.print("\"" + name + "\":" + blankOrSpace);
          generator.print("[" + blankOrNewLine);
          generator.indent();
          for (PrimitiveWrapper wrapper : wrappers) {
            printedElement = maybePrintFieldSeparator(printedElement);
            generator.print(wrapper.toJson().toString());
          }
          generator.print(blankOrNewLine);
          generator.outdent();
          generator.print("]");
        }
        if (hasExtension && !forAnalytics) {
          printedElement = maybePrintFieldSeparator(printedElement);
          generator.print("\"_" + name + "\":" + blankOrSpace);
          printRepeatedMessage(elements);
        }
      } else if (forAnalytics
          && ((Message) value).getDescriptorForType().equals(ReferenceId.getDescriptor())) {
        generator.print(
            "\"" + name + "\":" + blankOrSpace + "\"" + ((ReferenceId) value).getValue() + "\"");
      } else {
        Message message = (Message) value;
        PrimitiveWrapper wrapper = primitiveWrapperOf(message, defaultTimeZone);
        if (wrapper.hasValue()) {
          generator.print("\"" + name + "\":" + blankOrSpace + wrapper.toJson());
          printedElement = true;
        }
        Element element = wrapper.getElement();
        if (element != null && !forAnalytics) {
          printedElement = maybePrintFieldSeparator(printedElement);
          generator.print("\"_" + name + "\":" + blankOrSpace);
          print(element);
        }
      }
    }

    @SuppressWarnings("unchecked")
    private void printMessageField(String name, FieldDescriptor field, Object value)
        throws IOException {
      generator.print("\"" + name + "\":" + blankOrSpace);
      if (field.isRepeated()) {
        printRepeatedMessage((List<MessageOrBuilder>) value);
      } else {
        print((MessageOrBuilder) value);
      }
    }

    private void printRepeatedMessage(List<MessageOrBuilder> value) throws IOException {
      generator.print("[" + blankOrNewLine);
      generator.indent();
      boolean printedElement = false;
      for (MessageOrBuilder element : value) {
        printedElement = maybePrintFieldSeparator(printedElement);
        print(element);
      }
      generator.print(blankOrNewLine);
      generator.outdent();
      generator.print("]");
    }
  }

  private static final Parser PARSER = Parser.newBuilder().build();

  /**
   * Return a {@link Parser} instance which can parse json-format FHIR messages. The returned
   * instance is thread-safe.
   */
  public static Parser getParser() {
    return PARSER;
  }

  /**
   * Parser for json-format FHIR proto instances. This class is thread-safe.
   *
   * <p>Use {@link JsonFormat#getParser()} to obtain the default parser, or {@link Builder} to
   * control the parser behavior.
   */
  public static final class Parser {
    private final boolean useLenientJsonReader;
    private final JsonParser jsonParser;
    private final ZoneId defaultTimeZone;

    private Parser(boolean useLenientJsonReader, ZoneId defaultTimeZone) {
      this.useLenientJsonReader = useLenientJsonReader;
      this.jsonParser = new JsonParser();
      this.defaultTimeZone = defaultTimeZone;
    }

    /** Returns a new instance of {@link Builder} with default parameters. */
    public static Builder newBuilder() {
      return new Builder(ZoneId.systemDefault());
    }

    /** Builder that can be used to obtain new instances of {@link Parser}. */
    public static final class Builder {
      private final ZoneId defaultTimeZone;

      Builder(ZoneId defaultTimeZone) {
        this.defaultTimeZone = defaultTimeZone;
      }

      /*
       * Create a new {@link Parser} with a default timezone. Any Dates and DateTimes parsed by
       * this instance which do not have explicit timezone or timezone offset information will be
       * assumed to be measured in the default timezone.
       */
      public Builder withDefaultTimeZone(ZoneId defaultTimeZone) {
        return new Builder(defaultTimeZone);
      }

      public Parser build() {
        return new Parser(false /*useLenientJsonReader */, defaultTimeZone);
      }
    }

    /**
     * Parse a text-format message from {@code input} and merge the contents into {@code builder}.
     */
    public void merge(final Reader input, final Message.Builder builder) {
      JsonReader reader = new JsonReader(input);
      reader.setLenient(useLenientJsonReader);
      JsonElement json = jsonParser.parse(reader);
      if (json.isJsonObject()) {
        mergeMessage(json.getAsJsonObject(), builder);
      } else {
        parseAndWrap(json, builder, defaultTimeZone).copyInto(builder);
      }
    }

    /**
     * Parse a text-format message from {@code input} and merge the contents into {@code builder}.
     */
    public void merge(final CharSequence input, final Message.Builder builder) {
      merge(new StringReader(input.toString()), builder);
    }

    private Map<String, FieldDescriptor> getFieldMap(Descriptor descriptor) {
      Map<String, FieldDescriptor> nameToDescriptorMap = new HashMap<>();
      for (FieldDescriptor field : descriptor.getFields()) {
        if (field.getOptions().getExtension(Annotations.isChoiceType)) {
          // All the contained fields go in this message.
          Map<String, FieldDescriptor> innerMap = getFieldMap(field.getMessageType());
          for (Map.Entry<String, FieldDescriptor> entry : innerMap.entrySet()) {
            nameToDescriptorMap.put(
                field.getJsonName()
                    + CaseFormat.LOWER_CAMEL.to(CaseFormat.UPPER_CAMEL, entry.getKey()),
                field);
          }
        } else {
          nameToDescriptorMap.put(field.getJsonName(), field);
          if (isPrimitiveType(field)) {
            // Handle extensions on primitive fields.
            nameToDescriptorMap.put("_" + field.getJsonName(), field);
          }
        }
      }
      return nameToDescriptorMap;
    }

    private void mergeMessage(JsonObject json, Message.Builder builder) {
      if (builder.getDescriptorForType().equals(ContainedResource.getDescriptor())) {
        // We handle contained resources in a special way, since we need to inspect the input to
        // determine its type.
        parseContainedResource(json, builder);
        return;
      }

      // Create a map of what to expect.
      Descriptor descriptor = builder.getDescriptorForType();
      Map<String, FieldDescriptor> nameToDescriptorMap = getFieldMap(descriptor);

      for (Map.Entry<String, JsonElement> entry : json.entrySet()) {
        if (nameToDescriptorMap.containsKey(entry.getKey())) {
          FieldDescriptor field = nameToDescriptorMap.get(entry.getKey());
          if (field.getOptions().getExtension(Annotations.isChoiceType)) {
            mergeChoiceField(field, entry.getKey(), entry.getValue(), builder);
          } else {
            mergeField(field, entry.getValue(), builder);
          }
        } else if (entry.getKey().equals("resourceType")) {
          String inputType = entry.getValue().getAsString();
          if (!AnnotationUtils.isResource(descriptor) || !inputType.equals(descriptor.getName())) {
            throw new IllegalArgumentException(
                "Trying to parse a resource of type "
                    + inputType
                    + ", but the target field is of type "
                    + descriptor.getFullName());
          }
        } else {
          String names = "";
          for (Map.Entry<String, FieldDescriptor> e : nameToDescriptorMap.entrySet()) {
            names = names + " " + e.getKey();
          }
          throw new IllegalArgumentException(
              "Unknown field "
                  + entry.getKey()
                  + " in input of expected type "
                  + builder.getDescriptorForType().getFullName()
                  + ", known fields: "
                  + names);
        }
      }

      if (AnnotationUtils.isReference(builder.getDescriptorForType())) {
        // Special-case the "reference" field, which was parsed into the uri field.
        ResourceUtils.splitIfRelativeReference(builder);
      }
    }

    private void mergeChoiceField(
        FieldDescriptor field, String fieldName, JsonElement json, Message.Builder builder) {
      Descriptor descriptor = field.getMessageType();
      Map<String, FieldDescriptor> nameToDescriptorMap = getFieldMap(descriptor);
      String fieldNameSuffix =
          CaseFormat.UPPER_CAMEL.to(
              CaseFormat.LOWER_CAMEL, fieldName.substring(field.getJsonName().length()));
      FieldDescriptor choiceField = nameToDescriptorMap.get(fieldNameSuffix);
      if (choiceField == null) {
        throw new IllegalArgumentException(
            "Can't find field: "
                + fieldNameSuffix
                + " in type "
                + descriptor.getName()
                + " for field "
                + field.getName());
      }
      Message.Builder choiceTypeBuilder;
      if (builder.hasField(field)) {
        choiceTypeBuilder = ((Message) builder.getField(field)).toBuilder();
      } else {
        choiceTypeBuilder = builder.newBuilderForField(field);
      }
      mergeField(choiceField, json, choiceTypeBuilder);
      builder.setField(field, choiceTypeBuilder.build());
    }

    private void mergeField(FieldDescriptor field, JsonElement json, Message.Builder builder) {
      if (!isPrimitiveType(field)
          && ((field.isRepeated() && builder.getRepeatedFieldCount(field) > 0)
              || (!field.isRepeated() && builder.hasField(field)))) {
        throw new IllegalArgumentException(
            "Field " + field.getFullName() + " has already been set.");
      }
      if (field.getContainingOneof() != null) {
        FieldDescriptor existing = builder.getOneofFieldDescriptor(field.getContainingOneof());
        if (existing != null) {
          throw new IllegalArgumentException(
              "Cannot set field "
                  + field.getFullName()
                  + " because another field "
                  + existing.getFullName()
                  + " belonging to the same oneof has already been set ");
        }
      }
      if (field.isRepeated()) {
        mergeRepeatedField(field, json.getAsJsonArray(), builder);
      } else {
        Message value = parseFieldValue(field, json, builder);
        if (builder.hasField(field) && isPrimitiveType(field)) {
          builder.setField(field, mergePrimitiveField((Message) builder.getField(field), value));
        } else {
          builder.setField(field, value);
        }
      }
    }

    private Message mergePrimitiveField(Message first, Message second) {
      boolean firstHasValue = PrimitiveWrapper.hasValue(first);
      boolean secondHasValue = PrimitiveWrapper.hasValue(second);
      boolean hasValue = firstHasValue || secondHasValue;
      Message.Builder fieldMerger = first.newBuilderForType();

      fieldMerger.mergeFrom(first);
      fieldMerger.mergeFrom(second);
      // Clear the PrimitiveHasNoValueExtension if we have too many copies of it.
      if (!firstHasValue || !secondHasValue) {
        ExtensionWrapper extensions =
            ExtensionWrapper.fromExtensionsIn(fieldMerger)
                .clearMatchingExtensions(PRIMITIVE_HAS_NO_VALUE);
        // Add the PrimitiveHasNoValueExtension back if necessary.
        if (!hasValue) {
          extensions.add(PRIMITIVE_HAS_NO_VALUE);
        }
        FieldDescriptor extensionField =
            fieldMerger.getDescriptorForType().findFieldByName("extension");
        fieldMerger.setField(extensionField, extensions.build());
      }
      return fieldMerger.build();
    }

    private void mergeRepeatedField(
        FieldDescriptor field, JsonArray json, Message.Builder builder) {
      boolean hasExistingField = builder.getRepeatedFieldCount(field) > 0;
      if (hasExistingField && builder.getRepeatedFieldCount(field) != json.size()) {
        throw new IllegalArgumentException("Repeated field length mismatch for field: " + field);
      }

      for (int i = 0; i < json.size(); ++i) {
        Message value = parseFieldValue(field, json.get(i), builder);
        if (hasExistingField) {
          builder.setRepeatedField(
              field, i, mergePrimitiveField(value, (Message) builder.getRepeatedField(field, i)));
        } else {
          builder.addRepeatedField(field, value);
        }
      }
    }

    private void parseContainedResource(JsonObject json, Message.Builder builder) {
      String resourceType = json.get("resourceType").getAsString();
      FieldDescriptor resource = RESOURCE_TYPES.get(resourceType);
      if (resource == null) {
        throw new IllegalArgumentException("Unsupported resource type: " + resourceType);
      }
      Message.Builder innerBuilder = builder.newBuilderForField(resource);
      mergeMessage(json, innerBuilder);
      builder.setField(resource, innerBuilder.build()).build();
    }

    // Supress lack of compile-time type safety because of proto newBuilderForType
    @SuppressWarnings("unchecked")
    private Message parseFieldValue(
        FieldDescriptor field, JsonElement json, Message.Builder builder) {
      // Everything at the fhir-spec level should be a Message.
      if (field.getType() != FieldDescriptor.Type.MESSAGE) {
        throw new IllegalArgumentException(
            "Error in FHIR proto definition: Field " + field + " is not a message.");
      }

      Message.Builder subBuilder = builder.newBuilderForField(field);

      if (isPrimitiveType(field)) {
        if (json.isJsonObject()) {
          // Special-case primitive type extensions
          mergeMessage((JsonObject) json, subBuilder);
        }
        try {
          return parseAndWrap(json, subBuilder, defaultTimeZone).copyInto(subBuilder).build();
        } catch (IllegalArgumentException e) {
          throw new IllegalArgumentException("Error parsing field: " + field.getFullName(), e);
        }
      }

      if (!(json instanceof JsonObject)) {
        throw new IllegalArgumentException("Expected JsonObject for field " + field);
      } else {
        mergeMessage((JsonObject) json, subBuilder);
        return subBuilder.build();
      }
    }
  }

  public static PrimitiveWrapper primitiveWrapperOf(
      MessageOrBuilder message, ZoneId defaultTimeZone) {
    Descriptor descriptor = message.getDescriptorForType();
    if (descriptor.getOptions().hasExtension(Annotations.fhirValuesetUrl)) {
      return CodeWrapper.of(message);
    }
    switch (descriptor.getName()) {
      case "Base64Binary":
        return new Base64BinaryWrapper((Base64Binary) message);
      case "Boolean":
        return new BooleanWrapper((Boolean) message);
      case "Code":
        return new CodeWrapper((Code) message);
      case "Date":
        return new DateWrapper((Date) message);
      case "DateTime":
        if (defaultTimeZone == null) {
          return new DateTimeWrapper((DateTime) message);
        } else {
          return new DateTimeWrapper((DateTime) message, defaultTimeZone);
        }
      case "Decimal":
        return new DecimalWrapper((Decimal) message);
      case "Id":
        return new IdWrapper((Id) message);
      case "Instant":
        return new InstantWrapper((Instant) message);
      case "Integer":
        return new IntegerWrapper((Integer) message);
      case "Markdown":
        return new MarkdownWrapper((Markdown) message);
      case "Oid":
        return new OidWrapper((Oid) message);
      case "PositiveInt":
        return new PositiveIntWrapper((PositiveInt) message);
      case "String":
        return new StringWrapper((com.google.fhir.stu3.proto.String) message);
      case "Time":
        return new TimeWrapper((Time) message);
      case "UnsignedInt":
        return new UnsignedIntWrapper((UnsignedInt) message);
      case "Uri":
        return new UriWrapper((Uri) message);
      case "Xhtml":
        return new XhtmlWrapper((Xhtml) message);
      default:
        throw new IllegalArgumentException(
            "Unexpected primitive FHIR type: " + descriptor.getName());
    }
  }

  public static PrimitiveWrapper parseAndWrap(
      JsonElement json, MessageOrBuilder message, ZoneId defaultTimeZone) {
    Descriptor descriptor = message.getDescriptorForType();
    if (json.isJsonArray()) {
      // JsonArrays are not allowed here
      throw new IllegalArgumentException("Cannot wrap a JsonArray.");
    }
    // JSON objects represents extension on a primitive, and are treated as null values.
    if (json.isJsonObject()) {
      json = JsonNull.INSTANCE;
    }
    String jsonString = json.isJsonNull() ? null : json.getAsJsonPrimitive().getAsString();

    if (descriptor.getOptions().hasExtension(Annotations.fhirValuesetUrl)) {
      return new CodeWrapper(jsonString);
    }
    // TODO: Make proper class hierarchy for wrapper input types,
    // so these can all accept JsonElement in constructor, and do type checking there.
    switch (descriptor.getName()) {
      case "Base64Binary":
        checkIsString(json);
        return new Base64BinaryWrapper(jsonString);
      case "Boolean":
        checkIsBoolean(json);
        return new BooleanWrapper(jsonString);
      case "Code":
        checkIsString(json);
        return new CodeWrapper(jsonString);
      case "Date":
        checkIsString(json);
        return new DateWrapper(jsonString, defaultTimeZone);
      case "DateTime":
        checkIsString(json);
        return new DateTimeWrapper(jsonString, defaultTimeZone);
      case "Decimal":
        checkIsNumber(json);
        return new DecimalWrapper(jsonString);
      case "Id":
        checkIsString(json);
        return new IdWrapper(jsonString);
      case "Instant":
        checkIsString(json);
        return new InstantWrapper(jsonString);
      case "Integer":
        checkIsNumber(json);
        return new IntegerWrapper(jsonString);
      case "Markdown":
        checkIsString(json);
        return new MarkdownWrapper(jsonString);
      case "Oid":
        checkIsString(json);
        return new OidWrapper(jsonString);
      case "PositiveInt":
        checkIsNumber(json);
        return new PositiveIntWrapper(jsonString);
      case "String":
        checkIsString(json);
        return new StringWrapper(jsonString);
      case "Time":
        checkIsString(json);
        return new TimeWrapper(jsonString);
      case "UnsignedInt":
        checkIsNumber(json);
        return new UnsignedIntWrapper(jsonString);
      case "Uri":
        checkIsString(json);
        return new UriWrapper(jsonString);
      case "Xhtml":
        checkIsString(json);
        return new XhtmlWrapper(jsonString);
      default:
        throw new IllegalArgumentException(
            "Unexpected primitive FHIR type: " + descriptor.getName());
    }
  }

  private static void checkIsBoolean(JsonElement json) {
    if (!(json.isJsonNull() || json.isJsonObject())
        && !(json.isJsonPrimitive() && json.getAsJsonPrimitive().isBoolean())) {
      throw new IllegalArgumentException("Invalid JSON element for boolean: " + json);
    }
  }

  private static void checkIsNumber(JsonElement json) {
    if (!(json.isJsonNull() || json.isJsonObject())
        && !(json.isJsonPrimitive() && json.getAsJsonPrimitive().isNumber())) {
      throw new IllegalArgumentException("Invalid JSON element for number: " + json);
    }
  }

  private static void checkIsString(JsonElement json) {
    if (!(json.isJsonNull() || json.isJsonObject())
        && !(json.isJsonPrimitive() && json.getAsJsonPrimitive().isString())) {
      throw new IllegalArgumentException("Invalid JSON element for string-like: " + json);
    }
  }
}
