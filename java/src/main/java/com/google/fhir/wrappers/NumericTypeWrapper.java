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

package com.google.fhir.wrappers;

import com.google.gson.JsonPrimitive;
import com.google.protobuf.Message;
import java.math.BigDecimal;

/** An abstract wrapper class around numeric FHIR primitive types. */
public abstract class NumericTypeWrapper<T extends Message> extends PrimitiveWrapper<T> {

  protected NumericTypeWrapper(T t) {
    super(t);
  }

  @Override
  public JsonPrimitive toJson() {
    return new JsonPrimitive(new BigDecimal(toString()));
  }
}
