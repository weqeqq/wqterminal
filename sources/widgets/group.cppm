module;

#include <algorithm>
#include <utility>
#include <vector>

export module weqeqq.terminal:group;

import :protocol;
import :segment;
import :measure;

export namespace weqeqq::terminal {

class Group : public IRenderable {
 public:
  std::vector<Renderable> children;

  Group() = default;
  explicit Group(std::vector<Renderable> children)
      : children(std::move(children)) {}

  Group& Add(Renderable child) {
    children.push_back(std::move(child));
    return *this;
  }

  RenderResult Render(Console& console,
                      const ConsoleOptions& options) const override {
    RenderResult out;
    for (const Renderable& child : children) {
      RenderResult segs = child.Render(console, options);
      out.insert(out.end(), segs.begin(), segs.end());
    }
    return out;
  }

  Measurement Measure(Console& console,
                      const ConsoleOptions& options) const override {
    Measurement m{0, 0};
    for (const Renderable& child : children) {
      Measurement cm = child.Measure(console, options);
      m.minimum = std::max(m.minimum, cm.minimum);
      m.maximum = std::max(m.maximum, cm.maximum);
    }
    return m.WithMaximum(options.max_width);
  }
};

}
