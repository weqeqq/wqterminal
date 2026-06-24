module;

#include <string>
#include <string_view>
#include <vector>

export module weqeqq.terminal:box;

import :cells;

export namespace weqeqq::terminal {

class Box {
 public:
  std::string top_left, top, top_divider, top_right;
  std::string head_left, head_vertical, head_right;
  std::string head_row_left, head_row_horizontal, head_row_cross,
      head_row_right;
  std::string mid_left, mid_vertical, mid_right;
  std::string row_left, row_horizontal, row_cross, row_right;
  std::string foot_row_left, foot_row_horizontal, foot_row_cross,
      foot_row_right;
  std::string foot_left, foot_vertical, foot_right;
  std::string bottom_left, bottom, bottom_divider, bottom_right;

  explicit Box(std::string_view tmpl) { Parse(tmpl); }

  enum class Level { kHead, kRow, kFoot, kMid };

  Box Substitute(bool ascii_only) const;

  std::string GetTop(const std::vector<int>& widths) const {
    return Build(top_left, top, top_divider, top_right, widths, true);
  }
  std::string GetBottom(const std::vector<int>& widths) const {
    return Build(bottom_left, bottom, bottom_divider, bottom_right, widths,
                 true);
  }

  std::string GetRow(const std::vector<int>& widths, Level level = Level::kRow,
                     bool edge = true) const {
    switch (level) {
      case Level::kHead:
        return Build(head_row_left, head_row_horizontal, head_row_cross,
                     head_row_right, widths, edge);
      case Level::kFoot:
        return Build(foot_row_left, foot_row_horizontal, foot_row_cross,
                     foot_row_right, widths, edge);
      case Level::kMid:
        return Build(mid_left, " ", mid_vertical, mid_right, widths, edge);
      case Level::kRow:
      default:
        return Build(row_left, row_horizontal, row_cross, row_right, widths,
                     edge);
    }
  }

 private:
  static std::string Build(const std::string& left, const std::string& fill,
                           const std::string& cross, const std::string& right,
                           const std::vector<int>& widths, bool edge) {
    std::string out;
    if (edge) out += left;
    for (std::size_t i = 0; i < widths.size(); ++i) {
      for (int w = 0; w < widths[i]; ++w) out += fill;
      if (i + 1 < widths.size()) out += cross;
    }
    if (edge) out += right;
    return out;
  }

  void Parse(std::string_view tmpl) {
    std::vector<std::vector<std::string>> rows;
    std::size_t pos = 0;
    while (pos <= tmpl.size() && rows.size() < 8) {
      std::size_t nl = tmpl.find('\n', pos);
      std::string_view line = tmpl.substr(
          pos, nl == std::string_view::npos ? tmpl.size() - pos : nl - pos);
      std::u32string cps = ToCodepoints(line);
      std::vector<std::string> glyphs;
      for (char32_t cp : cps) {
        glyphs.push_back(FromCodepoints(std::u32string(1, cp)));
      }
      while (glyphs.size() < 4) glyphs.push_back(" ");
      rows.push_back(glyphs);
      if (nl == std::string_view::npos) break;
      pos = nl + 1;
    }
    auto& r = rows;
    top_left = r[0][0];
    top = r[0][1];
    top_divider = r[0][2];
    top_right = r[0][3];
    head_left = r[1][0];
    head_vertical = r[1][2];
    head_right = r[1][3];
    head_row_left = r[2][0];
    head_row_horizontal = r[2][1];
    head_row_cross = r[2][2];
    head_row_right = r[2][3];
    mid_left = r[3][0];
    mid_vertical = r[3][2];
    mid_right = r[3][3];
    row_left = r[4][0];
    row_horizontal = r[4][1];
    row_cross = r[4][2];
    row_right = r[4][3];
    foot_row_left = r[5][0];
    foot_row_horizontal = r[5][1];
    foot_row_cross = r[5][2];
    foot_row_right = r[5][3];
    foot_left = r[6][0];
    foot_vertical = r[6][2];
    foot_right = r[6][3];
    bottom_left = r[7][0];
    bottom = r[7][1];
    bottom_divider = r[7][2];
    bottom_right = r[7][3];
  }
};

inline const Box kAscii(
    "+--+\n"
    "| ||\n"
    "|-+|\n"
    "| ||\n"
    "|-+|\n"
    "|-+|\n"
    "| ||\n"
    "+--+\n");

inline const Box kSquare(
    "┌─┬┐\n"
    "│ ││\n"
    "├─┼┤\n"
    "│ ││\n"
    "├─┼┤\n"
    "├─┼┤\n"
    "│ ││\n"
    "└─┴┘\n");

inline const Box kRounded(
    "╭─┬╮\n"
    "│ ││\n"
    "├─┼┤\n"
    "│ ││\n"
    "├─┼┤\n"
    "├─┼┤\n"
    "│ ││\n"
    "╰─┴╯\n");

inline const Box kHeavy(
    "┏━┳┓\n"
    "┃ ┃┃\n"
    "┣━╋┫\n"
    "┃ ┃┃\n"
    "┣━╋┫\n"
    "┣━╋┫\n"
    "┃ ┃┃\n"
    "┗━┻┛\n");

inline const Box kDouble(
    "╔═╦╗\n"
    "║ ║║\n"
    "╠═╬╣\n"
    "║ ║║\n"
    "╠═╬╣\n"
    "╠═╬╣\n"
    "║ ║║\n"
    "╚═╩╝\n");

inline const Box kAscii2(
    "+-++\n"
    "| ||\n"
    "+-++\n"
    "| ||\n"
    "+-++\n"
    "+-++\n"
    "| ||\n"
    "+-++\n");

inline const Box kAsciiDoubleHead(
    "+-++\n"
    "| ||\n"
    "+=++\n"
    "| ||\n"
    "+-++\n"
    "+-++\n"
    "| ||\n"
    "+-++\n");

inline const Box kSquareDoubleHead(
    "┌─┬┐\n"
    "│ ││\n"
    "╞═╪╡\n"
    "│ ││\n"
    "├─┼┤\n"
    "├─┼┤\n"
    "│ ││\n"
    "└─┴┘\n");

inline const Box kMinimal(
    "  ╷ \n"
    "  │ \n"
    "╶─┼╴\n"
    "  │ \n"
    "╶─┼╴\n"
    "╶─┼╴\n"
    "  │ \n"
    "  ╵ \n");

inline const Box kMinimalHeavyHead(
    "  ╷ \n"
    "  │ \n"
    "╺━┿╸\n"
    "  │ \n"
    "╶─┼╴\n"
    "╶─┼╴\n"
    "  │ \n"
    "  ╵ \n");

inline const Box kMinimalDoubleHead(
    "  ╷ \n"
    "  │ \n"
    " ═╪ \n"
    "  │ \n"
    " ─┼ \n"
    " ─┼ \n"
    "  │ \n"
    "  ╵ \n");

inline const Box kSimple(
    "    \n"
    "    \n"
    " ── \n"
    "    \n"
    "    \n"
    " ── \n"
    "    \n"
    "    \n");

inline const Box kSimpleHead(
    "    \n"
    "    \n"
    " ── \n"
    "    \n"
    "    \n"
    "    \n"
    "    \n"
    "    \n");

inline const Box kSimpleHeavy(
    "    \n"
    "    \n"
    " ━━ \n"
    "    \n"
    "    \n"
    " ━━ \n"
    "    \n"
    "    \n");

inline const Box kHorizontals(
    " ── \n"
    "    \n"
    " ── \n"
    "    \n"
    " ── \n"
    " ── \n"
    "    \n"
    " ── \n");

inline const Box kHeavyEdge(
    "┏━┯┓\n"
    "┃ │┃\n"
    "┠─┼┨\n"
    "┃ │┃\n"
    "┠─┼┨\n"
    "┠─┼┨\n"
    "┃ │┃\n"
    "┗━┷┛\n");

inline const Box kHeavyHead(
    "┏━┳┓\n"
    "┃ ┃┃\n"
    "┡━╇┩\n"
    "│ ││\n"
    "├─┼┤\n"
    "├─┼┤\n"
    "│ ││\n"
    "└─┴┘\n");

inline const Box kDoubleEdge(
    "╔═╤╗\n"
    "║ │║\n"
    "╟─┼╢\n"
    "║ │║\n"
    "╟─┼╢\n"
    "╟─┼╢\n"
    "║ │║\n"
    "╚═╧╝\n");

inline const Box kMarkdown(
    "    \n"
    "| ||\n"
    "|-||\n"
    "| ||\n"
    "|-||\n"
    "|-||\n"
    "| ||\n"
    "    \n");

inline Box Box::Substitute(bool ascii_only) const {
  return ascii_only ? kAscii : *this;
}

}
