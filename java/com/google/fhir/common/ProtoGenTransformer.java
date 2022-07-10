//    Copyright 2020 Google Inc.
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

package com.google.fhir.common;

import com.google.common.base.Splitter;
import com.google.common.collect.HashBasedTable;
import com.google.common.collect.HashMultimap;
import com.google.common.collect.ImmutableMultimap;
import com.google.common.collect.ImmutableTable;
import com.google.common.collect.Multimap;
import com.google.common.collect.Table;
import com.google.errorprone.annotations.CanIgnoreReturnValue;
import com.google.fhir.r4.core.Canonical;
import com.google.fhir.r4.core.CapabilityStatement;
import com.google.fhir.r4.core.CodeSystem;
import com.google.fhir.r4.core.CompartmentDefinition;
import com.google.fhir.r4.core.ConceptMap;
import com.google.fhir.r4.core.ElementDefinition;
import com.google.fhir.r4.core.ElementDefinition.ElementDefinitionBinding;
import com.google.fhir.r4.core.ExtensionContextTypeCode;
import com.google.fhir.r4.core.OperationDefinition;
import com.google.fhir.r4.core.ResourceTypeCode;
import com.google.fhir.r4.core.StructureDefinition;
import com.google.fhir.r4.core.ValueSet;
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Message;
import java.util.function.BiConsumer;
import java.util.function.Predicate;

/**
 * Tool for converting resources used in ProtoGeneration tools to R4 so other versions can be
 * generated. This tool is explicitly NOT for generalized version conversion, and should ONLY be
 * used in the context of proto generation. There are no guarantees of completeness or correctness.
 *
 * <p>This allows directing specialized conversion rules for reading the JSON. E.g., if a code
 * string has its value changed from STU3 to R4, a converter can be specified so the json is parsed
 * successfully.
 *
 * <p>Fields to handle are specified as a tuple of (json field name, target message type) that the
 * parser should keep an eye out for. It will then execute a passed-in transform on the target
 * message, given the field value. So for instance, assume the field "foo" on SomeSubMessage is
 * renamed to "bar", and the value should get "SomePrefix-" added to it. In this case, the json
 * field name we want to look out for is "foo", so this transform would be added to a builder like
 *
 * <pre>{@code
 * builder.addTransform(
 *     "foo",
 *     SomeSubMessage.newBuilder(),
 *     (jsonValue, subMessage) -> subMessage.setBar("SomePrefix-" + jsonValue.getAsString()));
 * }</pre>
 *
 * An optional predicate argument allows the transform to only be executed in certain cases, e.g.,
 * depending on the value.
 *
 * <p>Mutli-field transforms allow operations that synthesize a result on the target based on
 * several input fields.
 */
final class ProtoGenTransformer {
  static final ProtoGenTransformer NO_OP = new Builder().build();
  static final ProtoGenTransformer STU3_TO_R4 = getStu3ProtoGenTransformer();

  /** Map from (json field name, Proto type that that is a property on) -> transforms */
  private final ImmutableTable<String, String, Transformer<?>> transformers;

  /** Map from proto type to multi-field transforms */
  private final ImmutableMultimap<String, MultiFieldTransformer<?>> multiFieldTransformers;

  private static class Transformer<B extends Message.Builder> {
    final String fieldName;
    final Descriptor type;
    final BiConsumer<JsonElement, B> transform;
    final Predicate<JsonElement> valuePredicate;

    Transformer(
        String fieldName,
        Descriptor type,
        Predicate<JsonElement> valuePredicate,
        BiConsumer<JsonElement, B> transform) {
      this.fieldName = fieldName;
      this.type = type;
      this.transform = transform;
      this.valuePredicate = valuePredicate;
    }

    @SuppressWarnings("unchecked")
    boolean apply(String testFieldName, Message.Builder builder, JsonElement element) {
      if (valuePredicate.test(element)) {
        if (element.isJsonArray()) {
          for (JsonElement subElement : element.getAsJsonArray()) {
            transform.accept(subElement, (B) builder);
          }
        } else {
          transform.accept(element, (B) builder);
        }
        return true;
      }
      return false;
    }
  }

  private static class MultiFieldTransformer<B extends Message.Builder> {
    final Descriptor type;
    final BiConsumer<JsonObject, B> transform;

    MultiFieldTransformer(Descriptor type, BiConsumer<JsonObject, B> transform) {
      this.type = type;
      this.transform = transform;
    }

    @SuppressWarnings("unchecked")
    void apply(JsonObject jsonObject, Message.Builder builder) {
      transform.accept(jsonObject, (B) builder);
    }
  }

  static class Builder {
    private final Table<String, String, Transformer<?>> transformers = HashBasedTable.create();
    private final Multimap<String, MultiFieldTransformer<?>> multiFieldTransformers =
        HashMultimap.create();

    ProtoGenTransformer build() {
      return new ProtoGenTransformer(
          ImmutableTable.copyOf(transformers), ImmutableMultimap.copyOf(multiFieldTransformers));
    }

    /** Tells the parser to ignore this field entirely. */
    <B extends Message.Builder> Builder ignore(String fieldName, B builder) {
      return addTransformer(fieldName, builder, (json, builderArg) -> {});
    }

    <B extends Message.Builder> Builder addTransformer(
        String fieldName, B builder, BiConsumer<JsonElement, B> transform) {
      return addTransformer(fieldName, builder, element -> true, transform);
    }

    @CanIgnoreReturnValue
    <B extends Message.Builder> Builder addTransformer(
        String fieldName,
        B builder,
        Predicate<JsonElement> valuePredicate,
        BiConsumer<JsonElement, B> transform) {
      transformers.put(
          fieldName,
          builder.getDescriptorForType().getFullName(),
          new Transformer<B>(fieldName, builder.getDescriptorForType(), valuePredicate, transform));
      return this;
    }

    @CanIgnoreReturnValue
    <B extends Message.Builder> Builder addMultiFieldTransformer(
        B builder, BiConsumer<JsonObject, B> transform, String... fields) {
      multiFieldTransformers.put(
          builder.getDescriptorForType().getFullName(),
          new MultiFieldTransformer<B>(builder.getDescriptorForType(), transform));
      for (String field : fields) {
        // These fields are considered to be already handled.
        ignore(field, builder);
      }
      return this;
    }
  }

  void performMultiFieldConversions(JsonObject json, Message.Builder builder) {
    for (MultiFieldTransformer<?> multiFieldTransformer :
        multiFieldTransformers.get(builder.getDescriptorForType().getFullName())) {
      multiFieldTransformer.apply(json, builder);
    }
  }

  /**
   * @return True if the field was handled. This implies that the Parser should do no further
   *     operations on this field.
   */
  boolean performSpecializedConversion(
      JsonElement json, String fieldName, Message.Builder builder) {
    Transformer<?> transformer =
        transformers.get(fieldName, builder.getDescriptorForType().getFullName());
    return transformer != null && transformer.apply(fieldName, builder, json);
  }

  private ProtoGenTransformer(
      ImmutableTable<String, String, Transformer<?>> transformers,
      ImmutableMultimap<String, MultiFieldTransformer<?>> multiFieldTransformers) {
    this.transformers = transformers;
    this.multiFieldTransformers = multiFieldTransformers;
  }

  private static ProtoGenTransformer getStu3ProtoGenTransformer() {
    Builder builder = new Builder();
    builder.ignore("extensible", ValueSet.newBuilder());
    builder.ignore("acceptUnknown", CapabilityStatement.newBuilder());
    builder.ignore("resource", OperationDefinition.newBuilder());

    builder.addTransformer(
        "valueSetReference",
        ElementDefinitionBinding.newBuilder(),
        (jsonElement, binding) -> binding.setValueSet(referenceToCanonical(jsonElement)));

    builder.addTransformer(
        "valueSetUri",
        ElementDefinitionBinding.newBuilder(),
        (jsonElement, binding) -> binding.getValueSetBuilder().setValue(jsonElement.getAsString()));

    builder.addTransformer(
        "profile",
        ElementDefinition.TypeRef.newBuilder(),
        (jsonElement, typeRef) ->
            typeRef.addProfile(Canonical.newBuilder().setValue(jsonElement.getAsString())));

    builder.addTransformer(
        "targetProfile",
        ElementDefinition.TypeRef.newBuilder(),
        (jsonElement, typeRef) ->
            typeRef.addTargetProfile(Canonical.newBuilder().setValue(jsonElement.getAsString())));

    builder.addTransformer(
        "valueSetReference",
        OperationDefinition.Parameter.Binding.newBuilder(),
        (jsonElement, binding) -> binding.setValueSet(referenceToCanonical(jsonElement)));

    builder.addTransformer(
        "profile",
        OperationDefinition.Parameter.newBuilder(),
        (jsonElement, parameter) -> parameter.addTargetProfile(referenceToCanonical(jsonElement)));

    builder.addTransformer(
        "targetReference",
        ConceptMap.newBuilder(),
        (jsonElement, conceptMap) ->
            conceptMap.getTargetBuilder().setCanonical(referenceToCanonical(jsonElement)));

    builder.addTransformer(
        "sourceReference",
        ConceptMap.newBuilder(),
        (jsonElement, conceptMap) ->
            conceptMap.getSourceBuilder().setCanonical(referenceToCanonical(jsonElement)));

    builder.addTransformer(
        "type",
        CapabilityStatement.Rest.Resource.newBuilder(),
        jsonElement -> jsonElement.getAsString().equals("BodySite"),
        (jsonElement, resource) ->
            resource.getTypeBuilder().setValue(ResourceTypeCode.Value.BODY_STRUCTURE));

    builder.addTransformer(
        "type",
        CompartmentDefinition.Resource.newBuilder(),
        jsonElement -> jsonElement.getAsString().equals("BodySite"),
        (jsonElement, resource) ->
            resource.getCodeBuilder().setValue(ResourceTypeCode.Value.BODY_STRUCTURE));

    builder.addTransformer(
        "reference",
        Canonical.newBuilder(),
        (jsonElement, canonical) -> canonical.setValue(jsonElement.getAsString()));

    // Some STU3 example uris are of the form "uri-a or uri-b", which either is valid but the
    // full string is not.
    builder.addTransformer(
        "valueUri",
        ElementDefinition.Example.newBuilder(),
        jsonElement -> jsonElement.getAsString().contains(" or "),
        (jsonElement, example) ->
            example
                .getValueBuilder()
                .getStringValueBuilder()
                .setValue(Splitter.on(" or ").splitToList(jsonElement.getAsString()).get(0)));

    // CodeSystem.identifier is singular in STU3
    builder.addMultiFieldTransformer(
        CodeSystem.newBuilder(),
        (jsonObject, codeSystem) -> {
          if (jsonObject.has("identifier")) {
            JsonObject identifier = jsonObject.getAsJsonObject("identifier");
            jsonObject.remove("identifier");
            JsonArray array = new JsonArray();
            array.add(identifier);
            jsonObject.add("identifier", array);
          }
        });

    // In R4, the STU3 fields "contextType" and "context" are combined into a single object on
    // "context"
    builder.addMultiFieldTransformer(
        StructureDefinition.newBuilder(),
        (jsonObject, structureDefinition) -> {
          if (!jsonObject.has("contextType")) {
            return;
          }
          String contextTypeString = jsonObject.getAsJsonPrimitive("contextType").getAsString();
          StructureDefinition.Context.TypeCode contextType;
          // R4 doesn't differentiate between Extension contexts "resource" and "datatype"
          if (contextTypeString.equals("resource") || contextTypeString.equals("datatype")) {
            contextType =
                StructureDefinition.Context.TypeCode.newBuilder()
                    .setValue(ExtensionContextTypeCode.Value.ELEMENT)
                    .build();
          } else if (contextTypeString.equals("extension")) {
            contextType =
                StructureDefinition.Context.TypeCode.newBuilder()
                    .setValue(ExtensionContextTypeCode.Value.EXTENSION)
                    .build();
          } else {
            throw new IllegalArgumentException("Unrecognized Context type: " + contextTypeString);
          }
          if (jsonObject.has("context")) {
            for (JsonElement contextElement : jsonObject.getAsJsonArray("context")) {
              StructureDefinition.Context.Builder contextBuilder =
                  StructureDefinition.Context.newBuilder();
              contextBuilder
                  .setType(contextType)
                  .getExpressionBuilder()
                  .setValue(contextElement.getAsJsonPrimitive().getAsString());
              structureDefinition.addContext(contextBuilder);
            }
          }
        },
        "contextType",
        "context");

    return builder.build();
  }

  private static Canonical referenceToCanonical(JsonElement json) {
    return Canonical.newBuilder()
        .setValue(json.getAsJsonObject().getAsJsonPrimitive("reference").getAsString())
        .build();
  }
}
