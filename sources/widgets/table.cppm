module;

#include <algorithm>
#include <cmath>
#include <optional>
#include <string>
#include <utility>
#include <vector>

export module weqeqq.terminal:table;

import :protocol;
import :segment;
import :style;
import :measure;
import :text;
import :box;
import :cells;
import :ratio;
import :align;

export namespace weqeqq::terminal {

struct Column {
  Text header;
  Text footer;
  JustifyMethod justify = JustifyMethod::kLeft;
  VAlign vertical = VAlign::kTop;
  Style header_style;
  Style style;
  std::optional<int> width;
  int ratio = 0;
  bool no_wrap = false;
  std::optional<int> min_width;
  std::optional<int> max_width;
  bool Flexible() const { return ratio > 0; }
};

}

export namespace weqeqq::terminal {

class Table : public IRenderable {
 public:
  Box box = kSquare;
  bool show_header = true;
  bool show_edge = true;
  bool show_lines = false;
  bool show_footer = false;
  bool expand = false;
  int pad_left = 1, pad_right = 1;
  Style header_style;
  std::optional<Text> title;
  std::optional<Text> caption;
  std::vector<Style> row_styles;

  std::vector<Column> columns;
  std::vector<std::vector<Renderable>> rows;
  std::vector<bool> row_end_section;

  Table() = default;

  static Table Grid() {
    Table t;
    t.box = kSimple;
    t.show_header = false;
    t.show_edge = false;
    t.show_lines = false;
    t.pad_left = 0;
    t.pad_right = 0;
    return t;
  }

  Table& AddColumn(const std::string& header_markup,
                   JustifyMethod justify = JustifyMethod::kLeft,
                   Style style = {}, std::optional<int> width = {},
                   int ratio = 0, bool no_wrap = false) {
    Column c;
    c.header = Text::FromMarkup(header_markup);
    c.justify = justify;
    c.style = std::move(style);
    c.width = width;
    c.ratio = ratio;
    c.no_wrap = no_wrap;
    columns.push_back(std::move(c));
    return *this;
  }

  Table& AddRow(std::vector<Renderable> cells) {
    while (columns.size() < cells.size()) columns.push_back(Column{});
    rows.push_back(std::move(cells));
    row_end_section.push_back(false);
    return *this;
  }

  Table& EndSection() {
    if (!row_end_section.empty()) row_end_section.back() = true;
    return *this;
  }

  Table& AddRowText(const std::vector<std::string>& cells) {
    std::vector<Renderable> rs;
    for (const std::string& c : cells) rs.emplace_back(Text::FromMarkup(c));
    return AddRow(std::move(rs));
  }

  RenderResult Render(Console& console,
                      const ConsoleOptions& options) const override {
    int ncols = static_cast<int>(columns.size());
    if (ncols == 0) return {};
    std::vector<int> widths = CalculateWidths(console, options);

    std::vector<int> col_widths = widths;
    int table_width = ExtraWidth();
    for (int w : widths) table_width += w;

    RenderResult out;

    if (title) EmitCaption(out, console, options, *title, table_width);

    if (show_edge) PushLine(out, box.GetTop(col_widths));

    if (show_header) {
      std::vector<Renderable> header_cells;
      for (int i = 0; i < ncols; ++i) {
        Text h = columns[i].header;
        Style hs = header_style.Combine(columns[i].header_style);
        if (hs) h.Stylize(hs, 0, h.Length());
        header_cells.emplace_back(std::move(h));
      }
      RenderRow(out, console, options, header_cells, widths, true,
                std::nullopt);
      PushLine(out, box.GetRow(col_widths, Box::Level::kHead, show_edge));
    }

    for (std::size_t r = 0; r < rows.size(); ++r) {
      std::vector<Renderable> cells = rows[r];
      while (static_cast<int>(cells.size()) < ncols)
        cells.emplace_back(Text(""));
      std::optional<Style> row_style;
      if (!row_styles.empty()) row_style = row_styles[r % row_styles.size()];
      RenderRow(out, console, options, cells, widths, false, row_style);
      bool section = r < row_end_section.size() && row_end_section[r];
      if ((show_lines || section) && r + 1 < rows.size())
        PushLine(out, box.GetRow(col_widths, Box::Level::kRow, show_edge));
    }

    if (show_footer) {
      PushLine(out, box.GetRow(col_widths, Box::Level::kFoot, show_edge));
      std::vector<Renderable> footer_cells;
      for (int i = 0; i < ncols; ++i)
        footer_cells.emplace_back(columns[i].footer);
      RenderRow(out, console, options, footer_cells, widths, false,
                std::nullopt);
    }

    if (show_edge) PushLine(out, box.GetBottom(col_widths));

    if (caption) EmitCaption(out, console, options, *caption, table_width);
    return out;
  }

  Measurement Measure(Console& console,
                      const ConsoleOptions& options) const override {
    std::vector<Measurement> ranges = MeasureColumns(console, options);
    int extra = ExtraWidth();
    int min_total = extra, max_total = extra;
    for (const Measurement& m : ranges) {
      min_total += m.minimum;
      max_total += m.maximum;
    }
    return Measurement{min_total, max_total}.WithMaximum(options.max_width);
  }

 private:
  int PadWidth() const { return pad_left + pad_right; }

  void EmitCaption(RenderResult& out, Console& console,
                   const ConsoleOptions& options, const Text& label,
                   int table_width) const {
    Text t = label;
    t.justify = JustifyMethod::kCenter;
    ConsoleOptions o = options.UpdateWidth(std::max(1, table_width));
    RenderResult segs = t.Render(console, o);
    out.insert(out.end(), segs.begin(), segs.end());
  }

  int ExtraWidth() const {
    int ncols = static_cast<int>(columns.size());
    int edges = show_edge ? 2 : 0;
    int dividers = ncols > 0 ? ncols - 1 : 0;
    return edges + dividers;
  }

  std::vector<Measurement> MeasureColumns(Console& console,
                                          const ConsoleOptions& options) const {
    int ncols = static_cast<int>(columns.size());
    std::vector<Measurement> result(ncols, Measurement{0, 0});
    int avail = options.max_width;
    ConsoleOptions cell_opts = options.UpdateWidth(avail);
    for (int i = 0; i < ncols; ++i) {
      const Column& col = columns[i];
      if (col.width) {
        result[i] = Measurement{*col.width, *col.width};
        continue;
      }
      Measurement m{0, 0};
      auto consider = [&](const Renderable& r) {
        Measurement cm = r.Measure(console, cell_opts);
        m.minimum = std::max(m.minimum, cm.minimum);
        m.maximum = std::max(m.maximum, cm.maximum);
      };
      if (show_header) consider(Renderable(col.header));
      if (show_footer) consider(Renderable(col.footer));
      for (const std::vector<Renderable>& row : rows) {
        if (i < static_cast<int>(row.size())) consider(row[i]);
      }
      m.minimum += PadWidth();
      m.maximum += PadWidth();
      Measurement clamped =
          m.Clamp(col.min_width.value_or(-1), col.max_width.value_or(-1));
      result[i] = clamped;
    }
    return result;
  }

  std::vector<int> CalculateWidths(Console& console,
                                   const ConsoleOptions& options) const {
    int max_width = options.max_width;
    std::vector<Measurement> ranges = MeasureColumns(console, options);
    int ncols = static_cast<int>(ranges.size());
    std::vector<int> widths;
    for (const Measurement& m : ranges)
      widths.push_back(std::max(1, m.maximum));

    int extra = ExtraWidth();

    if (expand) {
      std::vector<int> ratios;
      bool any_flex = false;
      for (const Column& c : columns) {
        ratios.push_back(c.Flexible() ? c.ratio : 0);
        if (c.Flexible()) any_flex = true;
      }
      if (any_flex) {
        int fixed = extra;
        for (int i = 0; i < ncols; ++i)
          if (ratios[i] == 0) fixed += widths[i];
        int flex_budget = std::max(0, max_width - fixed);
        std::vector<int> dist = RatioDistribute(flex_budget, ratios);
        for (int i = 0; i < ncols; ++i)
          if (ratios[i]) widths[i] = dist[i];
      }
    }

    int table_width = extra;
    for (int w : widths) table_width += w;

    if (table_width > max_width) {
      std::vector<int> wrapable;
      for (const Column& c : columns)
        wrapable.push_back(c.no_wrap || c.width ? 0 : 1);
      widths = CollapseWidths(widths, wrapable, max_width - extra);
      table_width = extra;
      for (int w : widths) table_width += w;
    } else if (table_width < max_width && expand) {
      std::vector<int> ones(ncols, 1);
      std::vector<int> pad = RatioDistribute(max_width - table_width, ones);
      for (int i = 0; i < ncols; ++i) widths[i] += pad[i];
    }
    int floor = std::max(1, PadWidth());
    for (int& w : widths) w = std::max(floor, w);
    return widths;
  }

  static std::vector<int> CollapseWidths(std::vector<int> widths,
                                         const std::vector<int>& wrapable,
                                         int max_width) {
    int total = 0;
    for (int w : widths) total += w;
    int excess = total - max_width;
    bool any = false;
    for (int w : wrapable)
      if (w) any = true;
    while (any && excess > 0) {
      int max_column = 0;
      for (std::size_t i = 0; i < widths.size(); ++i)
        if (wrapable[i]) max_column = std::max(max_column, widths[i]);
      int second_max = 0;
      for (std::size_t i = 0; i < widths.size(); ++i)
        if (wrapable[i] && widths[i] != max_column)
          second_max = std::max(second_max, widths[i]);
      int diff = max_column - second_max;
      std::vector<int> ratios;
      for (std::size_t i = 0; i < widths.size(); ++i)
        ratios.push_back((widths[i] == max_column && wrapable[i]) ? 1 : 0);
      bool any_ratio = false;
      for (int r : ratios)
        if (r) any_ratio = true;
      if (!any_ratio || diff == 0) break;
      std::vector<int> maxred(widths.size(), std::min(excess, diff));
      widths = RatioReduce(excess, ratios, maxred, widths);
      total = 0;
      for (int w : widths) total += w;
      excess = total - max_width;
    }
    return widths;
  }

  void RenderRow(RenderResult& out, Console& console,
                 const ConsoleOptions& options,
                 const std::vector<Renderable>& cells,
                 const std::vector<int>& widths, bool header,
                 std::optional<Style> row_style) const {
    int ncols = static_cast<int>(widths.size());
    std::vector<std::vector<Line>> cell_lines(ncols);
    int row_height = 1;
    for (int i = 0; i < ncols; ++i) {
      int content_width = std::max(0, widths[i] - PadWidth());
      ConsoleOptions o = options.UpdateWidth(content_width);
      o.justify = columns[i].justify;
      cell_lines[i] = RenderLines(cells[i], console, o, true, row_style);
      row_height = std::max(row_height, static_cast<int>(cell_lines[i].size()));
    }

    for (int i = 0; i < ncols; ++i) {
      int content_width = std::max(0, widths[i] - PadWidth());
      int missing = row_height - static_cast<int>(cell_lines[i].size());
      if (missing <= 0) continue;
      int top = 0;
      if (columns[i].vertical == VAlign::kMiddle)
        top = missing / 2;
      else if (columns[i].vertical == VAlign::kBottom)
        top = missing;
      Line blank = {Segment(std::string(content_width, ' '), row_style)};
      cell_lines[i].insert(cell_lines[i].begin(), top, blank);
      while (static_cast<int>(cell_lines[i].size()) < row_height)
        cell_lines[i].push_back(blank);
    }

    const std::string& vsep = header ? box.head_vertical : box.mid_vertical;
    const std::string& edge_l = header ? box.head_left : box.mid_left;
    const std::string& edge_r = header ? box.head_right : box.mid_right;

    for (int ln = 0; ln < row_height; ++ln) {
      if (show_edge) out.emplace_back(edge_l);
      for (int i = 0; i < ncols; ++i) {
        if (pad_left) out.emplace_back(std::string(pad_left, ' '), row_style);
        Line line = cell_lines[i][ln];
        out.insert(out.end(), line.begin(), line.end());
        if (pad_right) out.emplace_back(std::string(pad_right, ' '), row_style);
        if (i + 1 < ncols) out.emplace_back(vsep);
      }
      if (show_edge) out.emplace_back(edge_r);
      out.push_back(Segment("\n"));
    }
  }

  static void PushLine(RenderResult& out, const std::string& s) {
    out.emplace_back(s);
    out.push_back(Segment("\n"));
  }
};

}
