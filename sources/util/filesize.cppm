module;

#include <array>
#include <format>
#include <string>

export module weqeqq.terminal:filesize;

export namespace weqeqq::terminal {

namespace detail {
inline std::string FormatSize(long long size, double base,
                              const char* const* units, int unit_count) {
  if (size < static_cast<long long>(base)) {
    return std::to_string(size) + " " + units[0];
  }
  double value = static_cast<double>(size);
  int suffix = 0;
  while (value >= base && suffix < unit_count - 1) {
    value /= base;
    ++suffix;
  }
  return std::format("{:.1f} {}", value, units[suffix]);
}
}

inline std::string DecimalSize(long long size) {
  static const char* const units[] = {"bytes", "kB", "MB", "GB", "TB",
                                      "PB",    "EB", "ZB", "YB"};
  return detail::FormatSize(size, 1000.0, units, 9);
}

inline std::string BinarySize(long long size) {
  static const char* const units[] = {"bytes", "KiB", "MiB", "GiB", "TiB",
                                      "PiB",   "EiB", "ZiB", "YiB"};
  return detail::FormatSize(size, 1024.0, units, 9);
}

}
