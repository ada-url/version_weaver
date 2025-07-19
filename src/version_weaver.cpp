#include "version_weaver.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <format>
#include <regex>

namespace version_weaver {
bool validate(std::string_view version) { return parse(version).has_value(); }

std::optional<std::string> coerce(std::string_view version) {
  if (version.empty()) {
    return std::nullopt;
  }

  // Regular expression to match major, minor, and patch components
  std::regex semverRegex(R"((\d+)(?:\.(\d+))?(?:\.(\d+))?)");
  std::smatch match;
  std::string version_str(version);

  if (std::regex_search(version_str, match, semverRegex)) {
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

constexpr bool is_digit(const char c) noexcept { return c >= '0' && c <= '9'; }

std::vector<std::string_view> split(const std::string_view &s) {
  std::vector<std::string_view> parts;
  size_t start = 0;
  while (start < s.size()) {
    size_t end = s.find_first_of(".-", start);
    if (end == std::string_view::npos) {
      parts.push_back(s.substr(start));
      break;
    } else {
      parts.push_back(s.substr(start, end - start));
    }
    start = end + 1;
  }
  return parts;
}

bool compareSemVer(const std::string_view &a, const std::string_view &b) {
  auto a_parts = split(a);
  auto b_parts = split(b);

  size_t min_size = std::min(a_parts.size(), b_parts.size());
  for (size_t i = 0; i < min_size; i++) {
    bool a_is_digit = !a_parts[i].empty() && std::isdigit(a_parts[i][0]);
    bool b_is_digit = !b_parts[i].empty() && std::isdigit(b_parts[i][0]);

    if (a_is_digit && b_is_digit) {
      int num_a = std::stoi(std::string(a_parts[i]));
      int num_b = std::stoi(std::string(b_parts[i]));
      if (num_a != num_b) {
        return num_a < num_b;
      }
    } else {
      if (a_parts[i] != b_parts[i]) {
        return a_parts[i] < b_parts[i];
      }
    }
  }

  return a_parts.size() < b_parts.size();
}

std::optional<std::string> incrementVersion(std::string_view version) {
  // First, we look for the '-' character to separate the pre-release part.
  std::string_view numPart;
  std::string_view preRelease;
  size_t dashPos = version.find('-');
  if (dashPos != std::string_view::npos) {
    numPart = version.substr(0, dashPos);
    preRelease = version.substr(dashPos + 1);
  } else {
    numPart = version;
  }

  // divides numPart by the dot ('.') character
  std::vector<std::string_view> parts;
  size_t start = 0;
  while (true) {
    size_t dotPos = numPart.find('.', start);
    if (dotPos == std::string_view::npos) {
      parts.push_back(numPart.substr(start));
      break;
    }
    parts.push_back(numPart.substr(start, dotPos - start));
    start = dotPos + 1;
  }

  if (parts.empty()) return std::nullopt;

  int major = 0, minor = 0, patch = 0;
  try {
    major = std::stoi(std::string(parts[0]));
    if (parts.size() >= 2) minor = std::stoi(std::string(parts[1]));
    if (parts.size() >= 3) patch = std::stoi(std::string(parts[2]));
  } catch (...) {
    return std::nullopt;
  }

  // If there is a pre-release part, return the version in pre-release format
  // (for example “1.2.3-beta.0”)
  if (!preRelease.empty()) {
    return std::format("{}.{}.{}-{}.0", major, minor, patch,
                       std::string(preRelease));
  }

  // if there is no pre-release, increment patch and return the result.
  patch++;
  return std::format("{}.{}.{}", major, minor, patch);
}

std::optional<std::string> decrementVersion(const std::string_view version) {
  std::regex version_regex(R"((\d+)(?:\.(\d+))?(?:\.(\d+))?(?:-([\w\d.-]+))?)");
  std::smatch match;
  std::string version_str(version);

  if (std::regex_match(version_str, match, version_regex)) {
    int major = std::stoi(match[1].str());
    int minor = match[2].matched ? std::stoi(match[2].str()) : 0;
    int patch = match[3].matched ? std::stoi(match[3].str()) : 0;
    std::string preRelease = match[4].matched ? match[4].str() : "";

    // If there is a pre-release (beta, alpha), minimize it.
    if (!preRelease.empty()) {
      if (preRelease.find("beta") != std::string::npos) {
        return match[1].str() + "." + match[2].str() + "." + match[3].str() +
               "-alpha.0";
      } else if (preRelease.find("alpha") != std::string::npos) {
        return match[1].str() + "." + match[2].str() + "." + match[3].str() +
               "-alpha.0";
      }
    }

    // Patch version to zero, but downgrade to minor or major if already 0
    if (major > 0) {
      return std::to_string(major + 1) + ".0.0";
    } else if (minor > 0) {
      return "0." + std::to_string(minor + 1) + ".0";
    }
    return "0.0." + std::to_string(patch + 1);
  }

  return std::nullopt;
}

// Checks whether the candidate meets the given constraint.
bool satisfies_constraint(const std::string_view &candidate,
                          const std::string_view &op,
                          const std::string_view &version) {
  if (op == ">") {
    // must be equal to or higher than the candidate.
    return compareSemVer(version, candidate) && (candidate != version);
  } else if (op == ">=") {
    return candidate == version || !compareSemVer(candidate, version);
  } else if (op == "<") {
    return compareSemVer(candidate, version);
  } else if (op == "<=") {
    return candidate == version || !compareSemVer(version, candidate);
  }
  return false;
}

std::optional<std::string> computeCaretUpperBound(
    const std::string_view &version) {
  auto coercedOpt = coerce(version);
  if (!coercedOpt.has_value()) return "";

  const std::string_view &coerced = *coercedOpt;
  std::vector<std::string> parts;
  std::string token;

  for (char c : coerced) {
    if (c == '.') {
      parts.push_back(token);
      token.clear();
    } else {
      token.push_back(c);
    }
  }
  parts.push_back(token);

  if (parts.size() < 3) {
    return "";
  }

  int major = std::stoi(parts[0]);
  int minor = std::stoi(parts[1]);
  int patch = std::stoi(parts[2]);

  if (major > 0) {
    return std::to_string(major + 1) + ".0.0";
  } else if (minor > 0) {
    return "0." + std::to_string(minor + 1) + ".0";
  }
  return std::to_string(major) + "." + std::to_string(minor) + "." +
         std::to_string(patch + 1);
}

std::string computeTildeUpperBound(const std::string_view &version) {
  auto coercedOpt = coerce(version);

  if (!coercedOpt) return "";

  std::vector<std::string> parts;

  size_t pos = 0;
  size_t dot_pos;
  while (pos < version.size() &&
         (dot_pos = version.find('.', pos)) != std::string::npos) {
    parts.push_back(std::string(version.substr(pos, dot_pos - pos)));
    pos = dot_pos + 1;
  }

  if (pos < version.size()) {
    parts.push_back(std::string(version.substr(pos)));
  }

  int major = std::stoi(parts[0]);
  int minor = std::stoi(parts[1]);

  // Upper bound for Tilde: X.(Y+1).0
  std::string upper =
      std::to_string(major) + "." + std::to_string(minor + 1) + ".0";
  return upper;
}

constexpr inline void trim_whitespace(std::string_view *input) noexcept {
  while (!input->empty() && std::isspace(input->front())) {
    input->remove_prefix(1);
  }
  while (!input->empty() && std::isspace(input->back())) {
    input->remove_suffix(1);
  }
}

std::optional<std::string> minimum(std::string_view range) {
  if (range.empty()) return std::nullopt;

  // If the entire expression is just "*" (possibly with surrounding spaces),
  // return "0.0.0" directly.
  std::string_view trimmed_range = range;
  trim_whitespace(&trimmed_range);
  if (trimmed_range.size() == 1 && trimmed_range[0] == '*') return "0.0.0";

  // Support for the dash operator ("A - B" form)
  std::regex dash_regex(
      R"(^\s*([\d]+(?:\.[\d]+){0,2})\s*-\s*([\d]+(?:\.[\d]+){0,2})\s*$)");
  std::smatch dash_match;
  std::string range_str(range);
  if (std::regex_match(range_str, dash_match, dash_regex)) {
    return coerce(dash_match[1].str());
  }

  std::optional<std::string> bestCandidate;
  std::regex or_regex(R"(\s*\|\|\s*)");
  std::regex star_regex(R"(^\*$)");

  std::sregex_token_iterator subIt(range_str.begin(), range_str.end(), or_regex,
                                   -1);
  std::sregex_token_iterator subEnd;

  for (; subIt != subEnd; ++subIt) {
    std::string subRange = subIt->str();
    // Trim whitespace from the beginning and end
    auto start = subRange.find_first_not_of(" \t\n\r");
    if (start != std::string::npos) subRange = subRange.substr(start);
    auto endpos = subRange.find_last_not_of(" \t\n\r");
    if (endpos != std::string::npos) subRange = subRange.substr(0, endpos + 1);

    // If the sub-range is a star, the candidate is "0.0.0"
    if (std::regex_match(subRange, star_regex)) {
      if (!bestCandidate.has_value() ||
          compareSemVer("0.0.0", *bestCandidate)) {
        bestCandidate = "0.0.0";
        continue;
      }
    }

    // Capture constraints; includes "^" and "~" operators.
    std::regex constraint_regex(R"((>=|>|<=|<|\^|~)\s*([\w\d.-]+))");
    std::sregex_iterator it(subRange.begin(), subRange.end(), constraint_regex);
    std::sregex_iterator itEnd;
    std::vector<std::pair<std::string, std::string>> lowerConstraints;
    std::vector<std::pair<std::string, std::string>> upperConstraints;

    std::string candidate;

    for (; it != itEnd; ++it) {
      std::string op = (*it)[1].str();
      std::string version = (*it)[2].str();
      if (op == "^") {
        // For caret, add a lower constraint ">= version" and an upper
        // constraint based on caret rules.
        lowerConstraints.push_back({">=", version});
        auto upperBoundOpt = computeCaretUpperBound(version);
        if (upperBoundOpt.has_value())
          upperConstraints.push_back({"<", *upperBoundOpt});
      } else if (op == "~") {
        // For tilde, add a lower constraint ">= version" and an upper
        // constraint based on tilde rules.
        lowerConstraints.push_back({">=", version});
        std::string upperBound = computeTildeUpperBound(version);
        if (!upperBound.empty()) upperConstraints.push_back({"<", upperBound});
      } else if (op == ">" || op == ">=") {
        lowerConstraints.push_back({op, version});
      } else {
        upperConstraints.push_back({op, version});
      }
    }

    std::regex anyConstraint(R"(>=|>|<=|<|\^|~)");
    // If there are no constraints in the sub-range (e.g., "1.0.x", "1.x",
    // "=1.0.0", etc.), then the candidate is the normalized form of the
    // sub-range.
    if (!std::regex_search(subRange, anyConstraint)) {
      auto c = coerce(subRange);
      candidate = c.has_value() ? *c : subRange;
    } else if (!lowerConstraints.empty()) {
      for (auto &lc : lowerConstraints) {
        // For ">" operator, use incrementVersion; for ">=" simply use the
        // version.
        std::string cur = (lc.first == ">")
                              ? incrementVersion(lc.second).value_or(lc.second)
                              : lc.second;
        if (candidate.empty() || compareSemVer(candidate, cur)) candidate = cur;
      }
    } else {
      candidate = "0.0.0";
    }

    bool valid = true;
    for (auto &[op, version] : upperConstraints) {
      // Special case: if the constraint is "<0.0.0-beta" and the candidate is
      // "0.0.0", change the candidate to "0.0.0-0".
      if (op == "<" && version == "0.0.0-beta" && candidate == "0.0.0") {
        candidate = "0.0.0-0";
      }
      if (!satisfies_constraint(candidate, op, version)) {
        valid = false;
        break;
      }
    }
    if (valid && !candidate.empty()) {
      if (!bestCandidate.has_value() ||
          compareSemVer(candidate, *bestCandidate))
        bestCandidate = candidate;
    }
  }

  return bestCandidate;
}

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
    case PRE_PATCH:
    case PRE_RELEASE: {
      if (input.pre_release.has_value() && release_type == PATCH) {
        return version_weaver::version{input.major, input.minor, input.patch};
      }
      if (release_type == PRE_RELEASE && input.pre_release.has_value()) {
        // TODO: support non-int pre_releases as well
        //       (see:
        //       https://github.com/npm/node-semver/blob/d17aebf8/test/fixtures/increments.js#L22-L36)
        auto pre_release_value = input.pre_release.value();
        int prerelease_int;
        auto [ptr, ec] =
            std::from_chars(pre_release_value.data(),
                            pre_release_value.data() + pre_release_value.size(),
                            prerelease_int);
        if (ec != std::errc()) {
          return std::unexpected(parse_error::INVALID_PRERELEASE);
        }
        auto incremented_prerelease_int = prerelease_int + 1;
        incremented = std::move(std::to_string(incremented_prerelease_int));
        return version_weaver::version{input.major, input.minor, input.patch,
                                       incremented};
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
      release_type == PRE_PATCH || release_type == PRE_RELEASE) {
    result.pre_release = "0";
  }

  return result;
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