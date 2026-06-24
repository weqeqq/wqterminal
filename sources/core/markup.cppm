module;

#include <cctype>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

export module weqeqq.terminal:markup;

import :text;
import :style;
import :emoji;

export namespace weqeqq::terminal {

Text RenderMarkup(std::string_view markup, Style base_style = {},
                  std::function<Style(std::string_view)> resolver = {});

}

namespace weqeqq::terminal::detail {

struct OpenTag {
  int position;
  Style style;
  std::string name;
};

inline std::string trim(std::string_view s) {
  std::size_t b = 0, e = s.size();
  while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return std::string(s.substr(b, e - b));
}

inline bool ValidTagStart(char c) {
  return (c >= 'a' && c <= 'z') || c == '#' || c == '/' || c == '@';
}

}

namespace weqeqq::terminal {

inline Text RenderMarkup(std::string_view markup, Style base_style,
                         std::function<Style(std::string_view)> resolver) {
  auto resolve = [&](std::string_view def) -> Style {
    return resolver ? resolver(def) : Style::Parse(def);
  };
  Text text("", base_style);
  std::vector<detail::OpenTag> stack;
  std::string plain;

  auto flush = [&]() {
    if (!plain.empty()) {
      text.Append(ReplaceEmoji(plain));
      plain.clear();
    }
  };

  std::size_t i = 0;
  while (i < markup.size()) {
    char c = markup[i];
    if (c == '\\' && i + 1 < markup.size() && markup[i + 1] == '[') {
      plain.push_back('[');
      i += 2;
      continue;
    }
    if (c == '[') {
      std::size_t close = markup.find(']', i + 1);
      if (close == std::string_view::npos) {
        plain.push_back(c);
        ++i;
        continue;
      }
      std::string content(markup.substr(i + 1, close - i - 1));
      if (content.empty() || !detail::ValidTagStart(content[0])) {
        plain.push_back(c);
        ++i;
        continue;
      }
      flush();
      if (content[0] == '/') {
        std::string name = detail::trim(std::string_view(content).substr(1));
        if (name.empty()) {
          if (!stack.empty()) {
            detail::OpenTag top = stack.back();
            stack.pop_back();
            text.Stylize(top.style, top.position, text.Length());
          }
        } else {
          for (int idx = static_cast<int>(stack.size()) - 1; idx >= 0; --idx) {
            if (stack[idx].name == name) {
              detail::OpenTag open = stack[idx];
              stack.erase(stack.begin() + idx);
              text.Stylize(open.style, open.position, text.Length());
              break;
            }
          }
        }
      } else if (content[0] == '@') {
        std::size_t eq = content.find('=');
        std::string key = content.substr(0, eq);
        std::string value =
            eq != std::string::npos ? content.substr(eq + 1) : std::string();
        if (value.size() >= 2 &&
            (value.front() == '\'' || value.front() == '"') &&
            value.back() == value.front()) {
          value = value.substr(1, value.size() - 2);
        }
        Style st;
        st.meta[key] = value;
        stack.push_back(detail::OpenTag{text.Length(), st, content});
      } else {
        std::size_t eq = content.find('=');
        std::string style_def = content;
        if (eq != std::string::npos) {
          style_def = content.substr(0, eq) + " " + content.substr(eq + 1);
        }
        Style st = resolve(style_def);
        stack.push_back(detail::OpenTag{text.Length(), st, content});
      }
      i = close + 1;
      continue;
    }
    plain.push_back(c);
    ++i;
  }
  flush();
  for (const detail::OpenTag& open : stack) {
    text.Stylize(open.style, open.position, text.Length());
  }
  return text;
}

Text Text::FromMarkup(std::string_view markup, Style style,
                      std::function<Style(std::string_view)> resolver) {
  return RenderMarkup(markup, style, std::move(resolver));
}

}
