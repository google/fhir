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

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/flags/flag.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_join.h"

ABSL_DECLARE_FLAG(bool, add_unigram_tokenized_text);
ABSL_DECLARE_FLAG(bool, add_bigram_tokenized_text);
ABSL_DECLARE_FLAG(bool, add_trigram_tokenized_text);
ABSL_DECLARE_FLAG(bool, tokenizer_include_start_and_end);
ABSL_DECLARE_FLAG(bool, tokenizer_lowercase);
ABSL_DECLARE_FLAG(bool, tokenizer_replace_numbers);
ABSL_DECLARE_FLAG(std::string, tokenizer);

namespace google {
namespace fhir {
namespace seqex {

// This test fixture manages default flag settings, restoring between tests.
class TextTokenizerTest : public ::testing::Test {
 public:
  void SetUp() override {
    tokenizer_ = absl::GetFlag(FLAGS_tokenizer);
    tokenizer_include_start_and_end_ =
        absl::GetFlag(FLAGS_tokenizer_include_start_and_end);
    tokenizer_lowercase_ = absl::GetFlag(FLAGS_tokenizer_lowercase);
    tokenizer_replace_numbers_ = absl::GetFlag(FLAGS_tokenizer_replace_numbers);
    unigram_tokenized_text_ = absl::GetFlag(FLAGS_add_unigram_tokenized_text);
    bigram_tokenized_text_ = absl::GetFlag(FLAGS_add_bigram_tokenized_text);
    trigram_tokenized_text_ = absl::GetFlag(FLAGS_add_trigram_tokenized_text);
  }

  void TearDown() override {
    absl::SetFlag(&FLAGS_tokenizer, tokenizer_);
    absl::SetFlag(&FLAGS_tokenizer_include_start_and_end,
                  tokenizer_include_start_and_end_);
    absl::SetFlag(&FLAGS_tokenizer_lowercase, tokenizer_lowercase_);
    absl::SetFlag(&FLAGS_tokenizer_replace_numbers, tokenizer_replace_numbers_);
    absl::SetFlag(&FLAGS_add_unigram_tokenized_text, unigram_tokenized_text_);
    absl::SetFlag(&FLAGS_add_bigram_tokenized_text, bigram_tokenized_text_);
    absl::SetFlag(&FLAGS_add_bigram_tokenized_text, trigram_tokenized_text_);
  }

 private:
  std::string tokenizer_;
  bool tokenizer_lowercase_;
  bool tokenizer_include_start_and_end_;
  bool tokenizer_replace_numbers_;
  bool unigram_tokenized_text_;
  bool bigram_tokenized_text_;
  bool trigram_tokenized_text_;
};

std::vector<std::string> ExtractText(
    const std::vector<TextTokenizer::Token>& tokens) {
  std::vector<std::string> result;
  for (const auto& t : tokens) {
    result.push_back(t.text);
  }
  return result;
}

using ::testing::ElementsAre;

TEST_F(TextTokenizerTest, FromFlagsDefaultValue) {
  std::shared_ptr<TextTokenizer> tokenizer = TextTokenizer::FromFlags();
  ASSERT_THAT(tokenizer.get(), testing::Not(nullptr));
  EXPECT_THAT(ExtractText(tokenizer->Tokenize("These 2 tokens")),
              ElementsAre("these", "2", "tokens"));
}

TEST_F(TextTokenizerTest, FromFlagsOverride) {
  absl::SetFlag(&FLAGS_tokenizer, "single");
  std::shared_ptr<TextTokenizer> tokenizer = TextTokenizer::FromFlags();
  ASSERT_THAT(tokenizer.get(), testing::Not(nullptr));
  EXPECT_THAT(ExtractText(tokenizer->Tokenize("These 2 tokens")),
              ElementsAre("These 2 tokens"));
}

TEST_F(TextTokenizerTest, SimpleWordTokenizer) {
  absl::SetFlag(&FLAGS_tokenizer_lowercase, false);
  SimpleWordTokenizer t;
  EXPECT_THAT(ExtractText(t.Tokenize("These are tokens.")),
              ElementsAre("These", "are", "tokens"));
}

TEST_F(TextTokenizerTest, SimpleWordTokenizerLowerCase) {
  absl::SetFlag(&FLAGS_tokenizer_lowercase, true);
  SimpleWordTokenizer t;
  EXPECT_THAT(ExtractText(t.Tokenize("These are tokens")),
              ElementsAre("these", "are", "tokens"));
  EXPECT_THAT(ExtractText(t.Tokenize("These are.tokens")),
              ElementsAre("these", "are", "tokens"));
  EXPECT_THAT(ExtractText(t.Tokenize("These\n are\ntokens")),
              ElementsAre("these", "are", "tokens"));
  EXPECT_THAT(ExtractText(t.Tokenize("These are. tokens")),
              ElementsAre("these", "are", "tokens"));
}

TEST_F(TextTokenizerTest, SimpleWordTokenizerCharRanges) {
  absl::SetFlag(&FLAGS_tokenizer_lowercase, false);
  SimpleWordTokenizer t;
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

TEST_F(TextTokenizerTest, StartAndEndTokens) {
  SimpleWordTokenizer t;
  absl::SetFlag(&FLAGS_tokenizer_include_start_and_end, true);
  const std::string input = "These\n[**are**]\n\n\ntokens.";
  auto tokenized = t.Tokenize(input);
  EXPECT_THAT(tokenized, ElementsAre((TextTokenizer::Token){"<s>", 0, 0},
                                     (TextTokenizer::Token){"these", 0, 5},
                                     (TextTokenizer::Token){"are", 9, 12},
                                     (TextTokenizer::Token){"tokens", 18, 24},
                                     (TextTokenizer::Token){"</s>", 25, 25}));
}

TEST_F(TextTokenizerTest, SimpleWordTokenizerLongText) {
  SimpleWordTokenizer t;
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

TEST_F(TextTokenizerTest, SimpleWordTokenizerReplaceNumbers) {
  absl::SetFlag(&FLAGS_tokenizer_replace_numbers, true);
  SimpleWordTokenizer t;
  auto tokenized = t.Tokenize("11 + 22 equals 33");
  EXPECT_THAT(tokenized, ElementsAre((TextTokenizer::Token){"<N>", 0, 2},
                                     (TextTokenizer::Token){"<N>", 5, 7},
                                     (TextTokenizer::Token){"equals", 8, 14},
                                     (TextTokenizer::Token){"<N>", 15, 17}));
}

TEST_F(TextTokenizerTest, SimpleWordTokenizerAddBigrams) {
  absl::SetFlag(&FLAGS_tokenizer_replace_numbers, true);
  absl::SetFlag(&FLAGS_add_unigram_tokenized_text, false);
  absl::SetFlag(&FLAGS_add_bigram_tokenized_text, true);
  SimpleWordTokenizer t;
  EXPECT_THAT(ExtractText(t.Tokenize("3 tokens total")),
              ElementsAre("<s>_<N>", "<N>_tokens", "tokens_total"));
}

TEST_F(TextTokenizerTest, SimpleWordTokenizerAddBigramsWithStartEnd) {
  absl::SetFlag(&FLAGS_tokenizer_replace_numbers, true);
  absl::SetFlag(&FLAGS_add_unigram_tokenized_text, false);
  absl::SetFlag(&FLAGS_add_bigram_tokenized_text, true);
  absl::SetFlag(&FLAGS_tokenizer_include_start_and_end, true);
  SimpleWordTokenizer t;
  EXPECT_THAT(ExtractText(t.Tokenize("3 tokens total")),
              ElementsAre("<s>_<s>", "<s>_<N>", "<N>_tokens", "tokens_total",
                          "total_</s>", "</s>_</s>"));
}

TEST_F(TextTokenizerTest, SimpleWordTokenizerUnigramAndTrigrams) {
  absl::SetFlag(&FLAGS_tokenizer_replace_numbers, true);
  absl::SetFlag(&FLAGS_add_trigram_tokenized_text, true);
  SimpleWordTokenizer t;
  EXPECT_THAT(ExtractText(t.Tokenize("3 tokens total")),
              ElementsAre("<N>", "tokens", "total", "<s>_<s>_<N>",
                          "<s>_<N>_tokens", "<N>_tokens_total"));
}

TEST_F(TextTokenizerTest, SimpleWordTokenizerUnigramAndTrigramsWithStartEnd) {
  absl::SetFlag(&FLAGS_tokenizer_replace_numbers, true);
  absl::SetFlag(&FLAGS_add_trigram_tokenized_text, true);
  absl::SetFlag(&FLAGS_tokenizer_include_start_and_end, true);
  SimpleWordTokenizer t;
  EXPECT_THAT(
      ExtractText(t.Tokenize("3 tokens total")),
      ElementsAre("<s>", "<N>", "tokens", "total", "</s>", "<s>_<s>_<s>",
                  "<s>_<s>_<N>", "<s>_<N>_tokens", "<N>_tokens_total",
                  "tokens_total_</s>", "total_</s>_</s>", "</s>_</s>_</s>"));
}

TEST_F(TextTokenizerTest, SingleTokenTokenizer) {
  SingleTokenTokenizer t;
  const std::string text = "There  is\n but one token.";
  EXPECT_THAT(t.Tokenize(text),
              ElementsAre((TextTokenizer::Token){text, 0, (int)text.size()}));
}

}  // namespace seqex
}  // namespace fhir
}  // namespace google
