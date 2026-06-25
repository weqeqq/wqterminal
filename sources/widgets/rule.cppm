module;

#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>

export module weqeqq.terminal:rule;

import :protocol;
import :segment;
import :style;
import :text;
import :measure;
import :cells;
import :align;

export namespace weqeqq::terminal {

class Rule : public IRenderable {
 public:
  Text title;
  std::string characters = "─";
  Style style;
  HAlign align = HAlign::kCenter;

  Rule() = default;
  explicit Rule(Text title, Style style = {}, HAlign align = HAlign::kCenter)
      : title(std::move(title)), style(std::move(style)), align(align) {}
  explicit Rule(std::string title_markup)
      : title(Text::FromMarkup(title_markup)) {}

  RenderResult Render(Console&, const ConsoleOptions& options) const override {
    int width = options.max_width;
    int chars_len = std::max(1, CellLen(characters));
    std::optional<Style> st =
        style ? std::optional<Style>(style) : std::nullopt;

    auto repeat = [&](int target_cells) {
      std::string s;
      while (CellLen(s) < target_cells) s += characters;
      return SetCellSize(s, std::max(0, target_cells));
    };

    RenderResult out;
    if (title.IsEmpty()) {
      out.emplace_back(repeat(width), st);
      out.push_back(Segment("\n"));
      return out;
    }

    Text label = title;
    label.Truncate(std::max(0, width - 4), OverflowMethod::kEllipsis, false);
    int title_cells = label.CellLength();
    if (align == HAlign::kCenter) {
      int side = (width - title_cells - 2) / 2;
      int left = std::max(0, side);
      int right = std::max(0, width - title_cells - 2 - left);
      out.emplace_back(repeat(left) + " ", st);
      auto segs = label.RenderLine();
      out.insert(out.end(), segs.begin(), segs.end());
      out.emplace_back(" " + repeat(right), st);
    } else if (align == HAlign::kLeft) {
      auto segs = label.RenderLine();
      out.insert(out.end(), segs.begin(), segs.end());
      out.emplace_back(" " + repeat(std::max(0, width - title_cells - 1)), st);
    } else {
      out.emplace_back(repeat(std::max(0, width - title_cells - 1)) + " ", st);
      auto segs = label.RenderLine();
      out.insert(out.end(), segs.begin(), segs.end());
    }
    out.push_back(Segment("\n"));
    (void)chars_len;
    return out;
  }

  Measurement Measure(Console&, const ConsoleOptions& options) const override {
    return Measurement{1, options.max_width};
  }
};

}
