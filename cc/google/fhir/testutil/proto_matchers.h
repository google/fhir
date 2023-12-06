// Copyright 2019 Google LLC
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

#ifndef GOOGLE_FHIR_TESTUTIL_PROTO_MATCHERS_H_
#define GOOGLE_FHIR_TESTUTIL_PROTO_MATCHERS_H_

#include <iterator>
#include <memory>
#include <ostream>
#include <string>
#include <type_traits>
#include <vector>

#include "google/protobuf/util/message_differencer.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/message.h"

namespace google::fhir::testutil {

namespace internal {

// How to compare two fields (equal vs. equivalent).
typedef google::protobuf::util::MessageDifferencer::MessageFieldComparison
    ProtoFieldComparison;

// How to compare two floating-points (exact vs. approximate).
typedef google::protobuf::util::DefaultFieldComparator::FloatComparison
    ProtoFloatComparison;

// How to compare repeated fields (whether the order of elements matters).
typedef google::protobuf::util::MessageDifferencer::RepeatedFieldComparison
    RepeatedFieldComparison;

// Whether to compare all fields (full) or only fields present in the
// expected protobuf (partial).
typedef google::protobuf::util::MessageDifferencer::Scope ProtoComparisonScope;

const ProtoFieldComparison kProtoEqual =
    google::protobuf::util::MessageDifferencer::EQUAL;
const ProtoFieldComparison kProtoEquiv =
    google::protobuf::util::MessageDifferencer::EQUIVALENT;
const ProtoFloatComparison kProtoExact =
    google::protobuf::util::DefaultFieldComparator::EXACT;
const ProtoFloatComparison kProtoApproximate =
    google::protobuf::util::DefaultFieldComparator::APPROXIMATE;
const RepeatedFieldComparison kProtoCompareRepeatedFieldsRespectOrdering =
    google::protobuf::util::MessageDifferencer::AS_LIST;
const RepeatedFieldComparison kProtoCompareRepeatedFieldsIgnoringOrdering =
    google::protobuf::util::MessageDifferencer::AS_SET;
const ProtoComparisonScope kProtoFull = google::protobuf::util::MessageDifferencer::FULL;
const ProtoComparisonScope kProtoPartial =
    google::protobuf::util::MessageDifferencer::PARTIAL;

// Options for comparing two protobufs.
struct ProtoComparison {
  ProtoComparison()
      : field_comp(kProtoEqual),
        float_comp(kProtoExact),
        treating_nan_as_equal(false),
        has_custom_margin(false),
        has_custom_fraction(false),
        repeated_field_comp(kProtoCompareRepeatedFieldsRespectOrdering),
        scope(kProtoFull),
        float_margin(0.0),
        float_fraction(0.0) {}

  ProtoFieldComparison field_comp;
  ProtoFloatComparison float_comp;
  bool treating_nan_as_equal;
  bool has_custom_margin;    // only used when float_comp = APPROXIMATE
  bool has_custom_fraction;  // only used when float_comp = APPROXIMATE
  RepeatedFieldComparison repeated_field_comp;
  ProtoComparisonScope scope;
  double float_margin;    // only used when has_custom_margin is set.
  double float_fraction;  // only used when has_custom_fraction is set.
  std::vector<std::string> ignore_fields;
  std::vector<std::string> ignore_field_paths;
};

// Whether the protobuf must be initialized.
const bool kMustBeInitialized = true;
const bool kMayBeUninitialized = false;

// Parses the TextFormat representation of a protobuf, allowing required fields
// to be missing.  Returns true iff successful.
bool ParsePartialFromAscii(absl::string_view pb_ascii, google::protobuf::Message* proto,
                           std::string* error_text);

// Returns a protobuf of type Proto by parsing the given TextFormat
// representation of it.  Required fields can be missing, in which case the
// returned protobuf will not be fully initialized.
template <class Proto>
Proto MakePartialProtoFromAscii(absl::string_view str) {
  Proto proto;
  std::string error_text;
  ASSERT_TRUE(ParsePartialFromAscii(str, &proto, &error_text))
      << "Failed to parse \"" << str << "\" as a "
      << proto.GetDescriptor()->full_name() << ":\n"
      << error_text;
  return proto;
}

// Returns true iff p and q can be compared (i.e. have the same descriptor).
bool ProtoComparable(const google::protobuf::Message& p, const google::protobuf::Message& q);

// Returns true iff actual and expected are comparable and match.  The
// comp argument specifies how the two are compared.
bool ProtoCompare(const ProtoComparison& comp, const google::protobuf::Message& actual,
                  const google::protobuf::Message& expected);

// Overload for ProtoCompare where the expected message is specified as a text
// proto.  If the text cannot be parsed as a message of the same type as the
// actual message, a CHECK failure will cause the test to fail and no subsequent
// tests will be run.
template <typename Proto>
inline bool ProtoCompare(const ProtoComparison& comp, const Proto& actual,
                         absl::string_view expected) {
  return ProtoCompare(comp, actual, MakePartialProtoFromAscii<Proto>(expected));
}

// Describes the types of the expected and the actual protocol buffer.
std::string DescribeTypes(const google::protobuf::Message& expected,
                          const google::protobuf::Message& actual);

// Prints the protocol buffer pointed to by proto.
std::string PrintProtoPointee(const google::protobuf::Message* proto);

// Describes the differences between the two protocol buffers.
std::string DescribeDiff(const ProtoComparison& comp,
                         const google::protobuf::Message& actual,
                         const google::protobuf::Message& expected);

// Common code for implementing EqualsProto, EquivToProto,
// EqualsInitializedProto, and EquivToInitializedProto.
class ProtoMatcherBase {
 public:
  ProtoMatcherBase(
      bool must_be_initialized,     // Must the argument be fully initialized?
      const ProtoComparison& comp)  // How to compare the two protobufs.
      : must_be_initialized_(must_be_initialized), comp_(new auto(comp)) {}

  ProtoMatcherBase(const ProtoMatcherBase& other)
      : must_be_initialized_(other.must_be_initialized_),
        comp_(new auto(*other.comp_)) {}

  ProtoMatcherBase(ProtoMatcherBase&& other) = default;

  virtual ~ProtoMatcherBase() {}

  // Prints the expected protocol buffer.
  virtual void PrintExpectedTo(::std::ostream* os) const = 0;

  // Returns the expected value as a protobuf object; if the object
  // cannot be created (e.g. in ProtoStringMatcher), explains why to
  // 'listener' and returns NULL.  The caller must call
  // DeleteExpectedProto() on the returned value later.
  virtual const google::protobuf::Message* CreateExpectedProto(
      const google::protobuf::Message& arg,  // For determining the type of the
                                   // expected protobuf.
      ::testing::MatchResultListener* listener) const = 0;

  // Deletes the given expected protobuf, which must be obtained from
  // a call to CreateExpectedProto() earlier.
  virtual void DeleteExpectedProto(const google::protobuf::Message* expected) const = 0;

  // Makes this matcher compare floating-points approximately.
  void SetCompareApproximately() { comp_->float_comp = kProtoApproximate; }

  // Makes this matcher treating NaNs as equal when comparing floating-points.
  void SetCompareTreatingNaNsAsEqual() { comp_->treating_nan_as_equal = true; }

  // Makes this matcher ignore string elements specified by their fully
  // qualified names, i.e., names corresponding to FieldDescriptor.full_name().
  template <class Iterator>
  void AddCompareIgnoringFields(Iterator first, Iterator last) {
    comp_->ignore_fields.insert(comp_->ignore_fields.end(), first, last);
  }

  // Makes this matcher ignore string elements specified by their relative
  // FieldPath.
  template <class Iterator>
  void AddCompareIgnoringFieldPaths(Iterator first, Iterator last) {
    comp_->ignore_field_paths.insert(comp_->ignore_field_paths.end(), first,
                                     last);
  }

  // Makes this matcher compare repeated fields ignoring ordering of elements.
  void SetCompareRepeatedFieldsIgnoringOrdering() {
    comp_->repeated_field_comp = kProtoCompareRepeatedFieldsIgnoringOrdering;
  }

  // Sets the margin of error for approximate floating point comparison.
  void SetMargin(double margin) {
    ASSERT_GE(margin, 0.0) << "Using a negative margin for Approximately";
    comp_->has_custom_margin = true;
    comp_->float_margin = margin;
  }

  // Sets the relative fraction of error for approximate floating point
  // comparison.
  void SetFraction(double fraction) {
    ASSERT_TRUE(0.0 <= fraction && fraction < 1.0)
        << "Fraction for Approximately must be >= 0.0 and < 1.0";
    comp_->has_custom_fraction = true;
    comp_->float_fraction = fraction;
  }

  // Makes this matcher compare protobufs partially.
  void SetComparePartially() { comp_->scope = kProtoPartial; }

  bool MatchAndExplain(const google::protobuf::Message& arg,
                       ::testing::MatchResultListener* listener) const {
    return MatchAndExplain(arg, false, listener);
  }

  bool MatchAndExplain(const google::protobuf::Message* arg,
                       ::testing::MatchResultListener* listener) const {
    return (arg != NULL) && MatchAndExplain(*arg, true, listener);
  }

  // Describes the expected relation between the actual protobuf and
  // the expected one.
  void DescribeRelationToExpectedProto(::std::ostream* os) const {
    if (comp_->repeated_field_comp ==
        kProtoCompareRepeatedFieldsIgnoringOrdering) {
      *os << "(ignoring repeated field ordering) ";
    }
    if (!comp_->ignore_fields.empty()) {
      *os << "(ignoring fields: ";
      const char* sep = "";
      for (size_t i = 0; i < comp_->ignore_fields.size(); ++i, sep = ", ")
        *os << sep << comp_->ignore_fields[i];
      *os << ") ";
    }
    if (comp_->float_comp == kProtoApproximate) {
      *os << "approximately ";
      if (comp_->has_custom_margin || comp_->has_custom_fraction) {
        *os << "(";
        if (comp_->has_custom_margin) {
          std::stringstream ss;
          ss << std::setprecision(std::numeric_limits<double>::digits10 + 2)
             << comp_->float_margin;
          *os << "absolute error of float or double fields <= " << ss.str();
        }
        if (comp_->has_custom_margin && comp_->has_custom_fraction) {
          *os << " or ";
        }
        if (comp_->has_custom_fraction) {
          std::stringstream ss;
          ss << std::setprecision(std::numeric_limits<double>::digits10 + 2)
             << comp_->float_fraction;
          *os << "relative error of float or double fields <= " << ss.str();
        }
        *os << ") ";
      }
    }

    *os << (comp_->scope == kProtoPartial ? "partially " : "")
        << (comp_->field_comp == kProtoEqual ? "equal" : "equivalent")
        << (comp_->treating_nan_as_equal ? " (treating NaNs as equal)" : "")
        << " to ";
    PrintExpectedTo(os);
  }

  void DescribeTo(::std::ostream* os) const {
    *os << "is " << (must_be_initialized_ ? "fully initialized and " : "");
    DescribeRelationToExpectedProto(os);
  }

  void DescribeNegationTo(::std::ostream* os) const {
    *os << "is " << (must_be_initialized_ ? "not fully initialized or " : "")
        << "not ";
    DescribeRelationToExpectedProto(os);
  }

  bool must_be_initialized() const { return must_be_initialized_; }

  const ProtoComparison& comp() const { return *comp_; }

 private:
  bool MatchAndExplain(const google::protobuf::Message& arg, bool is_matcher_for_pointer,
                       ::testing::MatchResultListener* listener) const;

  const bool must_be_initialized_;
  std::unique_ptr<ProtoComparison> comp_;
};

// Returns a copy of the given proto2 message.
inline google::protobuf::Message* CloneProto2(const google::protobuf::Message& src) {
  google::protobuf::Message* clone = src.New();
  clone->CopyFrom(src);
  return clone;
}

// Implements EqualsProto, EquivToProto, EqualsInitializedProto, and
// EquivToInitializedProto, where the matcher parameter is a protobuf.
class ProtoMatcher : public ProtoMatcherBase {
 public:
  ProtoMatcher(
      const google::protobuf::Message& expected,  // The expected protobuf.
      bool must_be_initialized,     // Must the argument be fully initialized?
      const ProtoComparison& comp)  // How to compare the two protobufs.
      : ProtoMatcherBase(must_be_initialized, comp),
        expected_(CloneProto2(expected)) {
    if (must_be_initialized) {
      EXPECT_TRUE(expected.IsInitialized())
          << "The protocol buffer given to *InitializedProto() "
          << "must itself be initialized, but the following required fields "
          << "are missing: " << expected.InitializationErrorString() << ".";
    }
  }

  virtual void PrintExpectedTo(::std::ostream* os) const {
    *os << expected_->GetDescriptor()->full_name() << " ";
    ::testing::internal::UniversalPrint(*expected_, os);
  }

  virtual const google::protobuf::Message* CreateExpectedProto(
      const google::protobuf::Message& /* arg */,
      ::testing::MatchResultListener* /* listener */) const {
    return expected_.get();
  }

  virtual void DeleteExpectedProto(const google::protobuf::Message* expected) const {}

  const std::shared_ptr<const google::protobuf::Message>& expected() const {
    return expected_;
  }

 private:
  const std::shared_ptr<const google::protobuf::Message> expected_;
};

// Implements EqualsProto, EquivToProto, EqualsInitializedProto, and
// EquivToInitializedProto, where the matcher parameter is a string.
class ProtoStringMatcher : public ProtoMatcherBase {
 public:
  ProtoStringMatcher(
      absl::string_view
          expected,              // The text representing the expected protobuf.
      bool must_be_initialized,  // Must the argument be fully initialized?
      const ProtoComparison comp)  // How to compare the two protobufs.
      : ProtoMatcherBase(must_be_initialized, comp), expected_(expected) {}

  // Parses the expected string as a protobuf of the same type as arg,
  // and returns the parsed protobuf (or NULL when the parse fails).
  // The caller must call DeleteExpectedProto() on the return value
  // later.
  virtual const google::protobuf::Message* CreateExpectedProto(
      const google::protobuf::Message& arg,
      ::testing::MatchResultListener* listener) const {
    google::protobuf::Message* expected_proto = arg.New();
    // We don't insist that the expected string parses as an
    // *initialized* protobuf.  Otherwise EqualsProto("...") may
    // wrongfully fail when the actual protobuf is not fully
    // initialized.  If the user wants to ensure that the actual
    // protobuf is initialized, they should use
    // EqualsInitializedProto("...") instead of EqualsProto("..."),
    // and the MatchAndExplain() function in ProtoMatcherBase will
    // enforce it.
    std::string error_text;
    if (ParsePartialFromAscii(expected_, expected_proto, &error_text)) {
      return expected_proto;
    } else {
      delete expected_proto;
      if (listener->IsInterested()) {
        *listener << "where ";
        PrintExpectedTo(listener->stream());
        *listener << " doesn't parse as a " << arg.GetDescriptor()->full_name()
                  << ":\n"
                  << error_text;
      }
      return NULL;
    }
  }

  virtual void DeleteExpectedProto(const google::protobuf::Message* expected) const {
    delete expected;
  }

  virtual void PrintExpectedTo(::std::ostream* os) const {
    *os << "<" << expected_ << ">";
  }

 private:
  const std::string expected_;
};

// Implements EqualsProto and EquivToProto for 2-tuple matchers.
class TupleProtoMatcher {
 public:
  explicit TupleProtoMatcher(const ProtoComparison& comp)
      : comp_(new auto(comp)) {}

  TupleProtoMatcher(const TupleProtoMatcher& other)
      : comp_(new auto(*other.comp_)) {}
  TupleProtoMatcher(TupleProtoMatcher&& other) = default;

  template <typename T1, typename T2>
  operator ::testing::Matcher< ::testing::tuple<T1, T2> >() const {
    return MakeMatcher(new Impl< ::testing::tuple<T1, T2> >(*comp_));
  }
  template <typename T1, typename T2>
  operator ::testing::Matcher<const ::testing::tuple<T1, T2>&>() const {
    return MakeMatcher(new Impl<const ::testing::tuple<T1, T2>&>(*comp_));
  }

  // Allows matcher transformers, e.g., Approximately(), Partially(), etc. to
  // change the behavior of this 2-tuple matcher.
  TupleProtoMatcher& mutable_impl() { return *this; }

  // Makes this matcher compare floating-points approximately.
  void SetCompareApproximately() { comp_->float_comp = kProtoApproximate; }

  // Makes this matcher treating NaNs as equal when comparing floating-points.
  void SetCompareTreatingNaNsAsEqual() { comp_->treating_nan_as_equal = true; }

  // Makes this matcher ignore string elements specified by their fully
  // qualified names, i.e., names corresponding to FieldDescriptor.full_name().
  template <class Iterator>
  void AddCompareIgnoringFields(Iterator first, Iterator last) {
    comp_->ignore_fields.insert(comp_->ignore_fields.end(), first, last);
  }

  // Makes this matcher ignore string elements specified by their relative
  // FieldPath.
  template <class Iterator>
  void AddCompareIgnoringFieldPaths(Iterator first, Iterator last) {
    comp_->ignore_field_paths.insert(comp_->ignore_field_paths.end(), first,
                                     last);
  }

  // Makes this matcher compare repeated fields ignoring ordering of elements.
  void SetCompareRepeatedFieldsIgnoringOrdering() {
    comp_->repeated_field_comp = kProtoCompareRepeatedFieldsIgnoringOrdering;
  }

  // Sets the margin of error for approximate floating point comparison.
  void SetMargin(double margin) {
    ASSERT_GE(margin, 0.0) << "Using a negative margin for Approximately";
    comp_->has_custom_margin = true;
    comp_->float_margin = margin;
  }

  // Sets the relative fraction of error for approximate floating point
  // comparison.
  void SetFraction(double fraction) {
    ASSERT_TRUE(0.0 <= fraction && fraction <= 1.0)
        << "Fraction for Relatively must be >= 0.0 and < 1.0";
    comp_->has_custom_fraction = true;
    comp_->float_fraction = fraction;
  }

  // Makes this matcher compares protobufs partially.
  void SetComparePartially() { comp_->scope = kProtoPartial; }

 private:
  template <typename Tuple>
  class Impl : public ::testing::MatcherInterface<Tuple> {
   public:
    explicit Impl(const ProtoComparison& comp) : comp_(comp) {}
    virtual bool MatchAndExplain(
        Tuple args, ::testing::MatchResultListener* /* listener */) const {
      using ::testing::get;
      return ProtoCompare(comp_, get<0>(args), get<1>(args));
    }
    virtual void DescribeTo(::std::ostream* os) const {
      *os << (comp_.field_comp == kProtoEqual ? "are equal" : "are equivalent");
    }
    virtual void DescribeNegationTo(::std::ostream* os) const {
      *os << (comp_.field_comp == kProtoEqual ? "are not equal"
                                              : "are not equivalent");
    }

   private:
    const ProtoComparison comp_;
  };

  std::unique_ptr<ProtoComparison> comp_;
};

typedef ::testing::PolymorphicMatcher<ProtoMatcher> PolymorphicProtoMatcher;

}  // namespace internal

// Creates a polymorphic matcher that matches a 2-tuple where
// first.Equals(second) is true.
inline internal::TupleProtoMatcher EqualsProto() {
  internal::ProtoComparison comp;
  comp.field_comp = internal::kProtoEqual;
  return internal::TupleProtoMatcher(comp);
}

// Creates a polymorphic matcher that matches a 2-tuple where
// first.Equivalent(second) is true.
inline internal::TupleProtoMatcher EquivToProto() {
  internal::ProtoComparison comp;
  comp.field_comp = internal::kProtoEquiv;
  return internal::TupleProtoMatcher(comp);
}

// Constructs a matcher that matches the argument if
// argument.Equals(x) or argument->Equals(x) returns true.
inline internal::PolymorphicProtoMatcher EqualsProto(const google::protobuf::Message& x) {
  internal::ProtoComparison comp;
  comp.field_comp = internal::kProtoEqual;
  return ::testing::MakePolymorphicMatcher(
      internal::ProtoMatcher(x, internal::kMayBeUninitialized, comp));
}
inline ::testing::PolymorphicMatcher<internal::ProtoStringMatcher> EqualsProto(
    absl::string_view x) {
  internal::ProtoComparison comp;
  comp.field_comp = internal::kProtoEqual;
  return ::testing::MakePolymorphicMatcher(
      internal::ProtoStringMatcher(x, internal::kMayBeUninitialized, comp));
}
template <class Proto>
inline internal::PolymorphicProtoMatcher EqualsProto(absl::string_view str) {
  return EqualsProto(internal::MakePartialProtoFromAscii<Proto>(str));
}

// Constructs a matcher that matches the argument if
// argument.Equivalent(x) or argument->Equivalent(x) returns true.
inline internal::PolymorphicProtoMatcher EquivToProto(
    const google::protobuf::Message& x) {
  internal::ProtoComparison comp;
  comp.field_comp = internal::kProtoEquiv;
  return ::testing::MakePolymorphicMatcher(
      internal::ProtoMatcher(x, internal::kMayBeUninitialized, comp));
}
inline ::testing::PolymorphicMatcher<internal::ProtoStringMatcher> EquivToProto(
    absl::string_view x) {
  internal::ProtoComparison comp;
  comp.field_comp = internal::kProtoEquiv;
  return ::testing::MakePolymorphicMatcher(
      internal::ProtoStringMatcher(x, internal::kMayBeUninitialized, comp));
}
template <class Proto>
inline internal::PolymorphicProtoMatcher EquivToProto(absl::string_view str) {
  return EquivToProto(internal::MakePartialProtoFromAscii<Proto>(str));
}

// IgnoringRepeatedFieldOrdering(m) returns a matcher that is the same as m,
// except that it ignores the relative ordering of elements within each repeated
// field in m. See google::protobuf::MessageDifferencer::TreatAsSet() for more details.
template <class InnerProtoMatcher>
inline InnerProtoMatcher IgnoringRepeatedFieldOrdering(
    InnerProtoMatcher inner_proto_matcher) {
  inner_proto_matcher.mutable_impl().SetCompareRepeatedFieldsIgnoringOrdering();
  return inner_proto_matcher;
}

// Partially(m) returns a matcher that is the same as m, except that
// only fields present in the expected protobuf are considered (using
// google::protobuf::util::MessageDifferencer's PARTIAL comparison option).  For
// example, Partially(EqualsProto(p)) will ignore any field that's
// not set in p when comparing the protobufs. The inner matcher m can
// be any of the Equals* and EquivTo* protobuf matchers above.
template <class InnerProtoMatcher>
inline InnerProtoMatcher Partially(InnerProtoMatcher inner_proto_matcher) {
  inner_proto_matcher.mutable_impl().SetComparePartially();
  return inner_proto_matcher;
}

}  // namespace google::fhir::testutil

#endif  // GOOGLE_FHIR_TESTUTIL_PROTO_MATCHERS_H_
