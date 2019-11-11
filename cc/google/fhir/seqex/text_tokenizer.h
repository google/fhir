/*
 * Copyright 2018 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef GOOGLE_FHIR_SEQEX_TEXT_TOKENIZER_H_
#define GOOGLE_FHIR_SEQEX_TEXT_TOKENIZER_H_

#include <string>
#include <tuple>
#include <vector>

#include "absl/strings/string_view.h"

namespace google {
namespace fhir {
namespace seqex {


class TextTokenizer {
 public:
  struct Token {
    std::string text;
    int char_start;
    int char_end;

    bool operator==(const Token& rhs) const {
      return std::tie(text, char_start, char_end) ==
             std::tie(rhs.text, rhs.char_start, rhs.char_end);
    }

    std::ostream& operator<<(std::ostream& os) const {
      os << text.substr(char_start, char_end - char_start);
      return os;
    }
  };

  // Decomposes text into ordered strings.
  virtual std::vector<Token> Tokenize(absl::string_view text) = 0;
  virtual ~TextTokenizer() {}
};

// Breaks up text based on white-space and ignores punctuation.
class SimpleWordTokenizer : public TextTokenizer {
 public:
  explicit SimpleWordTokenizer(bool lowercase) : lowercase_(lowercase) {}
  std::vector<Token> Tokenize(absl::string_view text) override;

 private:
  bool lowercase_;
};

// The text IS the token.
class SingleTokenTokenizer : public TextTokenizer {
 public:
  std::vector<Token> Tokenize(absl::string_view text) override;
};

}  // namespace seqex
}  // namespace fhir
}  // namespace google

#endif  // GOOGLE_FHIR_SEQEX_TEXT_TOKENIZER_H_
