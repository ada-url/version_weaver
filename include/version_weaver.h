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
bool gt(std::string_view version1, std::string_view version2);
bool lt(std::string_view version1, std::string_view version2);
bool satisfies(std::string_view version, std::string_view range);
std::string coerce(std::string_view version);
std::string minimum(std::string_view range);
std::string clean(std::string_view range);

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

std::expected<Version, ParseError> parse(std::string_view version);
}  // namespace version_weaver

inline bool operator!=(const version_weaver::Version& first,
                       const version_weaver::Version& second) {
  if (first.major != second.major) return first.major != second.major;
  if (first.minor != second.minor) return first.minor != second.minor;
  return first.patch != second.patch;
}
inline bool operator==(const version_weaver::Version& first,
                       const version_weaver::Version& second) {
  if (first.major != second.major) return first.major == second.major;
  if (first.minor != second.minor) return first.minor == second.minor;
  return first.patch == second.patch;
}
/*
inline auto operator<=>(const version_weaver::Version& first, const
version_weaver::Version& second) { if (first.major != second.major) return
first.major <=> second.major; if (first.minor != second.minor) return
first.minor <=> second.minor; return first.patch <=> second.patch;
}*/
#endif  // VERSION_WEAVER_H
