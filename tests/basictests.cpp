#include "version_weaver.h"
#include <vector>

#include <gtest/gtest.h>

std::vector<std::pair<std::string, std::expected<version_weaver::Version,
                                                 version_weaver::ParseError>>>
    test_values = {
        {"1.0.0", version_weaver::Version{"1", "0", "0"}},
        {"1.0.0-alpha", version_weaver::Version{"1", "0", "0", "alpha"}},
        {"1.0.0-alpha.1", version_weaver::Version{"1", "0", "0", "alpha.1"}},
        {"1.0.0-0.3.7", version_weaver::Version{"1", "0", "0", "0.3.7"}},
        {"1.0.0-x.7.z.92", version_weaver::Version{"1", "0", "0", "x.7.z.92"}},
        {"1.0.0-x-y-z.--", version_weaver::Version{"1", "0", "0", "x-y-z.--"}},
        {"1.0.0-alpha+001",
         version_weaver::Version{"1", "0", "0", "alpha", "001"}},
        {"1.0.0+20130313144700",
         version_weaver::Version{"1", "0", "0", "", "20130313144700"}},
        {"1.0.0-beta+exp.sha.5114f85",
         version_weaver::Version{"1", "0", "0", "beta", "exp.sha.5114f85"}},
        {"1.0.0+21AF26D3----117B344092BD",
         version_weaver::Version{"1", "0", "0", "",
                                 "21AF26D3----117B344092BD"}},
};

TEST(basictests, parse) {
  for (const auto& [input, expected] : test_values) {
    auto parse_result = version_weaver::parse(input);
    ASSERT_EQ(parse_result.has_value(), expected.has_value());
    ASSERT_EQ(parse_result->major, expected->major);
    ASSERT_EQ(parse_result->minor, expected->minor);
    ASSERT_EQ(parse_result->patch, expected->patch);
  }

  SUCCEED();
}
