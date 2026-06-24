module;

#include <map>
#include <string>
#include <utility>

export module weqeqq.terminal:theme;

import :style;

export namespace weqeqq::terminal {

class Theme {
 public:
  std::map<std::string, Style> styles;

  Theme() = default;

  bool Has(const std::string& name) const { return styles.count(name) != 0; }

  Style Get(const std::string& name) const {
    auto it = styles.find(name);
    return it != styles.end() ? it->second : Style();
  }

  void Set(const std::string& name, Style style) {
    styles[name] = std::move(style);
  }
};

inline Theme DefaultTheme() {
  Theme t;
  auto s = [&](const char* name, const char* def) {
    t.styles[name] = Style::Parse(def);
  };
  s("none", "");
  s("dim", "dim");
  s("bold", "bold");
  s("italic", "italic");
  s("underline", "underline");
  s("repr.number", "bold cyan");
  s("repr.str", "green");
  s("repr.bool_true", "bold green");
  s("repr.bool_false", "bold red");
  s("repr.none", "italic magenta");
  s("repr.url", "underline blue");
  s("repr.path", "magenta");
  s("repr.brace", "bold");
  s("repr.ipv4", "bold bright_green");
  s("repr.uuid", "bright_yellow");
  s("table.header", "bold");
  s("table.footer", "bold");
  s("table.title", "italic");
  s("table.caption", "dim italic");
  s("progress.description", "");
  s("progress.percentage", "magenta");
  s("progress.remaining", "cyan");
  s("progress.elapsed", "yellow");
  s("progress.bar", "bold magenta");
  s("bar.complete", "magenta");
  s("bar.finished", "green");
  s("bar.back", "color(237)");
  s("tree.line", "");
  s("rule.line", "bright_green");
  s("rule.text", "");
  s("status.spinner", "green");
  s("prompt", "bold");
  s("prompt.choices", "magenta");
  s("prompt.default", "cyan");
  return t;
}

}
