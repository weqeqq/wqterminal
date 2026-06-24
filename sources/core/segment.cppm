module;

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

export module weqeqq.terminal:segment;

import :style;
import :cells;

export namespace weqeqq::terminal {

struct Segment {
  std::string text;
  std::optional<Style> style;
  bool is_control = false;

  Segment() = default;
  Segment(std::string text, std::optional<Style> style = std::nullopt,
          bool is_control = false)
      : text(std::move(text)),
        style(std::move(style)),
        is_control(is_control) {}

  int CellLength() const { return is_control ? 0 : CellLen(text); }

  bool operator==(const Segment&) const = default;

  static Segment Line() { return Segment("\n"); }
  static Segment Control(std::string codes) {
    return Segment(std::move(codes), std::nullopt, true);
  }
};

using Line = std::vector<Segment>;

inline int LineLength(const Line& line) {
  int total = 0;
  for (const Segment& s : line) total += s.CellLength();
  return total;
}

inline std::vector<Line> SplitLines(const std::vector<Segment>& segments) {
  std::vector<Line> lines;
  Line current;
  for (const Segment& segment : segments) {
    if (!segment.is_control && segment.text.find('\n') != std::string::npos) {
      std::string_view text = segment.text;
      std::size_t start = 0;
      while (start <= text.size()) {
        std::size_t nl = text.find('\n', start);
        if (nl == std::string_view::npos) {
          std::string_view rest = text.substr(start);
          if (!rest.empty())
            current.emplace_back(std::string(rest), segment.style);
          break;
        }
        std::string_view piece = text.substr(start, nl - start);
        if (!piece.empty())
          current.emplace_back(std::string(piece), segment.style);
        lines.push_back(std::move(current));
        current = Line();
        start = nl + 1;
      }
    } else {
      current.push_back(segment);
    }
  }
  if (!current.empty()) lines.push_back(std::move(current));
  return lines;
}

inline Line AdjustLineLength(const Line& line, int Length,
                             std::optional<Style> style = std::nullopt,
                             bool pad = true) {
  int len = LineLength(line);
  if (len < Length) {
    Line out = line;
    if (pad) out.emplace_back(std::string(Length - len, ' '), style);
    return out;
  }
  if (len > Length) {
    Line out;
    int acc = 0;
    for (const Segment& segment : line) {
      int seg_len = segment.CellLength();
      if (acc + seg_len < Length || segment.is_control) {
        out.push_back(segment);
        acc += seg_len;
      } else {
        out.emplace_back(SetCellSize(segment.text, Length - acc),
                         segment.style);
        break;
      }
    }
    return out;
  }
  return line;
}

inline std::vector<Segment> simplify(const std::vector<Segment>& segments) {
  std::vector<Segment> out;
  for (const Segment& segment : segments) {
    if (!out.empty() && !segment.is_control && !out.back().is_control &&
        out.back().style == segment.style) {
      out.back().text += segment.text;
    } else {
      out.push_back(segment);
    }
  }
  return out;
}

inline std::vector<Segment> ApplyStyle(const std::vector<Segment>& segments,
                                       const Style& style) {
  if (!style) return segments;
  std::vector<Segment> out;
  out.reserve(segments.size());
  for (const Segment& s : segments) {
    if (s.is_control) {
      out.push_back(s);
    } else {
      Style combined = s.style ? style.Combine(*s.style) : style;
      out.emplace_back(s.text, combined);
    }
  }
  return out;
}

inline std::pair<int, int> GetShape(const std::vector<Line>& lines) {
  int width = 0;
  for (const Line& line : lines) width = std::max(width, LineLength(line));
  return {width, static_cast<int>(lines.size())};
}

inline std::vector<Line> SetShape(const std::vector<Line>& lines, int width,
                                  int height = -1,
                                  std::optional<Style> style = std::nullopt) {
  std::vector<Line> out;
  out.reserve(lines.size());
  for (const Line& line : lines) {
    out.push_back(AdjustLineLength(line, width, style));
  }
  if (height >= 0) {
    while (static_cast<int>(out.size()) < height) {
      out.push_back(Line{Segment(std::string(width, ' '), style)});
    }
    if (static_cast<int>(out.size()) > height) out.resize(height);
  }
  return out;
}

inline std::string SliceTextCells(std::string_view text, int c0, int c1) {
  std::u32string cps = ToCodepoints(text);
  std::u32string out;
  int cell = 0;
  for (char32_t cp : cps) {
    int w = GetCodepointCellSize(cp);
    int cs = cell;
    int ce = cell + w;
    cell = ce;
    if (ce <= c0) continue;
    if (cs >= c1) break;
    if (cs >= c0 && ce <= c1) {
      out.push_back(cp);
    } else {
      int lo = std::max(cs, c0);
      int hi = std::min(ce, c1);
      out.append(static_cast<std::size_t>(std::max(0, hi - lo)), U' ');
    }
  }
  return FromCodepoints(out);
}

inline Line CropLine(const Line& line, int x, int width) {
  int end = x + width;
  Line out;
  int pos = 0;
  for (const Segment& seg : line) {
    if (seg.is_control) {
      if (pos >= x && pos < end) out.push_back(seg);
      continue;
    }
    int seg_len = seg.CellLength();
    int seg_start = pos;
    int seg_end = pos + seg_len;
    pos = seg_end;
    int a = std::max(seg_start, x);
    int b = std::min(seg_end, end);
    if (a >= b) continue;
    if (a == seg_start && b == seg_end) {
      out.push_back(seg);
    } else {
      out.emplace_back(SliceTextCells(seg.text, a - seg_start, b - seg_start),
                       seg.style);
    }
  }
  return out;
}

inline std::vector<Line> Divide(const Line& line,
                                const std::vector<int>& cuts) {
  std::vector<int> bounds;
  bounds.push_back(0);
  for (int c : cuts) bounds.push_back(c);
  bounds.push_back(LineLength(line));
  std::vector<Line> out;
  for (std::size_t i = 0; i + 1 < bounds.size(); ++i) {
    int a = bounds[i];
    int b = bounds[i + 1];
    out.push_back(CropLine(line, a, std::max(0, b - a)));
  }
  return out;
}

}
