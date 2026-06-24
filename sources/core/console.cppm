module;

#include <sys/ioctl.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

export module weqeqq.terminal:console;

import :protocol;
import :segment;
import :style;
import :color;
import :text;
import :markup;
import :measure;
import :control;
import :theme;
import :highlight;

export namespace weqeqq::terminal {

class Console {
 public:
  int width = 80;
  int height = 25;
  ColorSystem color_system = ColorSystem::kTrueColor;
  bool is_terminal = false;
  bool no_color = false;
  int tab_size = 8;
  std::FILE* file = stdout;

  bool highlight = true;
  RegexHighlighter highlighter = ReprHighlighter();

  std::optional<std::string> capture_buffer;

  Console() {
    theme_stack_.push_back(DefaultTheme());
    DetectEnvironment();
  }

  ~Console() {
    if (alt_screen_) {
      WriteRaw(control::DisableAltScreen());
      WriteRaw(control::ShowCursor());
    }
  }

  ConsoleOptions Options() const {
    ConsoleOptions o;
    o.min_width = 1;
    o.max_width = width;
    o.is_terminal = is_terminal;
    return o;
  }

  void PushTheme(Theme theme) { theme_stack_.push_back(std::move(theme)); }
  void PopTheme() {
    if (theme_stack_.size() > 1) theme_stack_.pop_back();
  }

  Style GetStyle(const std::string& name_or_def) const {
    for (auto it = theme_stack_.rbegin(); it != theme_stack_.rend(); ++it) {
      if (it->Has(name_or_def)) return it->Get(name_or_def);
    }
    return Style::Parse(name_or_def);
  }

  std::function<Style(std::string_view)> StyleResolver() const {
    return [this](std::string_view def) { return GetStyle(std::string(def)); };
  }

  Text RenderStr(std::string_view markup) const {
    return Text::FromMarkup(markup, Style{}, StyleResolver());
  }

  std::vector<Segment> Render(const Renderable& renderable,
                              std::optional<ConsoleOptions> opts = {}) {
    ConsoleOptions o = opts.value_or(Options());
    return renderable.Render(*this, o);
  }

  Measurement Measure(const Renderable& renderable,
                      std::optional<ConsoleOptions> opts = {}) {
    ConsoleOptions o = opts.value_or(Options());
    return renderable.Measure(*this, o).WithMaximum(o.max_width).Normalize();
  }

  std::vector<Line> RenderLines(const Renderable& renderable,
                                std::optional<ConsoleOptions> opts = {},
                                bool pad = true,
                                std::optional<Style> style = {}) {
    ConsoleOptions o = opts.value_or(Options());
    std::vector<Segment> segs = Render(renderable, o);
    if (style) segs = ApplyStyle(segs, *style);
    std::vector<Line> lines = SplitLines(segs);
    if (o.height) {
      if (static_cast<int>(lines.size()) > *o.height) lines.resize(*o.height);
      while (static_cast<int>(lines.size()) < *o.height)
        lines.push_back(Line{});
    }
    for (Line& line : lines) {
      line = AdjustLineLength(line, o.max_width, style, pad);
    }
    return lines;
  }

  void Print(const Renderable& renderable,
             std::optional<ConsoleOptions> opts = {}) {
    std::string s = SegmentsToString(Render(renderable, opts));
    if (s.empty() || s.back() != '\n') s.push_back('\n');
    WriteRaw(s);
  }

  void Print(std::string_view markup) {
    Text t = Text::FromMarkup(markup, Style{}, StyleResolver());
    if (highlight) highlighter.Highlight(t, theme_stack_.back());
    Print(Renderable(t));
  }
  void Print(const char* markup) { Print(std::string_view(markup)); }
  void Print(const std::string& markup) { Print(std::string_view(markup)); }

  void Print() { WriteRaw("\n"); }

  void HideCursor() { WriteRaw(control::HideCursor()); }
  void ShowCursor() { WriteRaw(control::ShowCursor()); }
  void Bell() { WriteRaw(control::Bell()); }

  void SetAltScreen(bool enable) {
    if (enable == alt_screen_) return;
    alt_screen_ = enable;
    WriteRaw(enable ? control::EnableAltScreen() : control::DisableAltScreen());
  }
  bool in_alt_screen() const { return alt_screen_; }

  void UpdateScreenLines(const std::vector<Line>& lines, int x = 0, int y = 0) {
    std::string out;
    for (std::size_t i = 0; i < lines.size(); ++i) {
      out += control::MoveTo(x, y + static_cast<int>(i));
      out += SegmentsToString(lines[i]);
    }
    WriteRaw(out);
  }

  void PrintText(std::string_view plain) {
    Print(Renderable(Text(std::string(plain))));
  }

  std::string SegmentsToString(const std::vector<Segment>& segs) const {
    std::string out;
    for (const Segment& seg : segs) {
      if (seg.is_control) {
        out += seg.text;
      } else if (seg.style && *seg.style && !no_color) {
        out += seg.style->Render(seg.text, color_system);
      } else {
        out += seg.text;
      }
    }
    return out;
  }

  std::string RenderToString(const Renderable& renderable,
                             std::optional<ConsoleOptions> opts = {}) {
    return SegmentsToString(Render(renderable, opts));
  }

  void WriteSegments(const std::vector<Segment>& segs) {
    WriteRaw(SegmentsToString(segs));
  }

  void WriteRaw(std::string_view data) {
    if (capture_buffer) {
      capture_buffer->append(data);
    } else {
      std::fwrite(data.data(), 1, data.size(), file);
      std::fflush(file);
    }
  }

  void BeginCapture() { capture_buffer = std::string(); }
  std::string EndCapture() {
    std::string out = capture_buffer.value_or(std::string());
    capture_buffer.reset();
    return out;
  }

 private:
  std::vector<Theme> theme_stack_;
  bool alt_screen_ = false;

  void DetectEnvironment() {
    is_terminal = isatty(fileno(stdout)) != 0;

    if (std::getenv("NO_COLOR")) {
      no_color = true;
      color_system = ColorSystem::kStandard;
    } else if (const char* ct = std::getenv("COLORTERM");
               ct &&
               (std::strstr(ct, "truecolor") || std::strstr(ct, "24bit"))) {
      color_system = ColorSystem::kTrueColor;
    } else if (const char* term = std::getenv("TERM");
               term && std::strstr(term, "256color")) {
      color_system = ColorSystem::kEightBit;
    } else {
      color_system = ColorSystem::kStandard;
    }

    struct winsize ws{};
    if (ioctl(fileno(stdout), TIOCGWINSZ, &ws) == 0) {
      if (ws.ws_col > 0) width = ws.ws_col;
      if (ws.ws_row > 0) height = ws.ws_row;
    }
    if (const char* cols = std::getenv("COLUMNS")) {
      int c = std::atoi(cols);
      if (c > 0) width = c;
    }
  }
};

}
