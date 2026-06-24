module;

#include <concepts>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

export module weqeqq.terminal:protocol;

import :segment;
import :measure;
import :style;

export namespace weqeqq::terminal {

class Console;

enum class JustifyMethod { kDefault, kLeft, kCenter, kRight, kFull };
enum class OverflowMethod { kFold, kCrop, kEllipsis, kIgnore };

struct ConsoleOptions {
  int min_width = 1;
  int max_width = 80;
  bool is_terminal = false;
  std::optional<int> height;
  std::optional<JustifyMethod> justify;
  std::optional<OverflowMethod> overflow;
  std::optional<bool> no_wrap;
  std::optional<bool> highlight;
  std::optional<bool> markup;

  ConsoleOptions UpdateWidth(int width) const {
    ConsoleOptions o = *this;
    o.min_width = o.max_width = width;
    return o;
  }

  ConsoleOptions UpdateDimensions(int width, int h) const {
    ConsoleOptions o = UpdateWidth(width);
    o.height = h;
    return o;
  }

  ConsoleOptions WithHeight(int h) const {
    ConsoleOptions o = *this;
    o.height = h;
    return o;
  }

  ConsoleOptions WithOverflow(OverflowMethod m) const {
    ConsoleOptions o = *this;
    o.overflow = m;
    return o;
  }

  ConsoleOptions WithJustify(JustifyMethod m) const {
    ConsoleOptions o = *this;
    o.justify = m;
    return o;
  }

  ConsoleOptions WithNoWrap(bool v) const {
    ConsoleOptions o = *this;
    o.no_wrap = v;
    return o;
  }
};

using RenderResult = std::vector<Segment>;

struct IRenderable {
  virtual ~IRenderable() = default;
  virtual RenderResult Render(Console& console,
                              const ConsoleOptions& options) const = 0;
  virtual Measurement Measure(Console& console,
                              const ConsoleOptions& options) const {
    (void)console;
    return Measurement{options.max_width, options.max_width};
  }
};

class Renderable {
 public:
  Renderable() = default;

  template <class T>
    requires std::derived_from<std::decay_t<T>, IRenderable> &&
             (!std::same_as<std::decay_t<T>, Renderable>)
  Renderable(T obj)
      : impl_(std::make_shared<std::decay_t<T>>(std::move(obj))) {}

  explicit Renderable(std::shared_ptr<const IRenderable> impl)
      : impl_(std::move(impl)) {}

  bool Valid() const { return static_cast<bool>(impl_); }
  const IRenderable* Get() const { return impl_.get(); }

  RenderResult Render(Console& console, const ConsoleOptions& options) const {
    return impl_->Render(console, options);
  }
  Measurement Measure(Console& console, const ConsoleOptions& options) const {
    return impl_->Measure(console, options);
  }

 private:
  std::shared_ptr<const IRenderable> impl_;
};

inline std::vector<Line> RenderLines(const Renderable& child, Console& console,
                                     const ConsoleOptions& options,
                                     bool pad = true,
                                     std::optional<Style> style = {}) {
  std::vector<Segment> segs = child.Render(console, options);
  if (style) segs = ApplyStyle(segs, *style);
  std::vector<Line> lines = SplitLines(segs);
  if (options.height) {
    if (static_cast<int>(lines.size()) > *options.height)
      lines.resize(*options.height);
    while (static_cast<int>(lines.size()) < *options.height)
      lines.push_back(Line{});
  }
  for (Line& line : lines) {
    line = AdjustLineLength(line, options.max_width, style, pad);
  }
  return lines;
}

}
