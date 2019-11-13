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

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.fail;

import com.google.fhir.r4.core.Decimal;
import java.math.BigDecimal;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit tests for {@link DecimalWrapper}. */
@RunWith(JUnit4.class)
public final class DecimalWrapperTest {

  private void expectDecimalOutOfRangeError(java.lang.String input) {
    try {
      DecimalWrapper wrapper = new DecimalWrapper(input);
      fail("Unexpected parse success for input: '" + input + "', result is " + wrapper.toString());
    } catch (IllegalArgumentException e) {
      assertThat(e).hasMessageThat().contains("Out of range decimal value");
    }
  }

  @Test
  public void parseDecimal() {
    Decimal parsed;
    Decimal expected;

    parsed = new DecimalWrapper("185").getWrapped();
    expected = Decimal.newBuilder().setValue("185").build();
    assertThat(parsed).isEqualTo(expected);

    parsed = new DecimalWrapper("-40").getWrapped();
    expected = Decimal.newBuilder().setValue("-40").build();
    assertThat(parsed).isEqualTo(expected);

    parsed = new DecimalWrapper("0.0099").getWrapped();
    expected = Decimal.newBuilder().setValue("0.0099").build();
    assertThat(parsed).isEqualTo(expected);

    parsed = new DecimalWrapper("100").getWrapped();
    expected = Decimal.newBuilder().setValue("100").build();
    assertThat(parsed).isEqualTo(expected);

    parsed = new DecimalWrapper("100.00").getWrapped();
    expected = Decimal.newBuilder().setValue("100.00").build();
    assertThat(parsed).isEqualTo(expected);

    parsed = new DecimalWrapper("0").getWrapped();
    expected = Decimal.newBuilder().setValue("0").build();
    assertThat(parsed).isEqualTo(expected);

    parsed = new DecimalWrapper("0.00").getWrapped();
    expected = Decimal.newBuilder().setValue("0.00").build();
    assertThat(parsed).isEqualTo(expected);

    // Test some error cases.
    expectDecimalOutOfRangeError(
        new BigDecimal(Double.MAX_VALUE).multiply(BigDecimal.TEN).toString());
    expectDecimalOutOfRangeError(
        new BigDecimal(Double.MAX_VALUE).multiply(BigDecimal.TEN).negate().toString());
  }

  @Test
  public void highPrecisionDecimal() {
    Decimal parsed = new DecimalWrapper("1.00065022141624642").getWrapped();
    Decimal expected = Decimal.newBuilder().setValue("1.00065022141624642").build();
    assertThat(parsed).isEqualTo(expected);
    assertThat(new DecimalWrapper(expected).toString()).isEqualTo("1.00065022141624642");
  }

  @Test
  public void printDecimal() {
    Decimal input;

    input = Decimal.newBuilder().setValue("185").build();
    assertThat(new DecimalWrapper(input).toString()).isEqualTo("185");

    input = Decimal.newBuilder().setValue("-40").build();
    assertThat(new DecimalWrapper(input).toString()).isEqualTo("-40");

    input = Decimal.newBuilder().setValue("0.0099").build();
    assertThat(new DecimalWrapper(input).toString()).isEqualTo("0.0099");

    input = Decimal.newBuilder().setValue("100").build();
    assertThat(new DecimalWrapper(input).toString()).isEqualTo("100");

    input = Decimal.newBuilder().setValue("100.00").build();
    assertThat(new DecimalWrapper(input).toString()).isEqualTo("100.00");

    input = Decimal.newBuilder().setValue("0").build();
    assertThat(new DecimalWrapper(input).toString()).isEqualTo("0");

    input = Decimal.newBuilder().setValue("0.00").build();
    assertThat(new DecimalWrapper(input).toString()).isEqualTo("0.00");
  }
}
