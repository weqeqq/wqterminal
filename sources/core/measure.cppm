module;

#include <algorithm>

export module weqeqq.terminal:measure;

export namespace weqeqq::terminal {

struct Measurement {
  int minimum = 0;
  int maximum = 0;

  constexpr int Span() const { return maximum - minimum; }

  constexpr Measurement Normalize() const {
    int lo = std::min(minimum, maximum);
    int hi = std::max(minimum, maximum);
    return Measurement{std::max(0, lo), std::max(0, hi)};
  }

  constexpr Measurement WithMaximum(int width) const {
    return Measurement{std::min(minimum, width), std::min(maximum, width)};
  }

  constexpr Measurement WithMinimum(int width) const {
    width = std::max(0, width);
    return Measurement{std::max(minimum, width), std::max(maximum, width)};
  }

  constexpr Measurement Clamp(int min_width = -1, int max_width = -1) const {
    Measurement m = *this;
    if (min_width >= 0) {
      m = m.WithMinimum(min_width);
    }
    if (max_width >= 0) {
      m = m.WithMaximum(max_width);
    }
    return m;
  }

  constexpr bool operator==(const Measurement&) const = default;
};

}
