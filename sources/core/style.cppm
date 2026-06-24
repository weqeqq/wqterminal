module;

#include <array>
#include <cctype>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

export module weqeqq.terminal:style;

import :color;

export namespace weqeqq::terminal {

enum Attribute : std::uint32_t {
  kBold = 0,
  kDim = 1,
  kItalic = 2,
  kUnderline = 3,
  kBlink = 4,
  kBlink2 = 5,
  kReverse = 6,
  kConceal = 7,
  kStrike = 8,
  kUnderline2 = 9,
  kFrame = 10,
  kEncircle = 11,
  kOverline = 12,
  kAttributeCount = 13,
};

}

namespace weqeqq::terminal::detail {

inline constexpr std::array<int, kAttributeCount> kAttributeSgr = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 21, 51, 52, 53};

inline std::uint64_t NextLinkId() {
  static std::uint64_t counter = 0;
  return ++counter;
}

}

export namespace weqeqq::terminal {

class Style {
 public:
  std::optional<Color> color;
  std::optional<Color> bgcolor;
  std::uint32_t set_attributes = 0;
  std::uint32_t attributes = 0;
  std::string link;
  std::map<std::string, std::string> meta;

  Style() = default;

  Style(std::optional<Color> fg, std::optional<Color> bg,
        std::vector<std::pair<Attribute, bool>> attrs = {},
        std::string link = {})
      : color(std::move(fg)), bgcolor(std::move(bg)), link(std::move(link)) {
    for (auto [a, v] : attrs) Set(a, v);
  }

  static Style Null() { return Style(); }

  Style& On(const std::string& key, const std::string& value) {
    meta[key] = value;
    return *this;
  }

  void Set(Attribute a, bool value) {
    std::uint32_t bit = 1u << a;
    set_attributes |= bit;
    if (value)
      attributes |= bit;
    else
      attributes &= ~bit;
  }

  std::optional<bool> Get(Attribute a) const {
    std::uint32_t bit = 1u << a;
    if (!(set_attributes & bit)) return std::nullopt;
    return (attributes & bit) != 0;
  }

  bool IsEmpty() const {
    return !color && !bgcolor && set_attributes == 0 && link.empty() &&
           meta.empty();
  }
  explicit operator bool() const { return !IsEmpty(); }

  Style Combine(const Style& other) const {
    if (other.IsEmpty()) return *this;
    if (IsEmpty()) return other;
    Style s;
    s.color = other.color ? other.color : color;
    s.bgcolor = other.bgcolor ? other.bgcolor : bgcolor;
    s.attributes = (attributes & ~other.set_attributes) |
                   (other.attributes & other.set_attributes);
    s.set_attributes = set_attributes | other.set_attributes;
    s.link = !other.link.empty() ? other.link : link;
    s.meta = meta;
    for (const auto& [k, v] : other.meta) s.meta[k] = v;
    return s;
  }

  Style operator+(const Style& other) const { return Combine(other); }

  std::string MakeAnsiCodes(
      ColorSystem system = ColorSystem::kTrueColor) const {
    std::vector<std::string> sgr;
    for (std::uint32_t bit = 0; bit < kAttributeCount; ++bit) {
      std::uint32_t mask = 1u << bit;
      if ((set_attributes & mask) && (attributes & mask)) {
        sgr.push_back(std::to_string(detail::kAttributeSgr[bit]));
      }
    }
    if (color) {
      for (auto& c : color->Downgrade(system).GetAnsiCodes(true))
        sgr.push_back(c);
    }
    if (bgcolor) {
      for (auto& c : bgcolor->Downgrade(system).GetAnsiCodes(false))
        sgr.push_back(c);
    }
    std::string out;
    for (std::size_t i = 0; i < sgr.size(); ++i) {
      if (i) out.push_back(';');
      out += sgr[i];
    }
    return out;
  }

  std::string Render(std::string_view text,
                     ColorSystem system = ColorSystem::kTrueColor) const {
    if (IsEmpty()) return std::string(text);
    std::string body = MakeAnsiCodes(system);
    std::string rendered;
    if (!body.empty()) {
      rendered = "\x1b[" + body + "m" + std::string(text) + "\x1b[0m";
    } else {
      rendered = std::string(text);
    }
    if (!link.empty()) {
      std::string id = std::to_string(detail::NextLinkId());
      rendered = "\x1b]8;id=" + id + ";" + link + "\x1b\\" + rendered +
                 "\x1b]8;;\x1b\\";
    }
    return rendered;
  }

  static Style Parse(std::string_view definition);

  bool operator==(const Style& o) const {
    return color == o.color && bgcolor == o.bgcolor &&
           set_attributes == o.set_attributes && attributes == o.attributes &&
           link == o.link && meta == o.meta;
  }
};

}

namespace weqeqq::terminal::detail {

inline std::optional<Attribute> AttributeFromWord(std::string_view w) {
  if (w == "bold" || w == "b") return kBold;
  if (w == "dim" || w == "d") return kDim;
  if (w == "italic" || w == "i") return kItalic;
  if (w == "underline" || w == "u") return kUnderline;
  if (w == "blink") return kBlink;
  if (w == "blink2") return kBlink2;
  if (w == "reverse" || w == "r") return kReverse;
  if (w == "conceal" || w == "c") return kConceal;
  if (w == "strike" || w == "s") return kStrike;
  if (w == "underline2" || w == "uu") return kUnderline2;
  if (w == "frame") return kFrame;
  if (w == "encircle") return kEncircle;
  if (w == "overline" || w == "o") return kOverline;
  return std::nullopt;
}

}

namespace weqeqq::terminal {

inline Style Style::Parse(std::string_view definition) {
  std::vector<std::string> words;
  std::string cur;
  for (char c : definition) {
    if (std::isspace(static_cast<unsigned char>(c))) {
      if (!cur.empty()) {
        words.push_back(cur);
        cur.clear();
      }
    } else {
      cur.push_back(c);
    }
  }
  if (!cur.empty()) words.push_back(cur);

  Style style;
  if (words.empty()) return style;
  if (words.size() == 1 && words[0] == "none") return style;

  for (std::size_t i = 0; i < words.size(); ++i) {
    std::string lower;
    for (char c : words[i])
      lower.push_back(
          static_cast<char>(std::tolower(static_cast<unsigned char>(c))));

    if (lower == "on") {
      if (++i >= words.size()) break;
      style.bgcolor = Color::Parse(words[i]);
    } else if (lower == "not") {
      if (++i >= words.size()) break;
      std::string n;
      for (char c : words[i])
        n.push_back(
            static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
      if (auto a = detail::AttributeFromWord(n)) style.Set(*a, false);
    } else if (lower == "link") {
      if (++i >= words.size()) break;
      style.link = words[i];
    } else if (lower == "none") {
    } else if (auto a = detail::AttributeFromWord(lower)) {
      style.Set(*a, true);
    } else {
      style.color = Color::Parse(words[i]);
    }
  }
  return style;
}

}
