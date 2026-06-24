module;

#include <algorithm>
#include <functional>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

export module weqeqq.terminal:text;

import :protocol;
import :segment;
import :style;
import :color;
import :measure;
import :cells;

export namespace weqeqq::terminal {

struct Span {
  int start = 0;
  int end = 0;
  Style style;
  bool operator==(const Span&) const = default;
};

}

namespace weqeqq::terminal::detail {

inline bool IsSpace(char32_t c) { return c == U' ' || c == U'\t'; }

inline int RstripLen(const std::u32string& s, int b, int e) {
  while (e > b && IsSpace(s[e - 1])) --e;
  return e - b;
}

inline std::vector<std::u32string> ChopCells(std::u32string_view word,
                                             int max_size, int position) {
  std::vector<std::u32string> lines;
  lines.emplace_back();
  int total = position;
  for (char32_t cp : word) {
    int sz = GetCodepointCellSize(cp);
    if (total + sz > max_size) {
      lines.emplace_back();
      lines.back().push_back(cp);
      total = sz;
    } else {
      lines.back().push_back(cp);
      total += sz;
    }
  }
  return lines;
}

inline std::vector<int> DivideLine(const std::u32string& text, int width,
                                   bool fold) {
  std::vector<int> divides;
  int line_position = 0;
  int n = static_cast<int>(text.size());
  int i = 0;
  while (i < n) {
    int start = i;
    while (i < n && IsSpace(text[i])) ++i;
    while (i < n && !IsSpace(text[i])) ++i;
    while (i < n && IsSpace(text[i])) ++i;
    if (i == start) break;
    std::u32string_view word(text.data() + start, i - start);
    int word_length = RstripLen(text, start, i);
    if (line_position + word_length > width) {
      if (word_length > width) {
        if (fold) {
          auto chopped = ChopCells(word, width, line_position);
          int run = start;
          for (std::size_t k = 0; k < chopped.size(); ++k) {
            bool last = k + 1 == chopped.size();
            if (run > 0) divides.push_back(run);
            if (last) {
              line_position = CellLen(std::u32string_view(chopped[k]));
            } else {
              run += static_cast<int>(chopped[k].size());
            }
          }
        } else {
          if (start > 0) divides.push_back(start);
          line_position = CellLen(word);
        }
      } else if (line_position > 0 && start > 0) {
        divides.push_back(start);
        line_position = CellLen(word);
      }
    } else {
      line_position += CellLen(word);
    }
  }
  return divides;
}

inline void ApplySgr(Style& s, std::string_view params) {
  std::vector<int> codes;
  int cur = 0;
  bool has = false;
  for (char c : params) {
    if (c == ';') {
      codes.push_back(has ? cur : 0);
      cur = 0;
      has = false;
    } else if (c >= '0' && c <= '9') {
      cur = cur * 10 + (c - '0');
      has = true;
    }
  }
  codes.push_back(has ? cur : 0);

  auto make_color = [](int n) {
    return Color("", n < 16 ? ColorType::kStandard : ColorType::kEightBit, n);
  };
  for (std::size_t i = 0; i < codes.size(); ++i) {
    int n = codes[i];
    switch (n) {
      case 0:
        s = Style{};
        break;
      case 1:
        s.Set(kBold, true);
        break;
      case 2:
        s.Set(kDim, true);
        break;
      case 3:
        s.Set(kItalic, true);
        break;
      case 4:
        s.Set(kUnderline, true);
        break;
      case 5:
        s.Set(kBlink, true);
        break;
      case 7:
        s.Set(kReverse, true);
        break;
      case 8:
        s.Set(kConceal, true);
        break;
      case 9:
        s.Set(kStrike, true);
        break;
      case 21:
        s.Set(kUnderline2, true);
        break;
      case 22:
        s.Set(kBold, false);
        s.Set(kDim, false);
        break;
      case 23:
        s.Set(kItalic, false);
        break;
      case 24:
        s.Set(kUnderline, false);
        break;
      case 25:
        s.Set(kBlink, false);
        break;
      case 27:
        s.Set(kReverse, false);
        break;
      case 28:
        s.Set(kConceal, false);
        break;
      case 29:
        s.Set(kStrike, false);
        break;
      case 39:
        s.color.reset();
        break;
      case 49:
        s.bgcolor.reset();
        break;
      case 53:
        s.Set(kOverline, true);
        break;
      case 38:
      case 48: {
        bool fg = n == 38;
        if (i + 1 < codes.size() && codes[i + 1] == 5 && i + 2 < codes.size()) {
          (fg ? s.color : s.bgcolor) = make_color(codes[i + 2]);
          i += 2;
        } else if (i + 1 < codes.size() && codes[i + 1] == 2 &&
                   i + 4 < codes.size()) {
          (fg ? s.color : s.bgcolor) =
              Color::FromRgb(codes[i + 2], codes[i + 3], codes[i + 4]);
          i += 4;
        }
        break;
      }
      default:
        if (n >= 30 && n <= 37)
          s.color = make_color(n - 30);
        else if (n >= 90 && n <= 97)
          s.color = make_color(n - 90 + 8);
        else if (n >= 40 && n <= 47)
          s.bgcolor = make_color(n - 40);
        else if (n >= 100 && n <= 107)
          s.bgcolor = make_color(n - 100 + 8);
        break;
    }
  }
}

inline int ByteToCp(std::string_view s, std::size_t byte_offset) {
  return static_cast<int>(ToCodepoints(s.substr(0, byte_offset)).size());
}

}

export namespace weqeqq::terminal {

class Text : public IRenderable {
 public:
  Style style;
  JustifyMethod justify = JustifyMethod::kDefault;
  OverflowMethod overflow = OverflowMethod::kFold;
  std::optional<bool> no_wrap;
  int tab_size = 8;

  Text() = default;
  Text(std::string text, Style style = {},
       JustifyMethod justify = JustifyMethod::kDefault,
       OverflowMethod overflow = OverflowMethod::kFold)
      : style(std::move(style)),
        justify(justify),
        overflow(overflow),
        cp_(ToCodepoints(text)) {}

  static Text FromMarkup(
      std::string_view markup, Style style = {},
      std::function<Style(std::string_view)> resolver = {});

  int Length() const { return static_cast<int>(cp_.size()); }
  int CellLength() const { return CellLen(std::u32string_view(cp_)); }
  std::string Plain() const { return FromCodepoints(cp_); }
  const std::u32string& Codepoints() const { return cp_; }
  const std::vector<Span>& Spans() const { return spans_; }
  bool IsEmpty() const { return cp_.empty(); }

  Text& Append(std::string text, Style s = {}) {
    int offset = Length();
    std::u32string add = ToCodepoints(text);
    cp_ += add;
    if (s)
      spans_.push_back(Span{offset, offset + static_cast<int>(add.size()), s});
    return *this;
  }

  Text& AppendText(const Text& other) {
    int offset = Length();
    if (other.style)
      spans_.push_back(Span{offset, offset + other.Length(), other.style});
    for (const Span& sp : other.spans_)
      spans_.push_back(Span{sp.start + offset, sp.end + offset, sp.style});
    cp_ += other.cp_;
    return *this;
  }

  Text& Stylize(Style s, int start, int end) {
    if (!s) return *this;
    start = std::clamp(start, 0, Length());
    end = std::clamp(end, 0, Length());
    if (start < end) spans_.push_back(Span{start, end, s});
    return *this;
  }

  static Text FromAnsi(std::string_view s) {
    Text out;
    Style cur;
    std::string run;
    auto flush = [&]() {
      if (!run.empty()) {
        out.Append(run, cur);
        run.clear();
      }
    };
    std::size_t i = 0;
    while (i < s.size()) {
      if (s[i] == '\x1b' && i + 1 < s.size() && s[i + 1] == '[') {
        std::size_t j = i + 2;
        while (j < s.size() && static_cast<unsigned char>(s[j]) >= 0x30 &&
               static_cast<unsigned char>(s[j]) <= 0x3f)
          ++j;
        while (j < s.size() && static_cast<unsigned char>(s[j]) >= 0x20 &&
               static_cast<unsigned char>(s[j]) <= 0x2f)
          ++j;
        if (j < s.size()) {
          if (s[j] == 'm') {
            flush();
            detail::ApplySgr(cur, s.substr(i + 2, j - (i + 2)));
          }
          i = j + 1;
          continue;
        }
      }
      run.push_back(s[i]);
      ++i;
    }
    flush();
    return out;
  }

  static Text Assemble(std::vector<std::pair<std::string, Style>> parts) {
    Text out;
    for (auto& [t, st] : parts) out.Append(t, st);
    return out;
  }

  Text& HighlightRegex(const std::string& pattern, Style style) {
    if (!style) return *this;
    std::string p = Plain();
    try {
      std::regex re(pattern);
      for (auto it = std::sregex_iterator(p.begin(), p.end(), re);
           it != std::sregex_iterator(); ++it) {
        const std::smatch& mt = *it;
        if (mt.length() == 0) continue;
        int a = detail::ByteToCp(p, static_cast<std::size_t>(mt.position()));
        int b = detail::ByteToCp(
            p, static_cast<std::size_t>(mt.position() + mt.length()));
        Stylize(style, a, b);
      }
    } catch (const std::regex_error&) {
    }
    return *this;
  }

  Text& HighlightWords(const std::vector<std::string>& words, Style style) {
    if (!style) return *this;
    std::string p = Plain();
    for (const std::string& w : words) {
      if (w.empty()) continue;
      std::size_t pos = 0;
      while ((pos = p.find(w, pos)) != std::string::npos) {
        int a = detail::ByteToCp(p, pos);
        int b = detail::ByteToCp(p, pos + w.size());
        Stylize(style, a, b);
        pos += w.size();
      }
    }
    return *this;
  }

  Style GetStyleAtOffset(int offset) const {
    Style st = style;
    for (const Span& sp : spans_)
      if (sp.start <= offset && offset < sp.end) st = st.Combine(sp.style);
    return st;
  }

  Text Slice(int start, int end) const {
    start = std::clamp(start, 0, Length());
    end = std::clamp(end, 0, Length());
    Text out;
    out.style = style;
    out.justify = justify;
    out.overflow = overflow;
    out.no_wrap = no_wrap;
    out.tab_size = tab_size;
    out.cp_ = cp_.substr(start, end - start);
    for (const Span& sp : spans_) {
      int ns = std::max(sp.start, start) - start;
      int ne = std::min(sp.end, end) - start;
      if (ns < ne) out.spans_.push_back(Span{ns, ne, sp.style});
    }
    return out;
  }

  std::vector<Text> Divide(const std::vector<int>& offsets) const {
    std::vector<int> bounds;
    bounds.push_back(0);
    for (int o : offsets) bounds.push_back(std::clamp(o, 0, Length()));
    bounds.push_back(Length());
    std::vector<Text> lines;
    for (std::size_t i = 0; i + 1 < bounds.size(); ++i) {
      lines.push_back(Slice(bounds[i], bounds[i + 1]));
    }
    return lines;
  }

  std::vector<Text> Split(char32_t sep) const {
    std::vector<Text> parts;
    int start = 0;
    for (int i = 0; i < Length(); ++i) {
      if (cp_[i] == sep) {
        parts.push_back(Slice(start, i));
        start = i + 1;
      }
    }
    parts.push_back(Slice(start, Length()));
    return parts;
  }

  Text Join(const std::vector<Text>& parts) const {
    Text out;
    for (std::size_t i = 0; i < parts.size(); ++i) {
      if (i) out.AppendText(*this);
      out.AppendText(parts[i]);
    }
    return out;
  }

  Text& PadLeft(int count) {
    if (count <= 0) return *this;
    cp_.insert(cp_.begin(), count, U' ');
    for (Span& sp : spans_) {
      sp.start += count;
      sp.end += count;
    }
    return *this;
  }
  Text& PadRight(int count) {
    if (count > 0) cp_.append(count, U' ');
    return *this;
  }

  Text& Rstrip() {
    int e = Length();
    while (e > 0 && detail::IsSpace(cp_[e - 1])) --e;
    return TruncateToLength(e);
  }

  Text& RstripEnd(int size) {
    if (CellLength() > size) {
      int e = Length();
      while (e > 0 && detail::IsSpace(cp_[e - 1])) --e;
      TruncateToLength(e);
    }
    return *this;
  }

  Text& Truncate(int width, OverflowMethod method, bool pad) {
    width = std::max(0, width);
    int cl = CellLength();
    if (cl > width && (method == OverflowMethod::kCrop ||
                       method == OverflowMethod::kEllipsis)) {
      bool ellipsis = method == OverflowMethod::kEllipsis && width >= 1;
      int target = ellipsis ? width - 1 : width;
      int acc = 0, cut = 0;
      for (int i = 0; i < Length(); ++i) {
        int w = GetCodepointCellSize(cp_[i]);
        if (acc + w > target) break;
        acc += w;
        cut = i + 1;
      }
      TruncateToLength(cut);
      if (ellipsis) cp_.push_back(U'…');
    }
    if (pad && CellLength() < width) PadRight(width - CellLength());
    return *this;
  }

  void ExpandTabs() {
    if (cp_.find(U'\t') == std::u32string::npos) return;
    std::u32string out;
    std::vector<int> map(cp_.size() + 1, 0);
    int col = 0;
    for (int i = 0; i < Length(); ++i) {
      map[i] = static_cast<int>(out.size());
      if (cp_[i] == U'\t') {
        int spaces = tab_size - (col % tab_size);
        if (spaces == 0) spaces = tab_size;
        out.append(spaces, U' ');
        col += spaces;
      } else {
        out.push_back(cp_[i]);
        col += GetCodepointCellSize(cp_[i]);
      }
    }
    map[Length()] = static_cast<int>(out.size());
    for (Span& sp : spans_) {
      sp.start = map[sp.start];
      sp.end = map[sp.end];
    }
    cp_ = std::move(out);
  }

  std::vector<Text> Wrap(int width, JustifyMethod jm, OverflowMethod om,
                         bool no_wrap_flag, int tab) {
    Text work = *this;
    work.tab_size = tab;
    work.ExpandTabs();
    std::vector<Text> all_lines;
    for (Text& raw : work.Split(U'\n')) {
      std::vector<Text> new_lines;
      if (no_wrap_flag) {
        new_lines.push_back(raw);
      } else {
        auto offsets =
            detail::DivideLine(raw.cp_, width, om == OverflowMethod::kFold);
        new_lines = raw.Divide(offsets);
      }
      for (Text& line : new_lines) line.RstripEnd(width);
      if (jm != JustifyMethod::kDefault) ApplyJustify(new_lines, width, jm, om);
      for (Text& line : new_lines) line.Truncate(width, om, false);
      for (Text& line : new_lines) all_lines.push_back(std::move(line));
    }
    return all_lines;
  }

  std::vector<Segment> RenderLine() const {
    std::vector<Segment> out;
    if (spans_.empty() && !style) {
      if (!cp_.empty()) out.emplace_back(FromCodepoints(cp_));
      return out;
    }
    std::vector<int> bounds = {0, Length()};
    for (const Span& sp : spans_) {
      bounds.push_back(std::clamp(sp.start, 0, Length()));
      bounds.push_back(std::clamp(sp.end, 0, Length()));
    }
    std::sort(bounds.begin(), bounds.end());
    bounds.erase(std::unique(bounds.begin(), bounds.end()), bounds.end());
    std::vector<Span> ordered = spans_;
    std::stable_sort(ordered.begin(), ordered.end(),
                     [](const Span& a, const Span& b) {
                       if (a.start != b.start) return a.start < b.start;
                       return a.end > b.end;
                     });
    for (std::size_t i = 0; i + 1 < bounds.size(); ++i) {
      int a = bounds[i], b = bounds[i + 1];
      if (a >= b) continue;
      Style st = style;
      for (const Span& sp : ordered) {
        if (sp.start <= a && b <= sp.end) st = st.Combine(sp.style);
      }
      std::optional<Style> os;
      if (st) os = st;
      out.emplace_back(FromCodepoints(cp_.substr(a, b - a)), os);
    }
    return out;
  }

  RenderResult Render(Console&, const ConsoleOptions& options) const override {
    JustifyMethod jm = justify != JustifyMethod::kDefault
                           ? justify
                           : options.justify.value_or(JustifyMethod::kDefault);
    OverflowMethod om = options.overflow.value_or(overflow);
    bool nw = no_wrap.value_or(options.no_wrap.value_or(false));
    Text self = *this;
    auto lines = self.Wrap(options.max_width, jm, om, nw, tab_size);
    RenderResult out;
    for (const Text& line : lines) {
      auto segs = line.RenderLine();
      out.insert(out.end(), segs.begin(), segs.end());
      out.push_back(Segment("\n"));
    }
    return out;
  }

  Measurement Measure(Console&, const ConsoleOptions& options) const override {
    int max_width = 0;
    int min_width = 0;
    int line_start = 0;
    int line_cells = 0;
    int word_cells = 0;
    auto flush_line = [&]() {
      max_width = std::max(max_width, line_cells);
      line_cells = 0;
    };
    auto flush_word = [&]() {
      min_width = std::max(min_width, word_cells);
      word_cells = 0;
    };
    for (int i = 0; i < Length(); ++i) {
      char32_t c = cp_[i];
      if (c == U'\n') {
        flush_word();
        flush_line();
      } else {
        int w = GetCodepointCellSize(c);
        line_cells += w;
        if (detail::IsSpace(c))
          flush_word();
        else
          word_cells += w;
      }
    }
    flush_word();
    flush_line();
    (void)line_start;
    if (min_width == 0) min_width = max_width;
    return Measurement{std::min(min_width, max_width), max_width}.WithMaximum(
        options.max_width);
  }

 private:
  std::u32string cp_;
  std::vector<Span> spans_;

  Text& TruncateToLength(int new_len) {
    new_len = std::clamp(new_len, 0, Length());
    cp_.resize(new_len);
    std::vector<Span> kept;
    for (Span& sp : spans_) {
      int e = std::min(sp.end, new_len);
      if (sp.start < e) kept.push_back(Span{sp.start, e, sp.style});
    }
    spans_ = std::move(kept);
    return *this;
  }

  static void ApplyJustify(std::vector<Text>& lines, int width,
                           JustifyMethod jm, OverflowMethod om) {
    if (jm == JustifyMethod::kLeft) {
      for (Text& line : lines) line.Truncate(width, om, true);
    } else if (jm == JustifyMethod::kCenter) {
      for (Text& line : lines) {
        line.Rstrip();
        line.Truncate(width, om, false);
        line.PadLeft((width - line.CellLength()) / 2);
        line.PadRight(width - line.CellLength());
      }
    } else if (jm == JustifyMethod::kRight) {
      for (Text& line : lines) {
        line.Rstrip();
        line.Truncate(width, om, false);
        line.PadLeft(width - line.CellLength());
      }
    } else if (jm == JustifyMethod::kFull) {
      for (std::size_t li = 0; li + 1 < lines.size(); ++li) {
        Text& line = lines[li];
        std::vector<Text> words = line.Split(U' ');
        int words_size = 0;
        for (const Text& w : words) words_size += w.CellLength();
        int num_spaces = static_cast<int>(words.size()) - 1;
        if (num_spaces <= 0) continue;
        std::vector<int> spaces(num_spaces, 1);
        int idx = 0;
        while (words_size + num_spaces < width) {
          spaces[spaces.size() - idx - 1] += 1;
          ++num_spaces;
          idx = (idx + 1) % static_cast<int>(spaces.size());
        }
        Text rebuilt;
        for (std::size_t wi = 0; wi < words.size(); ++wi) {
          rebuilt.AppendText(words[wi]);
          if (wi < spaces.size())
            rebuilt.Append(std::string(spaces[wi], ' '), line.style);
        }
        lines[li] = std::move(rebuilt);
      }
    }
  }
};

}
