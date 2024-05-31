#include "version_weaver.h"
namespace version_weaver {
  bool validate(std::string_view version) {
    return std::holds_alternative<Version>(parse(version));
  }

  bool gt(std::string_view version1, std::string_view version2)  { return true; }
  bool lt(std::string_view version1, std::string_view version2) { return true; }
  bool satisfies(std::string_view version, std::string_view range) { return true; }
  std::string coerce(std::string_view version)  { return ""; }
  std::string minimum(std::string_view range)  { return ""; }
  std::string clean(std::string_view range)  { return ""; }

  std::variant<Version, ParseError> parse(std::string_view input) {
    if (input.size() > MAX_VERSION_LENGTH) {
      return ParseError::VERSION_LARGER_THAN_MAX_LENGTH;
    }

    std::string_view input_copy = input;
    // TODO: Trim leading and trailing whitespace

    auto dot_iterator = input_copy.find('.');
    if (dot_iterator == std::string_view::npos) {
      // Only major exists. No minor or patch.
      return ParseError::INVALID_INPUT;
    }
    Version version;
    auto major = input_copy.substr(0, dot_iterator);

    if (major.empty() || major.front() == '0') {
      // Version components can not have leading zeroes.
      return ParseError::INVALID_INPUT;
    }
    version.major = major;
    input_copy = input_copy.substr(dot_iterator + 1);
    dot_iterator = input_copy.find('.');
    if (dot_iterator == std::string_view::npos) {
      // Only major and minor exists. No patch.
      return ParseError::INVALID_INPUT;
    }

    auto minor = input_copy.substr(0, dot_iterator);
    if (minor.empty() || minor.front() == '0') {
      // Version components can not have leading zeroes.
      return ParseError::INVALID_INPUT;
    }
    version.minor = minor;
    input_copy = input_copy.substr(dot_iterator + 1);
    dot_iterator = input_copy.find('.');
    if (dot_iterator == std::string_view::npos) {
      // Only major, minor and patch exists.
      return ParseError::INVALID_INPUT;
    }

    auto patch = input_copy.substr(0, dot_iterator);
    if (patch.empty() || patch.front() == '0') {
      // Version components can not have leading zeroes.
      return ParseError::INVALID_INPUT;
    }
    version.patch = patch;
    input_copy = input_copy.substr(dot_iterator + 1);

    auto pre_release_iterator = input_copy.find('-');
    if (pre_release_iterator != std::string_view::npos) {
      version.pre_release = input_copy.substr(0, pre_release_iterator);
      input_copy = input_copy.substr(pre_release_iterator + 1);
      if (input_copy.find('.') != std::string_view::npos) {
        // Build metadata exists.
        auto build_iterator = input_copy.find('.');
        version.build = input_copy.substr(0, build_iterator);
      }
    }

    return version;
  }
}