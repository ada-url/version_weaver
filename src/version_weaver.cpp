#include "version_weaver.h"
#include <algorithm>
#include <cctype>
namespace version_weaver {
bool validate(std::string_view version) { return bool(parse(version)); }

bool gt(std::string_view version1, std::string_view version2) { return true; }
bool lt(std::string_view version1, std::string_view version2) { return true; }
bool satisfies(std::string_view version, std::string_view range) {
  return true;
}
std::string coerce(std::string_view version) { return ""; }
std::string minimum(std::string_view range) { return ""; }
std::string clean(std::string_view range) { return ""; }

inline void trim_whitespace(std::string_view* input) noexcept {
  while (!input->empty() && std::isspace(input->front())) {
    input->remove_prefix(1);
  }
  while (!input->empty() && std::isspace(input->back())) {
    input->remove_suffix(1);
  }
}

std::expected<Version, ParseError> parse(std::string_view input) {
  if (input.size() > MAX_VERSION_LENGTH) {
    return std::unexpected(ParseError::VERSION_LARGER_THAN_MAX_LENGTH);
  }

  std::string_view input_copy = input;
  trim_whitespace(&input_copy);

  auto dot_iterator = input_copy.find('.');
  if (dot_iterator == std::string_view::npos) {
    // Only major exists. No minor or patch.
    return std::unexpected(ParseError::INVALID_INPUT);
  }
  Version version;
  auto major = input_copy.substr(0, dot_iterator);

  if (major.empty() || major.front() == '0') {
    // Version components can not have leading zeroes.
    return std::unexpected(ParseError::INVALID_INPUT);
  }
  version.major = major;
  input_copy = input_copy.substr(dot_iterator + 1);
  dot_iterator = input_copy.find('.');
  if (dot_iterator == std::string_view::npos) {
    // Only major and minor exists. No patch.
    return std::unexpected(ParseError::INVALID_INPUT);
  }

  auto minor = input_copy.substr(0, dot_iterator);
  if (minor.empty() || (minor.front() == '0' && minor.size() > 1)) {
    // Version components can not have leading zeroes.
    return std::unexpected(ParseError::INVALID_INPUT);
  }
  version.minor = minor;
  input_copy = input_copy.substr(dot_iterator + 1);
  dot_iterator = input_copy.find_first_of("-+");
  auto patch = (dot_iterator == std::string_view::npos)
                   ? input_copy
                   : input_copy.substr(0, dot_iterator);
  if (patch.empty() || (patch.front() == '0' && patch.size() > 1)) {
    return std::unexpected(ParseError::INVALID_INPUT);
  }
  version.patch = patch;
  if (dot_iterator == std::string_view::npos) {
    return version;
  }
  bool is_pre_release = input_copy[dot_iterator] == '-';
  input_copy = input_copy.substr(dot_iterator + 1);
  if (is_pre_release) {
    dot_iterator = input_copy.find('+');
    auto prerelease = (dot_iterator == std::string_view::npos)
                          ? input_copy
                          : input_copy.substr(0, dot_iterator);
    if (prerelease.empty()) {
      return std::unexpected(ParseError::INVALID_INPUT);
    }
    version.pre_release = prerelease;
    if (dot_iterator == std::string_view::npos) {
      return version;
    }
    input_copy = input_copy.substr(dot_iterator + 1);
  }
  if (input_copy.empty()) {
    return std::unexpected(ParseError::INVALID_INPUT);
  }
  version.build = input_copy;
  return version;
}

}  // namespace version_weaver