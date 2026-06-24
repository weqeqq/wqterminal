module;

#include <string>
#include <utility>

export module weqeqq.terminal:control;

import :protocol;
import :segment;

export namespace weqeqq::terminal {

enum class ControlType {
  kBell,
  kCarriageReturn,
  kHome,
  kClear,
  kShowCursor,
  kHideCursor,
  kEnableAltScreen,
  kDisableAltScreen,
  kCursorUp,
  kCursorDown,
  kCursorForward,
  kCursorBackward,
  kCursorMoveToColumn,
  kCursorMoveTo,
  kEraseInLine,
};

namespace control {

inline std::string Bell() { return "\a"; }
inline std::string CarriageReturn() { return "\r"; }
inline std::string Home() { return "\x1b[H"; }
inline std::string Clear() { return "\x1b[2J"; }
inline std::string ShowCursor() { return "\x1b[?25h"; }
inline std::string HideCursor() { return "\x1b[?25l"; }
inline std::string EnableAltScreen() { return "\x1b[?1049h"; }
inline std::string DisableAltScreen() { return "\x1b[?1049l"; }

inline std::string CursorUp(int n) {
  return n > 0 ? "\x1b[" + std::to_string(n) + "A" : std::string();
}
inline std::string CursorDown(int n) {
  return n > 0 ? "\x1b[" + std::to_string(n) + "B" : std::string();
}
inline std::string CursorForward(int n) {
  return n > 0 ? "\x1b[" + std::to_string(n) + "C" : std::string();
}
inline std::string CursorBackward(int n) {
  return n > 0 ? "\x1b[" + std::to_string(n) + "D" : std::string();
}

inline std::string MoveToColumn(int x) {
  return "\x1b[" + std::to_string(x + 1) + "G";
}
inline std::string MoveTo(int x, int y) {
  return "\x1b[" + std::to_string(y + 1) + ";" + std::to_string(x + 1) + "H";
}
inline std::string EraseInLine(int mode = 2) {
  return "\x1b[" + std::to_string(mode) + "K";
}
inline std::string EraseDown() { return "\x1b[J"; }

inline std::string MoveUp(int n) { return CursorUp(n); }
inline std::string EraseLine() { return EraseInLine(2); }

}

class Control : public IRenderable {
 public:
  std::string codes;

  Control() = default;
  explicit Control(std::string codes) : codes(std::move(codes)) {}

  static Control MoveTo(int x, int y) { return Control(control::MoveTo(x, y)); }
  static Control MoveToColumn(int x) {
    return Control(control::MoveToColumn(x));
  }
  static Control Bell() { return Control(control::Bell()); }
  static Control Home() { return Control(control::Home()); }
  static Control Clear() { return Control(control::Clear()); }
  static Control ShowCursor() { return Control(control::ShowCursor()); }
  static Control HideCursor() { return Control(control::HideCursor()); }

  RenderResult Render(Console&, const ConsoleOptions&) const override {
    return {Segment::Control(codes)};
  }
  Measurement Measure(Console&, const ConsoleOptions&) const override {
    return Measurement{0, 0};
  }
};

}
