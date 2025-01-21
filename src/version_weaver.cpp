#include "version_weaver.h"
#include <algorithm>
#include <cctype>
#include <regex>
#include <iostream>

namespace version_weaver {
bool validate(std::string_view version) { return parse(version).has_value(); }

std::string coerce(const std::string& version) {
  if (version.empty() || std::all_of(version.begin(), version.end(), isspace)) {
    return "0.0.0";
  }

  std::regex semverRegex(R"((\d+)(?:\.(\d+))?(?:\.(\d+))?)");
  std::smatch match;

  if (std::regex_search(version, match, semverRegex)) {
    std::string major = match[1].str();  // First number
    std::string minor = match[2].matched ? match[2].str()
                                         : "0";  // Second number or default "0"
    std::string patch =
        match[3].matched ? match[3].str() : "0";  // Third number or default "0"

    return major + "." + minor + "." + patch;
  }

  // Return “0.0.0.0” by default if no valid semver is found
  return "0.0.0";
}

std::string minimum(std::string_view range) { return ""; }

constexpr inline void trim_whitespace(std::string_view* input) noexcept {
  while (!input->empty() && std::isspace(input->front())) {
    input->remove_prefix(1);
  }
  while (!input->empty() && std::isspace(input->back())) {
    input->remove_suffix(1);
  }
}

constexpr inline bool contains_only_digits(std::string_view input) noexcept {
  // Optimization opportunity: Replace this with a hash table lookup.
  return input.find_first_not_of("0123456789") == std::string_view::npos;
}

std::expected<Version, ParseError> clean(std::string_view input) {
  std::string_view range = input;
  trim_whitespace(&range);
  if (range.empty()) return std::unexpected(ParseError::INVALID_INPUT);

  // Trim any leading value expect = and v.
  while (!range.empty() && (range.front() == '=' || range.front() == 'v')) {
    range.remove_prefix(1);
  }

  // If range starts with a non-digit character, it is invalid.
  if (!range.empty() && !std::isdigit(range.front())) {
    return std::unexpected(ParseError::INVALID_INPUT);
  }

  return parse(range);
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
  if (!contains_only_digits(major)) {
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
  if (!contains_only_digits(minor)) {
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
  if (!contains_only_digits(patch)) {
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