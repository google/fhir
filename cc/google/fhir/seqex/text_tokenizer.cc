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

#include "absl/flags/flag.h"
#include "absl/strings/ascii.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "tensorflow/core/platform/logging.h"
#include "re2/re2.h"

ABSL_FLAG(bool, add_unigram_tokenized_text, true,
          "Output unigrams in tokenized text.");
ABSL_FLAG(bool, add_bigram_tokenized_text, false,
          "Output bigrams in tokenized text.");
ABSL_FLAG(bool, add_trigram_tokenized_text, false,
          "Output trigrams in tokenized text.");
ABSL_FLAG(bool, tokenizer_include_start_and_end, false,
          "Wrap each tokenized sequence with special <s> and </s> tokens");
ABSL_FLAG(bool, tokenizer_lowercase, true, "Lowercase tokens.");
ABSL_FLAG(bool, tokenizer_replace_numbers, false,
          "Replace numbers in tokenized text with <N>.");
ABSL_FLAG(std::string, tokenizer, "simple", "Which tokenizer to use.");

namespace google {
namespace fhir {
namespace seqex {

namespace {

const char kWhitespaceChars[] = " \t\r\n";
const char kDefaultNumericToken[] = "<N>";
const char kEndToken[] = "</s>";
const TextTokenizer::Token start = {"<s>", 0, 0};

std::vector<TextTokenizer::Token> SplitToSimpleWordTokens(
    const std::string& text) {
  std::vector<TextTokenizer::Token> result;
  if (absl::GetFlag(FLAGS_tokenizer_include_start_and_end)) {
    result.push_back(start);
  }
  for (absl::string_view p : absl::StrSplit(
           text, absl::ByAnyChar(kWhitespaceChars), absl::SkipWhitespace())) {
    TextTokenizer::Token token;
    token.text = std::string(p);
    token.char_start = p.begin() - text.data();
    token.char_end = p.end() - text.data();
    CHECK_GE(token.char_start, 0);
    CHECK_GE(token.char_end, token.char_start);
    CHECK_GE(text.size(), token.char_end);
    result.push_back(token);
  }
  if (absl::GetFlag(FLAGS_tokenizer_include_start_and_end)) {
    int end_pos = text.length();
    result.emplace_back(TextTokenizer::Token{kEndToken, end_pos, end_pos});
  }
  return result;
}

}  // namespace

void TextTokenizer::ReplaceNumbers(std::vector<Token>* tokens) const {
  ::tensorflow::int32 temp;
  for (int i = 0; i < tokens->size(); i++) {
    // Replace the token string, but retain the character range.
    if (absl::SimpleAtoi((*tokens)[i].text, &temp))
      (*tokens)[i].text = kDefaultNumericToken;
  }
}

std::vector<TextTokenizer::Token> TextTokenizer::GenerateBigrams(
    const std::vector<Token>& tokens) const {
  std::vector<Token> result;
  result.reserve(tokens.size() + 1);
  auto* previous_token = &start;
  for (const auto& token : tokens) {
    result.emplace_back(
        Token{absl::StrCat(previous_token->text, "_", token.text),
              previous_token->char_start, token.char_end});
    previous_token = &token;
  }
  if (absl::GetFlag(FLAGS_tokenizer_include_start_and_end)) {
    result.emplace_back(
        Token{absl::StrCat(previous_token->text, "_", previous_token->text),
              previous_token->char_start, previous_token->char_end});
  }
  return result;
}

std::vector<TextTokenizer::Token> TextTokenizer::GenerateTrigrams(
    const std::vector<Token>& tokens) const {
  std::vector<Token> result;
  result.reserve(tokens.size() + 2);
  auto* previous_token = &start;
  auto* older_previous_token = &start;
  for (const auto& token : tokens) {
    result.emplace_back(
        Token{absl::StrCat(older_previous_token->text, "_",
                           previous_token->text, "_", token.text),
              older_previous_token->char_start, token.char_end});
    older_previous_token = previous_token;
    previous_token = &token;
  }
  if (absl::GetFlag(FLAGS_tokenizer_include_start_and_end)) {
    result.emplace_back(
        Token{absl::StrCat(older_previous_token->text, "_",
                           previous_token->text, "_", previous_token->text),
              older_previous_token->char_start, previous_token->char_end});
    result.emplace_back(
        Token{absl::StrCat(previous_token->text, "_", previous_token->text, "_",
                           previous_token->text),
              previous_token->char_start, previous_token->char_end});
  }
  return result;
}

std::vector<TextTokenizer::Token> TextTokenizer::ProcessTokens(
    const std::vector<Token>& tokens) const {
  std::vector<Token> result(tokens);
  std::vector<Token> bigrams;
  std::vector<Token> trigrams;
  if (absl::GetFlag(FLAGS_tokenizer_replace_numbers)) {
    ReplaceNumbers(&result);
  }
  if (absl::GetFlag(FLAGS_add_bigram_tokenized_text) && tokens.size() > 1) {
    bigrams = GenerateBigrams(result);
  }
  if (absl::GetFlag(FLAGS_add_trigram_tokenized_text) && tokens.size() > 2) {
    trigrams = GenerateTrigrams(result);
  }
  if (!absl::GetFlag(FLAGS_add_unigram_tokenized_text)) {
    // Remove unigrams if they were not requested.
    result.clear();
  }
  result.insert(result.end(), bigrams.begin(), bigrams.end());
  result.insert(result.end(), trigrams.begin(), trigrams.end());
  return result;
}

std::shared_ptr<TextTokenizer> TextTokenizer::FromFlags() {
  // Strings are tokenized if in the whitelist.
  if (absl::GetFlag(FLAGS_tokenizer) == "simple") {
    return std::make_shared<SimpleWordTokenizer>();
  } else if (absl::GetFlag(FLAGS_tokenizer) == "single") {
    return std::make_shared<SingleTokenTokenizer>();
  } else {
    LOG(FATAL) << "Unknown tokenizer: " << absl::GetFlag(FLAGS_tokenizer);
  }
}

// public
std::vector<TextTokenizer::Token> SimpleWordTokenizer::Tokenize(
    absl::string_view text) const {
  std::string nopunc;
  nopunc.assign(text.data(), text.size());
  static LazyRE2 kPunctuationRE = {R"re([\p{P}\p{S}])re"};
  RE2::GlobalReplace(&nopunc, *kPunctuationRE, " ");
  if (absl::GetFlag(FLAGS_tokenizer_lowercase)) {
    nopunc = absl::AsciiStrToLower(nopunc);
  }
  return ProcessTokens(SplitToSimpleWordTokens(nopunc));
}

// public
std::vector<TextTokenizer::Token> SingleTokenTokenizer::Tokenize(
    absl::string_view text) const {
  TextTokenizer::Token t;
  t.text = std::string(text);
  t.char_start = 0;
  t.char_end = t.text.size();
  return {t};
}

}  // namespace seqex
}  // namespace fhir
}  // namespace google
