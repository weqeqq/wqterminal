module;

#include <algorithm>
#include <cmath>
#include <string>

export module weqeqq.terminal:bar;

import :protocol;
import :segment;
import :style;
import :measure;

export namespace weqeqq::terminal {

class Bar : public IRenderable {
 public:
  double size = 100.0;
  double begin = 0.0;
  double end = 0.0;
  int width = -1;
  Style color = Style::Parse("bright_magenta");
  Style back = Style::Parse("color(237)");

  Bar() = default;
  Bar(double size, double begin, double end)
      : size(size), begin(begin), end(end) {}

  RenderResult Render(Console&, const ConsoleOptions& options) const override {
    int w = width > 0 ? width : options.max_width;
    if (w <= 0 || size <= 0) return {};
    auto cell = [&](double v) {
      return std::clamp(static_cast<int>(std::lround(v / size * w)), 0, w);
    };
    int b = cell(begin);
    int e = std::max(b, cell(end));

    RenderResult out;
    auto run = [&](int n, const Style& s) {
      if (n <= 0) return;
      std::string bar;
      for (int i = 0; i < n; ++i) bar += "━";
      out.emplace_back(bar, s);
    };
    run(b, back);
    run(e - b, color);
    run(w - e, back);
    return out;
  }

  Measurement Measure(Console&, const ConsoleOptions& options) const override {
    int w = width > 0 ? width : 4;
    return Measurement{4, std::max(4, w)}.WithMaximum(options.max_width);
  }
};

}
