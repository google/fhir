//    Copyright 2023 Google Inc.
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

import com.google.errorprone.annotations.CanIgnoreReturnValue;
import com.google.fhir.proto.ProtoGeneratorAnnotations;
import com.google.protobuf.DescriptorProtos.DescriptorProto;
import com.google.protobuf.DescriptorProtos.FieldDescriptorProto;
import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import java.util.ArrayList;
import java.util.List;

/** Test utilities for Generator files. */
final class ProtoGeneratorTestUtils {

  private ProtoGeneratorTestUtils() {}

  /**
   * Sorts all messages in a file, to make it easier to compare. This is not technically necessary
   * if we compare with ignoringRepeatedFieldOrder, but the diffs are so large if order is wrong
   * that it is very hard to read the diff.
   */
  static FileDescriptorProto sorted(FileDescriptorProto descriptor) {
    List<DescriptorProto> messages = new ArrayList<>(descriptor.getMessageTypeList());
    messages.sort((a, b) -> a.getName().compareTo(b.getName()));
    return descriptor.toBuilder().clearMessageType().addAllMessageType(messages).build();
  }

  /**
   * Cleans up some elements of the file that cause false diffs, such as protogenerator annotations,
   * which are only used for printing comments, dependencies that are pruned by post-processing, and
   * datatypes that are hardcoded rather than generated.
   */
  static FileDescriptorProto cleaned(FileDescriptorProto file) {
    FileDescriptorProto.Builder builder = file.toBuilder().clearName().clearDependency();
    builder.getOptionsBuilder().clearGoPackage();

    builder.clearMessageType();
    for (DescriptorProto message : file.getMessageTypeList()) {
      // Some datatypes are still hardcoded and added in the generation script, rather than by
      // the protogenerator.
      if (!message.getName().equals("CodingWithFixedCode")
          && !message.getName().equals("CodingWithFixedSystem")
          && !message.getName().equals("Extension")) {
        builder.addMessageType(clean(message.toBuilder()));
      }
    }
    return builder.build();
  }

  /**
   * Cleans up protogenerator annotations, which are only used for printing comments. Also removes
   * reserved ranges, since the ProtoGenerator represents these using the reservedReason annotation
   * (and so doesn't have reserved ranges until printing).
   */
  @CanIgnoreReturnValue
  private static DescriptorProto.Builder clean(DescriptorProto.Builder builder) {
    builder.getOptionsBuilder().clearExtension(ProtoGeneratorAnnotations.messageDescription);
    builder.clearReservedRange();

    List<FieldDescriptorProto.Builder> fields = new ArrayList<>(builder.getFieldBuilderList());
    builder.clearField();

    for (FieldDescriptorProto.Builder field : fields) {
      field.getOptionsBuilder().clearExtension(ProtoGeneratorAnnotations.fieldDescription);
      if (!field.getOptions().hasExtension(ProtoGeneratorAnnotations.reservedReason)) {
        builder.addField(field);
      }
    }
    for (DescriptorProto.Builder nested : builder.getNestedTypeBuilderList()) {
      clean(nested);
    }
    return builder;
  }
}
