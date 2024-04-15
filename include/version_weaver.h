#ifndef VERSION_WEAVER_H
#define VERSION_WEAVER_H
#include <string_view>
#include <string>

namespace version_weaver {
  bool validate(std::string_view version);
  bool gt(std::string_view version1, std::string_view version2);
  bool lt(std::string_view version1, std::string_view version2);
  bool satisfies(std::string_view version, std::string_view range);
  std::string coerce(std::string_view version);
  std::string minimum(std::string_view range);
  std::string clean(std::string_view range);
}

#endif // VERSION_WEAVER_H
