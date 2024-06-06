#include "version_weaver.h"
#include <cstdlib>
#include <vector>
#include <format>

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

bool test_parse() {
  for (const auto& [input, expected] : test_values) {
    auto result = version_weaver::parse(input);
    if (bool(result) != bool(expected)) {
      std::printf("Expected %d, got %d\n", bool(expected), bool(result));
      return false;
    }
    if (bool(result)) {
      if (result.value() != expected.value()) {
        std::printf("Expected \n");
        return false;
      }
    }
  }
  return true;
}
int main() {
  if (!test_parse()) {
    std::printf("Test failed\n");
    return EXIT_FAILURE;
  }
  std::printf("Tests succeeded!\n");
  return EXIT_SUCCESS;
}