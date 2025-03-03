#include "version_weaver.h"
#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>

namespace version_weaver {
bool validate(std::string_view version) { return parse(version).has_value(); }

std::optional<std::string> coerce(const std::string &version) {
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

bool compareSemVer(const std::string_view &a, const std::string &b) {
  std::vector<std::string> a_parts, b_parts;
  std::regex re(R"([.-])");  // Split on `.` or `-`
  std::sregex_token_iterator it_a(a.begin(), a.end(), re, -1), end;
  std::sregex_token_iterator it_b(b.begin(), b.end(), re, -1);

  while (it_a != end) {
    a_parts.push_back(*it_a++);
  }
  while (it_b != end) {
    b_parts.push_back(*it_b++);
  }

  size_t min_size = std::min(a_parts.size(), b_parts.size());
  for (size_t i = 0; i < min_size; i++) {
    if (std::isdigit(a_parts[i][0]) && std::isdigit(b_parts[i][0])) {
      int num_a = std::stoi(a_parts[i]);
      int num_b = std::stoi(b_parts[i]);
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

std::string incrementVersion(const std::string &version) {
  std::regex version_regex(R"((\d+)(?:\.(\d+))?(?:\.(\d+))?(?:-([\w\d.-]+))?)");
  std::smatch match;

  if (std::regex_match(version, match, version_regex)) {
    int major = std::stoi(match[1].str());
    int minor = match[2].matched ? std::stoi(match[2].str()) : 0;
    int patch = match[3].matched ? std::stoi(match[3].str()) : 0;
    std::string preRelease = match[4].matched ? match[4].str() : "";

    if (!preRelease.empty()) {
      return match[1].str() + "." + match[2].str() + "." + match[3].str() +
             "-" + preRelease + ".0";
    }

    // Increase Patch version
    patch++;
    return std::to_string(major) + "." + std::to_string(minor) + "." +
           std::to_string(patch);
  }

  return version;  // If not a valid version, replace
}

std::string decrementVersion(const std::string &version) {
  std::regex version_regex(R"((\d+)(?:\.(\d+))?(?:\.(\d+))?(?:-([\w\d.-]+))?)");
  std::smatch match;

  if (std::regex_match(version, match, version_regex)) {
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
    if (patch > 0) {
      patch--;
    } else if (minor > 0) {
      minor--;
      patch = 9;  // Consider the previous minor version by default.
    } else if (major > 0) {
      major--;
      minor = 9;
      patch = 9;
    } else {
      return "0.0.0";
    }

    return std::to_string(major) + "." + std::to_string(minor) + "." +
           std::to_string(patch);
  }

  return version;  // If not valid, replace.
}

// Checks whether the candidate meets the given constraint.
bool satisfiesConstraint(const std::string &candidate, const std::string &op,
                         const std::string &ver) {
  if (op == ">=") {
    // must be equal to or higher than the candidate.
    return candidate == ver || !compareSemVer(candidate, ver);
  } else if (op == ">") {
    return compareSemVer(ver, candidate) && (candidate != ver);
  } else if (op == "<") {
    return compareSemVer(candidate, ver);
  } else if (op == "<=") {
    return candidate == ver || !compareSemVer(ver, candidate);
  }
  return false;
}

std::string computeCaretUpperBound(const std::string &version) {
  auto coercedOpt = coerce(version);
  if (!coercedOpt.has_value()) return "";
  std::string ver = *coercedOpt;
  std::vector<std::string> parts;
  std::istringstream iss(ver);
  std::string token;
  while (std::getline(iss, token, '.')) {
    parts.push_back(token);
  }
  int major = std::stoi(parts[0]);
  int minor = std::stoi(parts[1]);
  int patch = std::stoi(parts[2]);
  std::string upper;
  if (major > 0) {
    upper = std::to_string(major + 1) + ".0.0";
  } else if (minor > 0) {
    upper = "0." + std::to_string(minor + 1) + ".0";
  } else {
    upper = "0.0." + std::to_string(patch + 1);
  }
  return upper;
}

std::string computeTildeUpperBound(const std::string &version) {
  auto coercedOpt = coerce(version);
  if (!coercedOpt.has_value()) return "";
  std::string ver = *coercedOpt;  // For example "1.1.1"
  std::vector<std::string> parts;
  std::istringstream iss(ver);
  std::string token;
  while (std::getline(iss, token, '.')) {
    parts.push_back(token);
  }
  int major = std::stoi(parts[0]);
  int minor = std::stoi(parts[1]);
  // Upper bound for Tilde: X.(Y+1).0
  std::string upper =
      std::to_string(major) + "." + std::to_string(minor + 1) + ".0";
  return upper;
}

std::optional<std::string> minimum(const std::string &range) {
  if (range.empty()) return std::nullopt;

  // If the entire expression is just "*" (possibly with surrounding spaces),
  // return "0.0.0" directly.
  std::regex star_regex(R"(^\s*\*\s*$)");
  if (std::regex_match(range, star_regex)) return "0.0.0";

  // Support for the dash operator ("A - B" form)
  std::regex dash_regex(
      R"(^\s*([\d]+(?:\.[\d]+){0,2})\s*-\s*([\d]+(?:\.[\d]+){0,2})\s*$)");
  std::smatch dash_match;
  if (std::regex_match(range, dash_match, dash_regex)) {
    std::string lower = dash_match[1].str();
    auto coercedOpt = coerce(lower);
    if (coercedOpt.has_value())
      return coercedOpt;
    else
      return std::nullopt;
  }

  std::vector<std::string> validCandidates;
  std::regex or_regex(R"(\s*\|\|\s*)");
  std::sregex_token_iterator subIt(range.begin(), range.end(), or_regex, -1);
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
      validCandidates.push_back("0.0.0");
      continue;
    }

    // Capture constraints; includes "^" and "~" operators.
    std::regex constraint_regex(R"((>=|>|<=|<|\^|~)\s*([\w\d.-]+))");
    std::sregex_iterator it(subRange.begin(), subRange.end(), constraint_regex);
    std::sregex_iterator itEnd;
    std::vector<std::pair<std::string, std::string>> lowerConstraints;
    std::vector<std::pair<std::string, std::string>> upperConstraints;
    for (; it != itEnd; ++it) {
      std::string op = (*it)[1].str();
      std::string ver = (*it)[2].str();
      if (op == "^") {
        // For caret, add a lower constraint ">= ver" and an upper constraint
        // based on caret rules.
        lowerConstraints.push_back({">=", ver});
        std::string upperBound = computeCaretUpperBound(ver);
        if (!upperBound.empty()) upperConstraints.push_back({"<", upperBound});
      } else if (op == "~") {
        // For tilde, add a lower constraint ">= ver" and an upper constraint
        // based on tilde rules.
        lowerConstraints.push_back({">=", ver});
        std::string upperBound = computeTildeUpperBound(ver);
        if (!upperBound.empty()) upperConstraints.push_back({"<", upperBound});
      } else if (op == ">" || op == ">=") {
        lowerConstraints.push_back({op, ver});
      } else {
        upperConstraints.push_back({op, ver});
      }
    }

    std::string candidate;
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
        std::string cur =
            (lc.first == ">") ? incrementVersion(lc.second) : lc.second;
        if (candidate.empty() || compareSemVer(candidate, cur)) candidate = cur;
      }
    } else {
      candidate = "0.0.0";
    }

    bool valid = true;
    for (auto &uc : upperConstraints) {
      std::string op = uc.first;
      std::string ver = uc.second;
      // Special case: if the constraint is "<0.0.0-beta" and the candidate is
      // "0.0.0", change the candidate to "0.0.0-0".
      if (op == "<" && ver == "0.0.0-beta" && candidate == "0.0.0") {
        candidate = "0.0.0-0";
      }
      if (!satisfiesConstraint(candidate, op, ver)) {
        valid = false;
        break;
      }
    }
    if (valid && !candidate.empty()) validCandidates.push_back(candidate);
  }

  if (validCandidates.empty()) return std::nullopt;
  return *std::min_element(validCandidates.begin(), validCandidates.end(),
                           compareSemVer);
}

constexpr inline void trim_whitespace(std::string_view *input) noexcept {
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