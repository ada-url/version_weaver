#include "version_weaver.h"
namespace version_weaver {
  bool validate(std::string_view version) { return true; }
  bool gt(std::string_view version1, std::string_view version2)  { return true; }
  bool lt(std::string_view version1, std::string_view version2) { return true; }
  bool satisfies(std::string_view version, std::string_view range) { return true; }
  std::string coerce(std::string_view version)  { return ""; }
  std::string minimum(std::string_view range)  { return ""; }
  std::string clean(std::string_view range)  { return ""; }
}