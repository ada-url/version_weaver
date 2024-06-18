#ifndef VERSION_WEAVER_H
#define VERSION_WEAVER_H
#include <optional>
#include <string>
#include <string_view>
#include <expected>

namespace version_weaver {

// https://semver.org/#does-semver-have-a-size-limit-on-the-version-string
static constexpr size_t MAX_VERSION_LENGTH = 256;

bool validate(std::string_view version);

bool satisfies(std::string_view version, std::string_view range);
std::string coerce(std::string_view version);
std::string minimum(std::string_view range);

// A normal version number MUST take the form X.Y.Z where X, Y, and Z are
// non-negative integers, and MUST NOT contain leading zeroes.
// X is the major version, Y is the minor version, and Z is the patch version.
// Each element MUST increase numerically.
// For instance: 1.9.0 -> 1.10.0 -> 1.11.0.
struct Version {
  std::string_view major;
  std::string_view minor;
  std::string_view patch;

  // A pre-release version MAY be denoted by appending a hyphen and a series
  // of dot separated identifiers immediately following the patch version.
  // - Identifiers MUST comprise only ASCII alphanumerics and hyphens
  // [0-9A-Za-z-].
  // - Identifiers MUST NOT be empty.
  // - Numeric identifiers MUST NOT include leading zeroes.
  //
  // Examples: 1.0.0-alpha, 1.0.0-alpha.1, 1.0.0-0.3.7, 1.0.0-x.7.z.92, 1.0.0-x-y-z.--.
  std::optional<std::string_view> pre_release;

  // Build metadata MAY be denoted by appending a plus sign and a series of dot
  // separated identifiers immediately following the patch or pre-release
  // version.
  // - Identifiers MUST comprise only ASCII alphanumerics and hyphens
  // [0-9A-Za-z-].
  // - Identifiers MUST NOT be empty.
  // Build metadata MUST be ignored when determining version precedence.
  // Thus two versions that differ only in the build metadata, have the same
  // precedence.
  //
  // Examples: 1.0.0-alpha+001, 1.0.0+20130313144700, 1.0.0-beta+exp.sha.5114f85,
  // 1.0.0+21AF26D3----117B344092BD.
  std::optional<std::string_view> build;
};

enum ParseError {
  VERSION_LARGER_THAN_MAX_LENGTH,
  INVALID_INPUT,
};

// This will return a cleaned and trimmed semver version.
// If the provided version is not valid a null will be returned.
// This does not work for ranges.
std::expected<Version, ParseError> clean(std::string_view input);

std::expected<Version, ParseError> parse(std::string_view version);
}  // namespace version_weaver

// https://semver.org/#spec-item-11
inline auto operator<=>(const version_weaver::Version& first,
                        const version_weaver::Version& second) {
  auto number_string_compare = [](std::string_view first,
                                  std::string_view second) {
    if (first.size() > second.size()) {
      return std::strong_ordering::greater;
    } else if (first.size() < second.size()) {
      return std::strong_ordering::less;
    }
    for (size_t i = 0; i < first.size(); i++) {
      if (first[i] > second[i]) {
        return std::strong_ordering::greater;
      } else if (first[i] < second[i]) {
        return std::strong_ordering::less;
      }
    }
    return std::strong_ordering::equal;
  };
  if (first.major != second.major) {
    return number_string_compare(first.major, second.major);
  }
  if (first.minor != second.minor) {
    return number_string_compare(first.minor, second.minor);
  }
  if (first.patch != second.patch) {
    return number_string_compare(first.patch, second.patch);
  }
  if (second.pre_release.has_value() && !first.pre_release.has_value()) {
    return std::strong_ordering::greater;
  }
  if (first.pre_release.has_value() && !second.pre_release.has_value()) {
    return std::strong_ordering::less;
  }
  if (!first.pre_release.has_value() && !second.pre_release.has_value()) {
    return std::strong_ordering::equal;
  }
  if (first.pre_release.value() == second.pre_release.value()) {
    return std::strong_ordering::equal;
  }
  auto only_digits = [](std::string_view first) {
    for (auto c : first) {
      if (c < '0' || c > '9') {
        return false;
      }
    }
    return true;
  };
  bool first_numeric = only_digits(first.pre_release.value());
  bool second_numeric = only_digits(second.pre_release.value());
  if (first_numeric && !second_numeric) {
    return std::strong_ordering::greater;
  }
  if (!first_numeric && second_numeric) {
    return std::strong_ordering::less;
  }
  if (first_numeric && second_numeric) {
    return number_string_compare(first.pre_release.value(),
                                 second.pre_release.value());
  }
  size_t min_size = std::min(first.pre_release.value().size(),
                             second.pre_release.value().size());
  for (size_t i = 0; i < min_size; i++) {
    if (first.pre_release.value()[i] > second.pre_release.value()[i]) {
      return std::strong_ordering::greater;
    } else if (first.pre_release.value()[i] < second.pre_release.value()[i]) {
      return std::strong_ordering::less;
    }
  }
  return first.pre_release.value().size() <=> second.pre_release.value().size();
}

#endif  // VERSION_WEAVER_H