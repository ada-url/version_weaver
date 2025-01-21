#include "version_weaver.h"
#include <format>
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
     version_weaver::Version{"1", "0", "0", std::nullopt, "20130313144700"}},
    {"1.0.0-beta+exp.sha.5114f85",
     version_weaver::Version{"1", "0", "0", "beta", "exp.sha.5114f85"}},
    {"1.0.0+21AF26D3----117B344092BD",
     version_weaver::Version{"1", "0", "0", std::nullopt,
                             "21AF26D3----117B344092BD"}},

};

TEST(basictests, parse) {
  for (const auto& [input, expected] : parse_values) {
    auto parse_result = version_weaver::parse(input);
    ASSERT_EQ(parse_result.has_value(), expected.has_value());
    if (parse_result.has_value()) {
      ASSERT_EQ(parse_result->major, expected->major);
      ASSERT_EQ(parse_result->minor, expected->minor);
      ASSERT_EQ(parse_result->patch, expected->patch);
      ASSERT_EQ(parse_result->pre_release, expected->pre_release);
      ASSERT_EQ(parse_result->build, expected->build);
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
    {"1.2.3", version_weaver::Version{"1", "2", "3"}},
    {" 1.2.3 ", version_weaver::Version{"1", "2", "3"}},
    {" 1.2.3-4 ", version_weaver::Version{"1", "2", "3", "4"}},
    {" 1.2.3-pre ", version_weaver::Version{"1", "2", "3", "pre"}},
    {"  =v1.2.3   ", version_weaver::Version{"1", "2", "3"}},
    {"v1.2.3", version_weaver::Version{"1", "2", "3"}},
    {" v1.2.3 ", version_weaver::Version{"1", "2", "3"}},
    {"\t1.2.3", version_weaver::Version{"1", "2", "3"}},
    {">1.2.3", std::unexpected(version_weaver::ParseError::INVALID_INPUT)},
    {"~1.2.3", std::unexpected(version_weaver::ParseError::INVALID_INPUT)},
    {"<=1.2.3", std::unexpected(version_weaver::ParseError::INVALID_INPUT)},
    {"1.2.x", std::unexpected(version_weaver::ParseError::INVALID_INPUT)},
};

TEST(basictests, clean) {
  for (const auto& [input, expected] : clean_values) {
    auto cleaned_result = version_weaver::clean(input);
    std::printf("input: %s\n", input.c_str());
    ASSERT_EQ(cleaned_result.has_value(), expected.has_value());
    if (cleaned_result.has_value()) {
      ASSERT_EQ(cleaned_result->major, expected->major);
      ASSERT_EQ(cleaned_result->minor, expected->minor);
      ASSERT_EQ(cleaned_result->patch, expected->patch);
      ASSERT_EQ(cleaned_result->pre_release, expected->pre_release);
      ASSERT_EQ(cleaned_result->build, expected->build);
    } else {
      ASSERT_EQ(cleaned_result.error(), expected.error());
    }
  }
}

using OrderingData = std::tuple<std::string, std::string, std::strong_ordering>;

std::vector<OrderingData> ordering_values = {
    {"1.0.0", "1.0.0", std::strong_ordering::equal},
    {"1.0.0", "1.0.1", std::strong_ordering::less},
    {"1.0.0", "1.1.0", std::strong_ordering::less},
    {"1.0.0", "2.0.0", std::strong_ordering::less},
    {"1.0.0", "1.0.0-alpha", std::strong_ordering::greater},
    {"1.0.0-alpha", "1.0.0-alpha", std::strong_ordering::equal},
    {"1.0.0-alpha", "1.0.0-alpha.1", std::strong_ordering::less},
    {"1.0.0-alpha.1", "1.0.0-beta", std::strong_ordering::less},
    {"1.0.0-beta", "1.0.0-beta.2", std::strong_ordering::less},
    {"1.0.0-beta.2", "1.0.0-beta.11", std::strong_ordering::greater},
    {"1.0.0-beta.11", "1.0.0-rc.1", std::strong_ordering::less},
    {"1.0.0-rc.1", "1.0.0", std::strong_ordering::less},
};

TEST(basictests, order) {
  for (const auto& [view1, view2, order] : ordering_values) {
    auto v1 = version_weaver::parse(view1).value();
    auto v2 = version_weaver::parse(view2).value();
    ASSERT_EQ(v1 <=> v2, order);
  }
}

using CoerceData = std::pair<std::string, std::string>;
std::vector<CoerceData> coerce_values = {
    {"001", "1.0.0"},
    {"01.002.03", "1.2.3"},
    {"000.000.000", "0.0.0"},
    {"11111111111111111", "11111111111111111.0.0"},
    {"999999999.999999.999999", "999999999.999999.999999"},
    {"0.0.01", "0.0.1"},
    {"v001", "1.0.0"},
    {"v01.002.03", "1.2.3"},
    {"1.2.3", "1.2.3"},
    {"v2", "2.0.0"},
    {" 1.2.3 ", "1.2.3"},
    {"1.2.3.4", "1.2.3"},
    {"v1.2.3", "1.2.3"},
    {"=1.2.3", "1.2.3"},
    {"1.2", "1.2.0"},
    {"1", "1.0.0"},
    {"1.2.x", "1.2.0"},
    {"alpha1.2.3", "1.2.3"},
    {"", "0.0.0"},
    {".1", "1.0.0"},
    {".1.", "1.0.0"},
    {"..1", "1.0.0"},
    {".1.1", "1.1.0"},
    {"1.", "1.0.0"},
    {"1.0", "1.0.0"},
    {"1.0.0", "1.0.0"},
    {"0", "0.0.0"},
    {"0.0", "0.0.0"},
    {"0.0.0", "0.0.0"},
    {"0.1", "0.1.0"},
    {"0.0.1", "0.0.1"},
    {"0.1.1", "0.1.1"},
    {"1", "1.0.0"},
    {"1.2", "1.2.0"},
    {"1.2.3", "1.2.3"},
    {"1.2.3.4", "1.2.3"},
    {"13", "13.0.0"},
    {"35.12", "35.12.0"},
    {"35.12.18", "35.12.18"},
    {"35.12.18.24", "35.12.18"},
    {"v1", "1.0.0"},
    {"v1.2", "1.2.0"},
    {"v1.2.3", "1.2.3"},
    {"v1.2.3.4", "1.2.3"},
    {" 1", "1.0.0"},
    {"1 ", "1.0.0"},
    {"1 0", "1.0.0"},
    {"1 1", "1.0.0"},
    {"1.1 1", "1.1.0"},
    {"1.1-1", "1.1.0"},
    {"a1", "1.0.0"},
    {"a1a", "1.0.0"},
    {"1a", "1.0.0"},
    {"version 1", "1.0.0"},
    {"version1", "1.0.0"},
    {"version1.0", "1.0.0"},
    {"version1.1", "1.1.0"},
    {"42.6.7.9.3-alpha", "42.6.7"},
    {"v2", "2.0.0"},
    {"v3.4 replaces v3.3.1", "3.4.0"},
    {"4.6.3.9.2-alpha2", "4.6.3"}};

TEST(basictests, coerce) {
  for (const auto& [input, expected] : coerce_values) {
    auto result = version_weaver::coerce(input);
    std::printf("input: '%s', expected: '%s', result: '%s'\n", input.c_str(),
                expected.c_str(), result.c_str());
    ASSERT_EQ(result, expected);
  }
}
