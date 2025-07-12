#include "version_weaver.h"
#include <algorithm>
#include <cctype>
#include <regex>
#include <charconv>

namespace version_weaver {
bool validate(std::string_view version) { return parse(version).has_value(); }

std::optional<std::string> coerce(const std::string& version) {
  if (version.empty()) {
    return std::nullopt;
  }

  // Regular expression to match major, minor, and patch components
  std::regex semverRegex(R"((\d+)(?:\.(\d+))?(?:\.(\d+))?)");
  std::smatch match;

  if (std::regex_search(version, match, semverRegex)) {
    std::string major =
        std::to_string(std::stoll(match[1].str()));  // First number
    std::string minor = match[2].matched
                            ? std::to_string(std::stoll(match[2].str()))
                            : "0";  // Second number or "0"
    std::string patch = match[3].matched
                            ? std::to_string(std::stoll(match[3].str()))
                            : "0";  // Third number or "0"

    return major + "." + minor + "." + patch;
  }

  return std::nullopt;
}

std::string minimum(std::string_view range) { return ""; }

std::expected<std::string, parse_error> inc(version input,
                                            release_type release_type) {
  version_weaver::version result;
  std::string incremented;
  switch (release_type) {
    case MAJOR:
    case PRE_MAJOR: {
      int major_int;
      auto [ptr, ec] =
          std::from_chars(input.major.data(),
                          input.major.data() + input.major.size(), major_int);
      if (ec != std::errc()) {
        return std::unexpected(parse_error::INVALID_MAJOR);
      }
      auto incremented_major_int = major_int + 1;
      incremented = std::move(std::to_string(incremented_major_int));
      result = version_weaver::version{incremented, "0", "0"};
      break;
    }
    case MINOR:
    case PRE_MINOR: {
      int minor_int;
      auto [ptr, ec] =
          std::from_chars(input.minor.data(),
                          input.minor.data() + input.minor.size(), minor_int);
      if (ec != std::errc()) {
        return std::unexpected(parse_error::INVALID_MINOR);
      }
      auto incremented_minor_int = minor_int + 1;
      incremented = std::move(std::to_string(incremented_minor_int));
      result = version_weaver::version{input.major, incremented, "0"};
      break;
    }
    case PATCH:
    case PRE_PATCH: {
      if (input.pre_release && release_type != PRE_PATCH) {
        return version_weaver::version{input.major, input.minor, input.patch};
      }
      int patch_int;
      auto [ptr, ec] =
          std::from_chars(input.patch.data(),
                          input.patch.data() + input.patch.size(), patch_int);
      if (ec != std::errc()) {
        return std::unexpected(parse_error::INVALID_PATCH);
      }
      auto incremented_patch_int = patch_int + 1;
      incremented = std::move(std::to_string(incremented_patch_int));
      result = version_weaver::version{input.major, input.minor, incremented};
      break;
    }
    case RELEASE: {
      if (!input.pre_release.has_value()) {
        return std::unexpected(parse_error::INVALID_INPUT);
      }
      return version_weaver::version{input.major, input.minor, input.patch};
    };
    default:
      return std::unexpected(parse_error::INVALID_RELEASE_TYPE);
  }

  if (release_type == PRE_MAJOR || release_type == PRE_MINOR ||
      release_type == PRE_PATCH) {
    result.pre_release = "0";
  }

  return result;
}

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

std::expected<version, parse_error> clean(std::string_view input) {
  std::string_view range = input;
  trim_whitespace(&range);
  if (range.empty()) return std::unexpected(parse_error::INVALID_INPUT);

  // Trim any leading value expect = and v.
  while (!range.empty() && (range.front() == '=' || range.front() == 'v')) {
    range.remove_prefix(1);
  }

  // If range starts with a non-digit character, it is invalid.
  if (!range.empty() && !std::isdigit(range.front())) {
    return std::unexpected(parse_error::INVALID_INPUT);
  }

  return parse(range);
}

std::expected<version, parse_error> parse(std::string_view input) {
  if (input.size() > MAX_VERSION_LENGTH) {
    return std::unexpected(parse_error::VERSION_LARGER_THAN_MAX_LENGTH);
  }

  std::string_view input_copy = input;
  trim_whitespace(&input_copy);

  auto dot_iterator = input_copy.find('.');
  if (dot_iterator == std::string_view::npos) {
    // Only major exists. No minor or patch.
    return std::unexpected(parse_error::INVALID_INPUT);
  }
  version version;
  auto major = input_copy.substr(0, dot_iterator);

  if (major.empty() || major.front() == '0') {
    // Version components can not have leading zeroes.
    return std::unexpected(parse_error::INVALID_INPUT);
  }
  if (!contains_only_digits(major)) {
    return std::unexpected(parse_error::INVALID_INPUT);
  }
  version.major = major;
  input_copy = input_copy.substr(dot_iterator + 1);
  dot_iterator = input_copy.find('.');
  if (dot_iterator == std::string_view::npos) {
    // Only major and minor exists. No patch.
    return std::unexpected(parse_error::INVALID_INPUT);
  }

  auto minor = input_copy.substr(0, dot_iterator);
  if (minor.empty() || (minor.front() == '0' && minor.size() > 1)) {
    // Version components can not have leading zeroes.
    return std::unexpected(parse_error::INVALID_INPUT);
  }
  if (!contains_only_digits(minor)) {
    return std::unexpected(parse_error::INVALID_INPUT);
  }
  version.minor = minor;
  input_copy = input_copy.substr(dot_iterator + 1);
  dot_iterator = input_copy.find_first_of("-+");
  auto patch = (dot_iterator == std::string_view::npos)
                   ? input_copy
                   : input_copy.substr(0, dot_iterator);
  if (patch.empty() || (patch.front() == '0' && patch.size() > 1)) {
    return std::unexpected(parse_error::INVALID_INPUT);
  }
  if (!contains_only_digits(patch)) {
    return std::unexpected(parse_error::INVALID_INPUT);
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
      return std::unexpected(parse_error::INVALID_INPUT);
    }
    version.pre_release = prerelease;
    if (dot_iterator == std::string_view::npos) {
      return version;
    }
    input_copy = input_copy.substr(dot_iterator + 1);
  }
  if (input_copy.empty()) {
    return std::unexpected(parse_error::INVALID_INPUT);
  }
  version.build = input_copy;
  return version;
}

}  // namespace version_weaver