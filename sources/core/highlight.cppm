module;

#include <string>
#include <utility>
#include <vector>

export module weqeqq.terminal:highlight;

import :text;
import :style;
import :theme;

export namespace weqeqq::terminal {

class RegexHighlighter {
 public:
  std::vector<std::pair<std::string, std::string>> rules;

  void Highlight(Text& text, const Theme& theme) const {
    for (const auto& [pattern, name] : rules) {
      Style style = theme.Get(name);
      if (style) text.HighlightRegex(pattern, style);
    }
  }
};

inline RegexHighlighter ReprHighlighter() {
  RegexHighlighter h;
  h.rules = {
      {R"([+-]?\b\d+(\.\d+)?\b)", "repr.number"},
      {R"('[^']*')", "repr.str"},
      {R"("[^"]*")", "repr.str"},
      {R"(\b(True|true)\b)", "repr.bool_true"},
      {R"(\b(False|false)\b)", "repr.bool_false"},
      {R"(\b(None|null|nullptr)\b)", "repr.none"},
      {R"(\b\d{1,3}(\.\d{1,3}){3}\b)", "repr.ipv4"},
      {R"(\b[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}\b)",
       "repr.uuid"},
      {R"(https?://[^\s]+)", "repr.url"},
      {R"([\[\](){}])", "repr.brace"},
  };
  return h;
}

}
