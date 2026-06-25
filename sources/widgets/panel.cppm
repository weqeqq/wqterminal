module;

#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>

export module weqeqq.terminal:panel;

import :protocol;
import :segment;
import :style;
import :measure;
import :text;
import :box;
import :cells;
import :align;

export namespace weqeqq::terminal {

class Panel : public IRenderable {
 public:
  Renderable child;
  Box box = kRounded;
  std::optional<Text> title;
  std::optional<Text> subtitle;
  Style style;
  Style border_style;
  bool expand = true;
  HAlign title_align = HAlign::kCenter;
  HAlign subtitle_align = HAlign::kCenter;
  int pad_top = 0, pad_right = 1, pad_bottom = 0, pad_left = 1;

  explicit Panel(Renderable child, Box box = kRounded)
      : child(std::move(child)), box(std::move(box)) {}

  Panel& WithTitle(Text t) {
    title = std::move(t);
    return *this;
  }
  Panel& WithTitle(const std::string& markup) {
    title = Text::FromMarkup(markup);
    return *this;
  }
  Panel& WithBorderStyle(Style s) {
    border_style = std::move(s);
    return *this;
  }

  RenderResult Render(Console& console,
                      const ConsoleOptions& options) const override {
    Style bstyle = style.Combine(border_style);
    std::optional<Style> bs =
        bstyle ? std::optional<Style>(bstyle) : std::nullopt;

    int pad_h = pad_left + pad_right;
    int width;
    if (expand) {
      width = options.max_width;
    } else {
      int child_max = child.Measure(console, options).maximum;
      width = std::min(child_max + pad_h + 2, options.max_width);
    }
    width = std::max(width, 2 + pad_h);
    int inner = width - 2;
    int child_width = inner - pad_h;

    ConsoleOptions child_opts = options.UpdateWidth(std::max(0, child_width));
    if (options.height)
      child_opts.height =
          std::max(0, *options.height - 2 - pad_top - pad_bottom);
    std::vector<Line> lines = RenderLines(child, console, child_opts, true);

    RenderResult out;
    AppendEdge(out, box.top_left, box.top, box.top_right, inner, title, bs,
               title_align);

    auto blank_content = [&]() {
      out.emplace_back(box.mid_left, bs);
      out.emplace_back(std::string(inner, ' '));
      out.emplace_back(box.mid_right, bs);
      out.push_back(Segment("\n"));
    };
    for (int i = 0; i < pad_top; ++i) blank_content();
    for (Line& line : lines) {
      out.emplace_back(box.mid_left, bs);
      if (pad_left) out.emplace_back(std::string(pad_left, ' '));
      out.insert(out.end(), line.begin(), line.end());
      if (pad_right) out.emplace_back(std::string(pad_right, ' '));
      out.emplace_back(box.mid_right, bs);
      out.push_back(Segment("\n"));
    }
    for (int i = 0; i < pad_bottom; ++i) blank_content();

    AppendEdge(out, box.bottom_left, box.bottom, box.bottom_right, inner,
               subtitle, bs, subtitle_align);
    return out;
  }

  Measurement Measure(Console& console,
                      const ConsoleOptions& options) const override {
    int extra = 2 + pad_left + pad_right;
    ConsoleOptions o =
        options.UpdateWidth(std::max(0, options.max_width - extra));
    Measurement m = child.Measure(console, o);
    return Measurement{m.minimum + extra, m.maximum + extra}.WithMaximum(
        options.max_width);
  }

 private:
  static void AppendEdge(RenderResult& out, const std::string& corner_left,
                         const std::string& fill,
                         const std::string& corner_right, int inner,
                         const std::optional<Text>& label,
                         const std::optional<Style>& bs, HAlign align) {
    auto fill_n = [&](int n) {
      std::string s;
      for (int i = 0; i < n; ++i) s += fill;
      return s;
    };
    out.emplace_back(corner_left, bs);
    if (label && !label->IsEmpty()) {
      Text t = *label;
      t.Truncate(std::max(0, inner - 4), OverflowMethod::kEllipsis, false);
      int tc = t.CellLength();
      int side = inner - tc - 2;
      int left = std::max(0, side / 2);
      if (align == HAlign::kLeft)
        left = 0;
      else if (align == HAlign::kRight)
        left = std::max(0, side);
      int right = std::max(0, side - left);
      out.emplace_back(fill_n(left) + " ", bs);
      auto segs = t.RenderLine();
      out.insert(out.end(), segs.begin(), segs.end());
      out.emplace_back(" " + fill_n(right), bs);
    } else {
      out.emplace_back(fill_n(inner), bs);
    }
    out.emplace_back(corner_right, bs);
    out.push_back(Segment("\n"));
  }
};

}
