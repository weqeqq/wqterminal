module;

#include <algorithm>
#include <array>
#include <string>
#include <utility>
#include <vector>
#include <optional>

export module weqeqq.terminal:padding;

import :protocol;
import :segment;
import :style;
import :measure;

export namespace weqeqq::terminal {

class Padding : public IRenderable {
 public:
  Renderable child;
  int top = 0, right = 0, bottom = 0, left = 0;
  Style style;
  bool expand = true;

  Padding(Renderable child, int top, int right, int bottom, int left,
          Style style = {}, bool expand = true)
      : child(std::move(child)),
        top(top),
        right(right),
        bottom(bottom),
        left(left),
        style(std::move(style)),
        expand(expand) {}

  static Padding All(Renderable child, int pad, Style style = {}) {
    return Padding(std::move(child), pad, pad, pad, pad, std::move(style));
  }
  static Padding Symmetric(Renderable child, int vertical, int horizontal,
                           Style style = {}) {
    return Padding(std::move(child), vertical, horizontal, vertical, horizontal,
                   std::move(style));
  }

  RenderResult Render(Console& console,
                      const ConsoleOptions& options) const override {
    int width = options.max_width;
    if (!expand) {
      int child_max = child.Measure(console, options).maximum;
      width = std::min(child_max + left + right, options.max_width);
    }
    ConsoleOptions child_opts = options.UpdateWidth(width - left - right);
    if (options.height) child_opts.height = *options.height - top - bottom;

    std::optional<Style> st =
        style ? std::optional<Style>(style) : std::nullopt;
    std::vector<Line> lines = RenderLines(child, console, child_opts, true, st);

    RenderResult out;
    Line blank = {Segment(std::string(width, ' '), st), Segment("\n")};
    for (int i = 0; i < top; ++i)
      out.insert(out.end(), blank.begin(), blank.end());
    for (Line& line : lines) {
      if (left) out.emplace_back(std::string(left, ' '), st);
      out.insert(out.end(), line.begin(), line.end());
      if (right) out.emplace_back(std::string(right, ' '), st);
      out.push_back(Segment("\n"));
    }
    for (int i = 0; i < bottom; ++i)
      out.insert(out.end(), blank.begin(), blank.end());
    return out;
  }

  Measurement Measure(Console& console,
                      const ConsoleOptions& options) const override {
    int extra = left + right;
    ConsoleOptions o = options.UpdateWidth(options.max_width - extra);
    Measurement m = child.Measure(console, o);
    return Measurement{m.minimum + extra, m.maximum + extra}.WithMaximum(
        options.max_width);
  }
};

}
