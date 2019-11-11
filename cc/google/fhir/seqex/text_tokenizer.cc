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

#include "absl/strings/ascii.h"
#include "absl/strings/str_split.h"
#include "tensorflow/core/platform/logging.h"
#include "re2/re2.h"

using std::vector;

namespace google {
namespace fhir {
namespace seqex {

namespace {
const char kWhitespaceChars[] = " \t\r\n";

vector<TextTokenizer::Token> SplitToSimpleWordTokens(const std::string& text) {
  std::vector<TextTokenizer::Token> result;
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
  return result;
}
}  // namespace

// public
std::vector<TextTokenizer::Token> SimpleWordTokenizer::Tokenize(
    absl::string_view text) {
  std::string nopunc;
  nopunc.assign(text.data(), text.size());
  static LazyRE2 kPunctuationRE = {R"re([\p{P}\p{S}])re"};
  RE2::GlobalReplace(&nopunc, *kPunctuationRE, " ");
  if (this->lowercase_) {
    nopunc = absl::AsciiStrToLower(nopunc);
  }
  return SplitToSimpleWordTokens(nopunc);
}

// public
std::vector<TextTokenizer::Token> SingleTokenTokenizer::Tokenize(
    absl::string_view text) {
  TextTokenizer::Token t;
  t.text = std::string(text);
  t.char_start = 0;
  t.char_end = t.text.size();
  return {t};
}

}  // namespace seqex
}  // namespace fhir
}  // namespace google
