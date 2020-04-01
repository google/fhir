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

package com.google.fhir.protogen;

import com.google.common.base.Splitter;
import com.google.common.collect.ImmutableSet;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/**
 * Generic test case for ensuring generated protos are up to date. This rule is automatically added
 * to every profile set by the generation rules.
 */
@RunWith(JUnit4.class)
public final class GeneratedProtoTest {

  private static final Splitter splitter = Splitter.on(",").omitEmptyStrings();

  @Test
  public void testGeneratedProto() throws Exception {
    ProtoGeneratorTestUtils.testGeneratedProto(
        System.getProperty("fhir_package"),
        System.getProperty("rule_name"),
        ImmutableSet.copyOf(splitter.splitToList(System.getProperty("dependencies"))),
        ImmutableSet.copyOf(splitter.splitToList(System.getProperty("imports"))));
  }
}
