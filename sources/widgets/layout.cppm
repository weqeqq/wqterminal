module;

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

export module weqeqq.terminal:layout;

import :protocol;
import :segment;
import :measure;
import :ratio;
import :console;

export namespace weqeqq::terminal {

class Layout : public IRenderable {
 public:
  std::string name;
  int size = -1;
  int ratio = 1;
  int minimum_size = 1;
  bool visible = true;

  Layout() = default;
  explicit Layout(Renderable r, std::string name = "")
      : name(std::move(name)), renderable_(std::move(r)) {}
  explicit Layout(std::string name) : name(std::move(name)) {}

  Layout& Update(Renderable r) {
    renderable_ = std::move(r);
    children_.clear();
    direction_ = Direction::kNone;
    return *this;
  }

  Layout& SplitRow(std::vector<Layout> columns) {
    direction_ = Direction::kRow;
    children_ = std::move(columns);
    renderable_.reset();
    return *this;
  }

  Layout& SplitColumn(std::vector<Layout> rows) {
    direction_ = Direction::kColumn;
    children_ = std::move(rows);
    renderable_.reset();
    return *this;
  }

  std::vector<Layout>& children() { return children_; }

  Layout* Find(const std::string& n) {
    if (name == n) return this;
    for (Layout& c : children_)
      if (Layout* f = c.Find(n)) return f;
    return nullptr;
  }

  Layout& operator[](const std::string& n) {
    if (Layout* f = Find(n)) return *f;
    throw std::out_of_range("Layout: no region named '" + n + "'");
  }

  RenderResult Render(Console& console,
                      const ConsoleOptions& options) const override {
    int w = options.max_width;
    int h = options.height.value_or(console.height);
    if (h <= 0) h = 1;
    std::vector<Line> lines = RenderRegion(console, w, h);
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

 private:
  enum class Direction { kNone, kRow, kColumn };
  Direction direction_ = Direction::kNone;
  std::optional<Renderable> renderable_;
  std::vector<Layout> children_;

  std::vector<Line> RenderRegion(Console& console, int w, int h) const {
    std::vector<const Layout*> kids;
    for (const Layout& c : children_)
      if (c.visible) kids.push_back(&c);

    std::vector<Line> result;
    if (direction_ == Direction::kNone || kids.empty()) {
      if (renderable_) {
        ConsoleOptions o;
        o.min_width = 1;
        o.max_width = std::max(1, w);
        o.height = std::max(0, h);
        result = RenderLines(*renderable_, console, o, true);
      }
    } else {
      std::vector<RatioEdge> edges;
      for (const Layout* c : kids)
        edges.push_back(RatioEdge{c->size, c->ratio, c->minimum_size});

      if (direction_ == Direction::kRow) {
        std::vector<int> widths = RatioResolve(w, edges);
        std::vector<std::vector<Line>> blocks(kids.size());
        for (std::size_t i = 0; i < kids.size(); ++i)
          blocks[i] = kids[i]->RenderRegion(console, widths[i], h);
        result.assign(h, Line{});
        for (int r = 0; r < h; ++r)
          for (std::size_t i = 0; i < kids.size(); ++i)
            result[r].insert(result[r].end(), blocks[i][r].begin(),
                             blocks[i][r].end());
      } else {
        std::vector<int> heights = RatioResolve(h, edges);
        for (std::size_t i = 0; i < kids.size(); ++i) {
          std::vector<Line> blk = kids[i]->RenderRegion(console, w, heights[i]);
          for (Line& l : blk) result.push_back(std::move(l));
        }
      }
    }
    return Normalize(std::move(result), w, h);
  }

  static std::vector<Line> Normalize(std::vector<Line> lines, int w, int h) {
    for (Line& l : lines) l = AdjustLineLength(l, w);
    if (static_cast<int>(lines.size()) > h) lines.resize(h);
    while (static_cast<int>(lines.size()) < h)
      lines.push_back(Line{Segment(std::string(std::max(0, w), ' '))});
    return lines;
  }
};

}
