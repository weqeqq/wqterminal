module;

#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>

export module weqeqq.terminal:columns;

import :protocol;
import :segment;
import :measure;

export namespace weqeqq::terminal {

class Columns : public IRenderable {
 public:
  std::vector<Renderable> children;
  int padding = 1;
  bool equal = false;
  bool column_first = false;
  bool right_to_left = false;
  bool expand = false;
  std::optional<int> column_count;

  Columns() = default;
  explicit Columns(std::vector<Renderable> children, int padding = 1)
      : children(std::move(children)), padding(padding) {}

  Columns& Add(Renderable child) {
    children.push_back(std::move(child));
    return *this;
  }

  RenderResult Render(Console& console,
                      const ConsoleOptions& options) const override {
    int n = static_cast<int>(children.size());
    if (n == 0) return {};

    std::vector<int> item_max(n);
    int widest = 1;
    for (int i = 0; i < n; ++i) {
      item_max[i] = std::max(1, children[i].Measure(console, options).maximum);
      widest = std::max(widest, item_max[i]);
    }

    int avail = options.max_width;
    int cols;
    if (column_count) {
      cols = std::max(1, *column_count);
    } else {
      cols = std::max(1, (avail + padding) / (widest + padding));
    }
    cols = std::min(cols, n);

    int rows = (n + cols - 1) / cols;
    if (column_first) cols = (n + rows - 1) / rows;

    auto index_at = [&](int r, int lc) {
      return column_first ? lc * rows + r : r * cols + lc;
    };
    auto col_of = [&](int i) { return column_first ? i / rows : i % cols; };

    std::vector<int> col_width(cols, 1);
    for (int i = 0; i < n; ++i)
      col_width[col_of(i)] = std::max(col_width[col_of(i)], item_max[i]);
    if (equal) {
      int w = *std::max_element(col_width.begin(), col_width.end());
      std::fill(col_width.begin(), col_width.end(), w);
    }
    int total = padding * (cols - 1);
    for (int w : col_width) total += w;
    if (total > avail && cols > 0) {
      int over = total - avail;
      for (int c = 0; c < cols && over > 0; ++c) {
        int reduce = std::min(over, std::max(0, col_width[c] - 1));
        col_width[c] -= reduce;
        over -= reduce;
      }
    } else if (expand && total < avail && cols > 0) {
      int extra = avail - total;
      for (int c = 0; c < cols; ++c) {
        int add = extra / cols + (c < extra % cols ? 1 : 0);
        col_width[c] += add;
      }
    }

    RenderResult out;
    for (int r = 0; r < rows; ++r) {
      int row_height = 1;
      std::vector<std::vector<Line>> cell_lines(cols);
      for (int vc = 0; vc < cols; ++vc) {
        int lc = right_to_left ? cols - 1 - vc : vc;
        int idx = index_at(r, lc);
        if (idx < 0 || idx >= n) continue;
        ConsoleOptions o = options.UpdateWidth(col_width[lc]);
        cell_lines[vc] = RenderLines(children[idx], console, o, true);
        row_height =
            std::max(row_height, static_cast<int>(cell_lines[vc].size()));
      }
      for (int ln = 0; ln < row_height; ++ln) {
        for (int vc = 0; vc < cols; ++vc) {
          int lc = right_to_left ? cols - 1 - vc : vc;
          int idx = index_at(r, lc);
          if (idx < 0 || idx >= n) {
            out.emplace_back(std::string(col_width[lc], ' '));
          } else if (ln < static_cast<int>(cell_lines[vc].size())) {
            Line& line = cell_lines[vc][ln];
            out.insert(out.end(), line.begin(), line.end());
          } else {
            out.emplace_back(std::string(col_width[lc], ' '));
          }
          if (vc + 1 < cols) out.emplace_back(std::string(padding, ' '));
        }
        out.push_back(Segment("\n"));
      }
    }
    return out;
  }

  Measurement Measure(Console& console,
                      const ConsoleOptions& options) const override {
    int widest = 1;
    for (const Renderable& c : children)
      widest = std::max(widest, c.Measure(console, options).maximum);
    return Measurement{widest, options.max_width};
  }
};

}
