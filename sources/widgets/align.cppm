module;

#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>

export module weqeqq.terminal:align;

import :protocol;
import :segment;
import :style;
import :measure;

export namespace weqeqq::terminal {

enum class HAlign { kLeft, kCenter, kRight };
enum class VAlign { kTop, kMiddle, kBottom };

class Align : public IRenderable {
 public:
  Renderable child;
  HAlign align = HAlign::kLeft;
  std::optional<VAlign> vertical;
  Style style;

  Align(Renderable child, HAlign align = HAlign::kLeft,
        std::optional<VAlign> vertical = {}, Style style = {})
      : child(std::move(child)),
        align(align),
        vertical(vertical),
        style(std::move(style)) {}

  static Align Center(Renderable child) {
    return Align(std::move(child), HAlign::kCenter);
  }
  static Align Right(Renderable child) {
    return Align(std::move(child), HAlign::kRight);
  }

  RenderResult Render(Console& console,
                      const ConsoleOptions& options) const override {
    int max_width = options.max_width;
    int child_width =
        std::min(child.Measure(console, options).maximum, max_width);
    child_width = std::max(child_width, 0);

    ConsoleOptions child_opts = options.UpdateWidth(child_width);
    std::optional<Style> st =
        style ? std::optional<Style>(style) : std::nullopt;
    std::vector<Line> lines = RenderLines(child, console, child_opts, true, st);

    int excess = max_width - child_width;
    int left = 0;
    if (align == HAlign::kCenter)
      left = excess / 2;
    else if (align == HAlign::kRight)
      left = excess;
    int right = excess - left;

    int top_blank = 0, bottom_blank = 0;
    if (options.height && vertical) {
      int extra = *options.height - static_cast<int>(lines.size());
      if (extra > 0) {
        if (*vertical == VAlign::kMiddle)
          top_blank = extra / 2;
        else if (*vertical == VAlign::kBottom)
          top_blank = extra;
        bottom_blank = extra - top_blank;
      }
    }

    RenderResult out;
    auto blank_row = [&]() {
      out.emplace_back(std::string(max_width, ' '), st);
      out.push_back(Segment("\n"));
    };
    for (int i = 0; i < top_blank; ++i) blank_row();
    for (Line& line : lines) {
      if (left) out.emplace_back(std::string(left, ' '), st);
      out.insert(out.end(), line.begin(), line.end());
      if (right) out.emplace_back(std::string(right, ' '), st);
      out.push_back(Segment("\n"));
    }
    for (int i = 0; i < bottom_blank; ++i) blank_row();
    return out;
  }

  Measurement Measure(Console& console,
                      const ConsoleOptions& options) const override {
    return child.Measure(console, options);
  }
};

}
