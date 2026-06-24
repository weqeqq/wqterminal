module;

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

export module weqeqq.terminal:tree;

import :protocol;
import :segment;
import :style;
import :measure;
import :cells;

export namespace weqeqq::terminal {

class Tree : public IRenderable {
 public:
  Renderable label;
  std::vector<Tree> children;
  Style style;
  Style guide_style;

  explicit Tree(Renderable label, Style style = {}, Style guide_style = {})
      : label(std::move(label)),
        style(std::move(style)),
        guide_style(std::move(guide_style)) {}

  Tree& Add(Renderable child_label) {
    children.emplace_back(std::move(child_label));
    return children.back();
  }

  RenderResult Render(Console& console,
                      const ConsoleOptions& options) const override {
    RenderResult out;
    walk(*this, "", "", console, options, out);
    return out;
  }

  Measurement Measure(Console& console,
                      const ConsoleOptions& options) const override {
    Measurement m = label.Measure(console, options);
    int max_child = 0;
    for (const Tree& c : children) {
      Measurement cm = c.Measure(console, options);
      max_child = std::max(max_child, cm.maximum + 4);
    }
    return Measurement{m.minimum, std::max(m.maximum, max_child)}.WithMaximum(
        options.max_width);
  }

 private:
  void walk(const Tree& node, const std::string& line_prefix,
            const std::string& subtree_prefix, Console& console,
            const ConsoleOptions& options, RenderResult& out) const {
    std::optional<Style> gstyle =
        guide_style ? std::optional<Style>(guide_style) : std::nullopt;

    int prefix_cells = CellLen(line_prefix);
    ConsoleOptions label_opts =
        options.UpdateWidth(std::max(1, options.max_width - prefix_cells));
    std::vector<Line> label_lines =
        RenderLines(node.label, console, label_opts, false);

    for (std::size_t i = 0; i < label_lines.size(); ++i) {
      const std::string& guide = (i == 0) ? line_prefix : subtree_prefix;
      if (!guide.empty()) out.emplace_back(guide, gstyle);
      out.insert(out.end(), label_lines[i].begin(), label_lines[i].end());
      out.push_back(Segment("\n"));
    }

    for (std::size_t i = 0; i < node.children.size(); ++i) {
      bool last = i + 1 == node.children.size();
      std::string branch = subtree_prefix + (last ? "└── " : "├── ");
      std::string cont = subtree_prefix + (last ? "    " : "│   ");
      walk(node.children[i], branch, cont, console, options, out);
    }
  }
};

}
