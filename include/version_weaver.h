#ifndef VERSION_WEAVER_H
#define VERSION_WEAVER_H
#include <optional>
#include <string>
#include <string_view>
#include <expected>

namespace version_weaver {

// https://semver.org/#does-semver-have-a-size-limit-on-the-version-string
static constexpr size_t MAX_VERSION_LENGTH = 256;

// Validate a version string.
// A valid version string MUST be a non-empty string of characters that
// conform to the grammar:
// version       ::= major '.' minor '.' patch [ '-' pre-release ] [ '+' build ]
// major         ::= non-zero-digit *digit
// minor         ::= non-zero-digit *digit
// patch         ::= non-zero-digit *digit
// pre-release   ::= identifier *('.' identifier)
// identifier    ::= non-zero-digit *digit / alpha / alpha-numeric
// build         ::= identifier *('.' identifier)
// non-zero-digit ::= '1' / '2' / '3' / '4' / '5' / '6' / '7' / '8' / '9'
// digit         ::= '0' / non-zero-digit
bool validate(std::string_view version);

bool satisfies(std::string_view version, std::string_view range);
std::optional<std::string> coerce(const std::string_view version);
std::optional<std::string> incrementVersion(std::string_view version);
std::optional<std::string> decrementVersion(std::string_view version);
std::optional<std::string> minimum(std::string_view range);

// A normal version number MUST take the form X.Y.Z where X, Y, and Z are
// non-negative integers, and MUST NOT contain leading zeroes.
// X is the major version, Y is the minor version, and Z is the patch version.
// Each element MUST increase numerically.
// For instance: 1.9.0 -> 1.10.0 -> 1.11.0.
struct version {
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

  inline operator std::string() const {
    std::string result = std::string(major) + "." + std::string(minor) + "." +
                         std::string(patch);
    if (pre_release.has_value()) {
      result += "-" + std::string(pre_release.value());
    }
    if (build.has_value()) {
      result += "+" + std::string(build.value());
    }
    return result;
  }
};

enum parse_error {
  VERSION_LARGER_THAN_MAX_LENGTH,
  INVALID_INPUT,
  INVALID_MAJOR,
  INVALID_MINOR,
  INVALID_PATCH,
  INVALID_RELEASE_TYPE,
};

// This will return a cleaned and trimmed semver version.
// If the provided version is not valid a null will be returned.
// This does not work for ranges.
std::expected<version, parse_error> clean(std::string_view input);

std::expected<version, parse_error> parse(std::string_view version);

enum release_type {
  MAJOR,
  MINOR,
  PATCH,
  // TODO: also support
  //  - PRE_MAJOR
  //  - PRE_MINOR
  //  - PRE_PATCH
  //  - PRE_RELEASE
  //  - RELEASE
};

// Increment the version according to the provided release type.
std::expected<std::string, parse_error> inc(version input,
                                            release_type release_type);

inline std::expected<std::string, parse_error> increment(
    std::string_view input, release_type release_type) {
  auto parts = parse(input);
  if (!parts.has_value()) {
    return std::unexpected(parts.error());
  }
  return inc(parts.value(), release_type);
}

inline std::expected<std::string, parse_error> operator+(
    std::string_view lhs, const release_type& rhs) {
  return increment(lhs, rhs);
}

inline std::expected<std::string, parse_error> operator+(
    const std::string& lhs, const release_type& rhs) {
  return operator+(std::string_view(lhs), rhs);
}

}  // namespace version_weaver

// https://semver.org/#spec-item-11
inline auto operator<=>(const version_weaver::version& first,
                        const version_weaver::version& second) {
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