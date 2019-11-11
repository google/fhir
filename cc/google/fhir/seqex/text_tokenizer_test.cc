// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "google/fhir/seqex/text_tokenizer.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_join.h"

namespace google {
namespace fhir {
namespace seqex {

std::vector<std::string> ExtractText(
    const std::vector<TextTokenizer::Token>& tokens) {
  std::vector<std::string> result;
  for (const auto& t : tokens) {
    result.push_back(t.text);
  }
  return result;
}

using ::testing::ElementsAre;

TEST(TextTokenizerTest, SimpleWordTokenizer) {
  SimpleWordTokenizer t(false);
  EXPECT_THAT(ExtractText(t.Tokenize("These are tokens.")),
              ElementsAre("These", "are", "tokens"));
  SimpleWordTokenizer tl(true);
  EXPECT_THAT(ExtractText(tl.Tokenize("These are tokens")),
              ElementsAre("these", "are", "tokens"));
  EXPECT_THAT(ExtractText(tl.Tokenize("These are.tokens")),
              ElementsAre("these", "are", "tokens"));
  EXPECT_THAT(ExtractText(tl.Tokenize("These\n are\ntokens")),
              ElementsAre("these", "are", "tokens"));
  EXPECT_THAT(ExtractText(tl.Tokenize("These are. tokens")),
              ElementsAre("these", "are", "tokens"));
}

TEST(TextTokenizerTest, SimpleWordTokenizer_CharRanges) {
  SimpleWordTokenizer t(false);
  const std::string input = "These\n[**are**]\n\n\ntokens.";
  auto tokenized = t.Tokenize(input);
  EXPECT_THAT(tokenized, ElementsAre((TextTokenizer::Token){"These", 0, 5},
                                     (TextTokenizer::Token){"are", 9, 12},
                                     (TextTokenizer::Token){"tokens", 18, 24}));
  for (const auto& token : tokenized) {
    EXPECT_EQ(token.text, input.substr(token.char_start,
                                       token.char_end - token.char_start));
  }
}

TEST(TextTokenizerTest, SimpleWordTokenizer_LongText) {
  SimpleWordTokenizer t(true);
  const std::string input =
      "See elsewhere for objective data.\n\n20 year old russian speaking male "
      "admitted to [**Hospital Ward Name **] 10 on [**2000-1-10**] "
      "with\ndecompensated ETOH cirrohis with ascites and grade III "
      "esophogeal\nvarices";
  auto tokenized = t.Tokenize(input);
  ASSERT_EQ(32, tokenized.size());
  for (const auto& token : tokenized) {
    EXPECT_EQ(token.text,
              absl::AsciiStrToLower(input.substr(
                  token.char_start, token.char_end - token.char_start)));
  }
}

TEST(TextTokenizerTest, SingleTokenTokenizerTest) {
  SingleTokenTokenizer t;
  const std::string text = "There  is\n but one token.";
  EXPECT_THAT(t.Tokenize(text),
              ElementsAre((TextTokenizer::Token){text, 0, (int)text.size()}));
}

}  // namespace seqex
}  // namespace fhir
}  // namespace google
