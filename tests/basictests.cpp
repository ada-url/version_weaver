#include "version_weaver.h"
#include <vector>

#include <gtest/gtest.h>

using TestData =
    std::pair<std::string, std::expected<version_weaver::Version,
                                         version_weaver::ParseError>>;
std::vector<TestData> parse_values = {
    {"1.0.0", version_weaver::Version{"1", "0", "0"}},
    {"1.0.0-alpha", version_weaver::Version{"1", "0", "0", "alpha"}},
    {"1.0.0-alpha.1", version_weaver::Version{"1", "0", "0", "alpha.1"}},
    {"1.0.0-0.3.7", version_weaver::Version{"1", "0", "0", "0.3.7"}},
    {"1.0.0-x.7.z.92", version_weaver::Version{"1", "0", "0", "x.7.z.92"}},
    {"1.0.0-x-y-z.--", version_weaver::Version{"1", "0", "0", "x-y-z.--"}},
    {"1.0.0-alpha+001", version_weaver::Version{"1", "0", "0", "alpha", "001"}},
    {"1.0.0+20130313144700",
     version_weaver::Version{"1", "0", "0", "", "20130313144700"}},
    {"1.0.0-beta+exp.sha.5114f85",
     version_weaver::Version{"1", "0", "0", "beta", "exp.sha.5114f85"}},
    {"1.0.0+21AF26D3----117B344092BD",
     version_weaver::Version{"1", "0", "0", "", "21AF26D3----117B344092BD"}},

};

TEST(basictests, parse) {
  for (const auto& [input, expected] : parse_values) {
    auto parse_result = version_weaver::parse(input);
    ASSERT_EQ(parse_result.has_value(), expected.has_value());
    if (parse_result.has_value()) {
      ASSERT_EQ(parse_result->major, expected->major);
      ASSERT_EQ(parse_result->minor, expected->minor);
      ASSERT_EQ(parse_result->patch, expected->patch);
    } else {
      ASSERT_EQ(parse_result.error(), expected.error());
    }
  }

  SUCCEED();
}

// A normal version number MUST take the form X.Y.Z
// where X, Y, and Z are non-negative integers, and
// MUST NOT contain leading zeroes.
TEST(basictests, leading_zeroes) {
  ASSERT_FALSE(version_weaver::parse("0.0.0").has_value());
  ASSERT_FALSE(version_weaver::parse("01.0.0").has_value());
  ASSERT_FALSE(version_weaver::parse("1.01.0").has_value());
  ASSERT_FALSE(version_weaver::parse("1.0.01").has_value());
}

std::vector<TestData> clean_values = {
    {"1.0.0", version_weaver::Version{"1", "0", "0"}},
    {"  =v1.2.3   ", version_weaver::Version{"1", "2", "3"}},
    {"", std::unexpected(version_weaver::ParseError::INVALID_INPUT)},
};

TEST(basictests, clean) {
  for (const auto& [input, expected] : clean_values) {
    auto cleaned_result = version_weaver::clean(input);
    ASSERT_EQ(cleaned_result.has_value(), expected.has_value());
    if (cleaned_result.has_value()) {
      ASSERT_EQ(cleaned_result->major, expected->major);
      ASSERT_EQ(cleaned_result->minor, expected->minor);
      ASSERT_EQ(cleaned_result->patch, expected->patch);
    } else {
      ASSERT_EQ(cleaned_result.error(), expected.error());
    }
  }
}
