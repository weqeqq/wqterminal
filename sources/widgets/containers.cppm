module;

#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>

export module weqeqq.terminal:containers;

import :protocol;
import :segment;
import :style;
import :measure;

export namespace weqeqq::terminal {

class Constrain : public IRenderable {
 public:
  Renderable child;
  int max = 80;

  Constrain(Renderable child, int max) : child(std::move(child)), max(max) {}

  RenderResult Render(Console& console,
                      const ConsoleOptions& options) const override {
    ConsoleOptions o = options;
    o.max_width = std::min(options.max_width, max);
    return child.Render(console, o);
  }
  Measurement Measure(Console& console,
                      const ConsoleOptions& options) const override {
    ConsoleOptions o = options;
    o.max_width = std::min(options.max_width, max);
    return child.Measure(console, o);
  }
};

class Styled : public IRenderable {
 public:
  Renderable child;
  Style style;

  Styled(Renderable child, Style style)
      : child(std::move(child)), style(std::move(style)) {}

  RenderResult Render(Console& console,
                      const ConsoleOptions& options) const override {
    return ApplyStyle(child.Render(console, options), style);
  }
  Measurement Measure(Console& console,
                      const ConsoleOptions& options) const override {
    return child.Measure(console, options);
  }
};

class Screen : public IRenderable {
 public:
  Renderable child;
  Style style;

  explicit Screen(Renderable child, Style style = {})
      : child(std::move(child)), style(std::move(style)) {}

  RenderResult Render(Console& console,
                      const ConsoleOptions& options) const override {
    std::optional<Style> st =
        style ? std::optional<Style>(style) : std::nullopt;
    std::vector<Line> lines = RenderLines(child, console, options, true, st);
    RenderResult out;
    for (Line& line : lines) {
      out.insert(out.end(), line.begin(), line.end());
      out.push_back(Segment("\n"));
    }
    return out;
  }
  Measurement Measure(Console&, const ConsoleOptions& options) const override {
    return Measurement{options.max_width, options.max_width};
  }
};

}
