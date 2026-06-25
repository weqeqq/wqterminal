module;

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

export module weqeqq.terminal:color;

export namespace weqeqq::terminal {

enum class ColorSystem {
  kStandard = 1,
  kEightBit = 2,
  kTrueColor = 3,
  kWindows = 4
};

enum class ColorType {
  kDefault = 0,
  kStandard = 1,
  kEightBit = 2,
  kTrueColor = 3,
  kWindows = 4
};

struct ColorTriplet {
  std::uint8_t red = 0;
  std::uint8_t green = 0;
  std::uint8_t blue = 0;

  std::string Hex() const {
    char buf[8];
    std::snprintf(buf, sizeof(buf), "#%02x%02x%02x", red, green, blue);
    return std::string(buf);
  }

  std::array<double, 3> Normalized() const {
    return {red / 255.0, green / 255.0, blue / 255.0};
  }

  constexpr bool operator==(const ColorTriplet&) const = default;
};

}

namespace weqeqq::terminal::detail {

constexpr std::array<ColorTriplet, 256> MakeEightBitPalette() {
  std::array<ColorTriplet, 256> p{};
  constexpr std::array<ColorTriplet, 16> system = {{
      {0, 0, 0},
      {128, 0, 0},
      {0, 128, 0},
      {128, 128, 0},
      {0, 0, 128},
      {128, 0, 128},
      {0, 128, 128},
      {192, 192, 192},
      {128, 128, 128},
      {255, 0, 0},
      {0, 255, 0},
      {255, 255, 0},
      {0, 0, 255},
      {255, 0, 255},
      {0, 255, 255},
      {255, 255, 255},
  }};
  for (int i = 0; i < 16; ++i) p[i] = system[i];
  constexpr std::array<int, 6> levels = {0, 95, 135, 175, 215, 255};
  for (int r = 0; r < 6; ++r)
    for (int g = 0; g < 6; ++g)
      for (int b = 0; b < 6; ++b) {
        int idx = 16 + 36 * r + 6 * g + b;
        p[idx] = ColorTriplet{static_cast<std::uint8_t>(levels[r]),
                              static_cast<std::uint8_t>(levels[g]),
                              static_cast<std::uint8_t>(levels[b])};
      }
  for (int i = 0; i < 24; ++i) {
    int v = 8 + 10 * i;
    p[232 + i] =
        ColorTriplet{static_cast<std::uint8_t>(v), static_cast<std::uint8_t>(v),
                     static_cast<std::uint8_t>(v)};
  }
  return p;
}

inline constexpr std::array<ColorTriplet, 256> kEightBitPalette =
    MakeEightBitPalette();

inline constexpr std::array<ColorTriplet, 16> kStandardPalette = {{
    {0, 0, 0},
    {170, 0, 0},
    {0, 170, 0},
    {170, 85, 0},
    {0, 0, 170},
    {170, 0, 170},
    {0, 170, 170},
    {170, 170, 170},
    {85, 85, 85},
    {255, 85, 85},
    {85, 255, 85},
    {255, 255, 85},
    {85, 85, 255},
    {255, 85, 255},
    {85, 255, 255},
    {255, 255, 255},
}};

inline constexpr std::array<ColorTriplet, 16> kWindowsPalette = {{
    {12, 12, 12},
    {197, 15, 31},
    {19, 161, 14},
    {193, 156, 0},
    {0, 55, 218},
    {136, 23, 152},
    {58, 150, 221},
    {204, 204, 204},
    {118, 118, 118},
    {231, 72, 86},
    {22, 198, 12},
    {249, 241, 165},
    {59, 120, 255},
    {180, 0, 158},
    {97, 214, 214},
    {242, 242, 242},
}};

template <std::size_t N>
int PaletteMatch(const std::array<ColorTriplet, N>& palette, ColorTriplet c) {
  int best = 0;
  double best_dist = 1e30;
  for (std::size_t i = 0; i < palette.size(); ++i) {
    int r1 = c.red, g1 = c.green, b1 = c.blue;
    int r2 = palette[i].red, g2 = palette[i].green, b2 = palette[i].blue;
    int red_mean = (r1 + r2) / 2;
    int dr = r1 - r2, dg = g1 - g2, db = b1 - b2;
    double dist = (((512 + red_mean) * dr * dr) >> 8) + 4.0 * dg * dg +
                  (((767 - red_mean) * db * db) >> 8);
    if (dist < best_dist) {
      best_dist = dist;
      best = static_cast<int>(i);
    }
  }
  return best;
}

inline std::array<double, 2> RgbToLs(double r, double g, double b) {
  double maxc = std::max({r, g, b});
  double minc = std::min({r, g, b});
  double l = (minc + maxc) / 2.0;
  if (minc == maxc) return {l, 0.0};
  double s;
  if (l <= 0.5) {
    s = (maxc - minc) / (maxc + minc);
  } else {
    s = (maxc - minc) / (2.0 - maxc - minc);
  }
  return {l, s};
}

const std::unordered_map<std::string, int>& AnsiColorNames();

}

export namespace weqeqq::terminal {

class ColorParseError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

class Color {
 public:
  std::string name;
  ColorType type = ColorType::kDefault;
  std::optional<int> number;
  std::optional<ColorTriplet> triplet;

  Color() = default;
  Color(std::string name, ColorType type, std::optional<int> number = {},
        std::optional<ColorTriplet> triplet = {})
      : name(std::move(name)), type(type), number(number), triplet(triplet) {}

  bool IsDefault() const { return type == ColorType::kDefault; }

  ColorSystem System() const {
    switch (type) {
      case ColorType::kDefault:
      case ColorType::kStandard:
        return ColorSystem::kStandard;
      case ColorType::kWindows:
        return ColorSystem::kWindows;
      case ColorType::kEightBit:
        return ColorSystem::kEightBit;
      case ColorType::kTrueColor:
      default:
        return ColorSystem::kTrueColor;
    }
  }

  static Color FromTriplet(ColorTriplet triplet) {
    return Color(triplet.Hex(), ColorType::kTrueColor, {}, triplet);
  }

  static Color FromRgb(int r, int g, int b) {
    return FromTriplet(ColorTriplet{static_cast<std::uint8_t>(r),
                                    static_cast<std::uint8_t>(g),
                                    static_cast<std::uint8_t>(b)});
  }

  static Color DefaultColor() { return Color("default", ColorType::kDefault); }

  ColorTriplet GetTrueColor(bool foreground = true) const {
    switch (type) {
      case ColorType::kTrueColor:
        return triplet.value_or(ColorTriplet{});
      case ColorType::kEightBit:
        return detail::kEightBitPalette[std::clamp(number.value_or(0), 0, 255)];
      case ColorType::kStandard:
        return detail::kStandardPalette[(number.value_or(0)) & 0x0f];
      case ColorType::kWindows:
        return detail::kWindowsPalette[(number.value_or(0)) & 0x0f];
      case ColorType::kDefault:
      default:
        return foreground ? ColorTriplet{255, 255, 255} : ColorTriplet{0, 0, 0};
    }
  }

  Color Blend(const Color& other, double t) const {
    ColorTriplet a = GetTrueColor();
    ColorTriplet b = other.GetTrueColor();
    auto lerp = [&](int x, int y) {
      return static_cast<std::uint8_t>(
          std::lround(x + (y - x) * std::clamp(t, 0.0, 1.0)));
    };
    return Color::FromTriplet(ColorTriplet{
        lerp(a.red, b.red), lerp(a.green, b.green), lerp(a.blue, b.blue)});
  }

  std::vector<std::string> GetAnsiCodes(bool foreground = true) const {
    switch (type) {
      case ColorType::kDefault:
        return {foreground ? "39" : "49"};
      case ColorType::kWindows:
      case ColorType::kStandard: {
        int n = number.value_or(0);
        int base = (n < 8) ? (foreground ? 30 : 40) : (foreground ? 82 : 92);
        return {std::to_string(base + n)};
      }
      case ColorType::kEightBit:
        return {foreground ? "38" : "48", "5",
                std::to_string(number.value_or(0))};
      case ColorType::kTrueColor:
      default: {
        ColorTriplet t = triplet.value_or(ColorTriplet{});
        return {foreground ? "38" : "48", "2", std::to_string(t.red),
                std::to_string(t.green), std::to_string(t.blue)};
      }
    }
  }

  Color Downgrade(ColorSystem target) const {
    if (type == ColorType::kDefault || type == ColorType::kWindows)
      return *this;

    if (target == ColorSystem::kEightBit &&
        System() == ColorSystem::kTrueColor) {
      ColorTriplet t = triplet.value();
      auto norm = t.Normalized();
      auto ls = detail::RgbToLs(norm[0], norm[1], norm[2]);
      double l = ls[0], s = ls[1];
      if (s < 0.15) {
        int gray = static_cast<int>(std::lround(l * 25.0));
        int color_number;
        if (gray == 0)
          color_number = 16;
        else if (gray == 25)
          color_number = 231;
        else
          color_number = 231 + gray;
        return Color(name, ColorType::kEightBit, color_number);
      }
      auto six = [](int v) {
        return v < 95 ? v / 95.0 : 1.0 + (v - 95) / 40.0;
      };
      int cn = 16 + 36 * static_cast<int>(std::lround(six(t.red))) +
               6 * static_cast<int>(std::lround(six(t.green))) +
               static_cast<int>(std::lround(six(t.blue)));
      return Color(name, ColorType::kEightBit, cn);
    }

    if (target == ColorSystem::kStandard) {
      ColorTriplet t = (System() == ColorSystem::kTrueColor)
                           ? triplet.value()
                           : detail::kEightBitPalette[number.value_or(0)];
      int cn = detail::PaletteMatch(detail::kStandardPalette, t);
      return Color(name, ColorType::kStandard, cn);
    }
    return *this;
  }

  static Color Parse(std::string_view text);

  bool operator==(const Color& o) const {
    return name == o.name && type == o.type && number == o.number &&
           triplet == o.triplet;
  }
};

}

namespace weqeqq::terminal::detail {

inline const std::unordered_map<std::string, int>& AnsiColorNames() {
  static const std::unordered_map<std::string, int> names = {
      {"black", 0},
      {"red", 1},
      {"green", 2},
      {"yellow", 3},
      {"blue", 4},
      {"magenta", 5},
      {"cyan", 6},
      {"white", 7},
      {"bright_black", 8},
      {"bright_red", 9},
      {"bright_green", 10},
      {"bright_yellow", 11},
      {"bright_blue", 12},
      {"bright_magenta", 13},
      {"bright_cyan", 14},
      {"bright_white", 15},
      {"gray0", 16},
      {"grey0", 16},
      {"navy_blue", 17},
      {"dark_blue", 18},
      {"blue3", 20},
      {"blue1", 21},
      {"dark_green", 22},
      {"deep_sky_blue4", 25},
      {"dodger_blue3", 26},
      {"dodger_blue2", 27},
      {"green4", 28},
      {"spring_green4", 29},
      {"turquoise4", 30},
      {"deep_sky_blue3", 32},
      {"dodger_blue1", 33},
      {"dark_cyan", 36},
      {"light_sea_green", 37},
      {"deep_sky_blue2", 38},
      {"deep_sky_blue1", 39},
      {"green3", 40},
      {"spring_green3", 41},
      {"cyan3", 43},
      {"dark_turquoise", 44},
      {"turquoise2", 45},
      {"green1", 46},
      {"spring_green2", 47},
      {"spring_green1", 48},
      {"medium_spring_green", 49},
      {"cyan2", 50},
      {"cyan1", 51},
      {"purple4", 55},
      {"purple3", 56},
      {"blue_violet", 57},
      {"gray37", 59},
      {"grey37", 59},
      {"medium_purple4", 60},
      {"slate_blue3", 62},
      {"royal_blue1", 63},
      {"chartreuse4", 64},
      {"pale_turquoise4", 66},
      {"steel_blue", 67},
      {"steel_blue3", 68},
      {"cornflower_blue", 69},
      {"dark_sea_green4", 71},
      {"cadet_blue", 73},
      {"sky_blue3", 74},
      {"chartreuse3", 76},
      {"sea_green3", 78},
      {"aquamarine3", 79},
      {"medium_turquoise", 80},
      {"steel_blue1", 81},
      {"sea_green2", 83},
      {"sea_green1", 85},
      {"dark_slate_gray2", 87},
      {"dark_red", 88},
      {"dark_magenta", 91},
      {"orange4", 94},
      {"light_pink4", 95},
      {"plum4", 96},
      {"medium_purple3", 98},
      {"slate_blue1", 99},
      {"wheat4", 101},
      {"gray53", 102},
      {"grey53", 102},
      {"light_slate_gray", 103},
      {"light_slate_grey", 103},
      {"medium_purple", 104},
      {"light_slate_blue", 105},
      {"yellow4", 106},
      {"dark_sea_green", 108},
      {"light_sky_blue3", 110},
      {"sky_blue2", 111},
      {"chartreuse2", 112},
      {"pale_green3", 114},
      {"dark_slate_gray3", 116},
      {"sky_blue1", 117},
      {"chartreuse1", 118},
      {"light_green", 120},
      {"aquamarine1", 122},
      {"dark_slate_gray1", 123},
      {"deep_pink4", 125},
      {"medium_violet_red", 126},
      {"dark_violet", 128},
      {"purple", 129},
      {"medium_orchid3", 133},
      {"medium_orchid", 134},
      {"dark_goldenrod", 136},
      {"rosy_brown", 138},
      {"gray63", 139},
      {"grey63", 139},
      {"medium_purple2", 140},
      {"medium_purple1", 141},
      {"dark_khaki", 143},
      {"navajo_white3", 144},
      {"gray69", 145},
      {"grey69", 145},
      {"light_steel_blue3", 146},
      {"light_steel_blue", 147},
      {"dark_olive_green3", 149},
      {"dark_sea_green3", 150},
      {"light_cyan3", 152},
      {"light_sky_blue1", 153},
      {"green_yellow", 154},
      {"dark_olive_green2", 155},
      {"pale_green1", 156},
      {"dark_sea_green2", 157},
      {"pale_turquoise1", 159},
      {"red3", 160},
      {"deep_pink3", 162},
      {"magenta3", 164},
      {"dark_orange3", 166},
      {"indian_red", 167},
      {"hot_pink3", 168},
      {"hot_pink2", 169},
      {"orchid", 170},
      {"orange3", 172},
      {"light_salmon3", 173},
      {"light_pink3", 174},
      {"pink3", 175},
      {"plum3", 176},
      {"violet", 177},
      {"gold3", 178},
      {"light_goldenrod3", 179},
      {"tan", 180},
      {"misty_rose3", 181},
      {"thistle3", 182},
      {"plum2", 183},
      {"yellow3", 184},
      {"khaki3", 185},
      {"light_yellow3", 187},
      {"gray84", 188},
      {"grey84", 188},
      {"light_steel_blue1", 189},
      {"yellow2", 190},
      {"dark_olive_green1", 192},
      {"dark_sea_green1", 193},
      {"honeydew2", 194},
      {"light_cyan1", 195},
      {"red1", 196},
      {"deep_pink2", 197},
      {"deep_pink1", 199},
      {"magenta2", 200},
      {"magenta1", 201},
      {"orange_red1", 202},
      {"indian_red1", 204},
      {"hot_pink", 206},
      {"medium_orchid1", 207},
      {"dark_orange", 208},
      {"salmon1", 209},
      {"light_coral", 210},
      {"pale_violet_red1", 211},
      {"orchid2", 212},
      {"orchid1", 213},
      {"orange1", 214},
      {"sandy_brown", 215},
      {"light_salmon1", 216},
      {"light_pink1", 217},
      {"pink1", 218},
      {"plum1", 219},
      {"gold1", 220},
      {"light_goldenrod2", 222},
      {"navajo_white1", 223},
      {"misty_rose1", 224},
      {"thistle1", 225},
      {"yellow1", 226},
      {"light_goldenrod1", 227},
      {"khaki1", 228},
      {"wheat1", 229},
      {"cornsilk1", 230},
      {"gray100", 231},
      {"grey100", 231},
      {"gray3", 232},
      {"grey3", 232},
      {"gray7", 233},
      {"grey7", 233},
      {"gray11", 234},
      {"grey11", 234},
      {"gray15", 235},
      {"grey15", 235},
      {"gray19", 236},
      {"grey19", 236},
      {"gray23", 237},
      {"grey23", 237},
      {"gray27", 238},
      {"grey27", 238},
      {"gray30", 239},
      {"grey30", 239},
      {"gray35", 240},
      {"grey35", 240},
      {"gray39", 241},
      {"grey39", 241},
      {"gray42", 242},
      {"grey42", 242},
      {"gray46", 243},
      {"grey46", 243},
      {"gray50", 244},
      {"grey50", 244},
      {"gray54", 245},
      {"grey54", 245},
      {"gray58", 246},
      {"grey58", 246},
      {"gray62", 247},
      {"grey62", 247},
      {"gray66", 248},
      {"grey66", 248},
      {"gray70", 249},
      {"grey70", 249},
      {"gray74", 250},
      {"grey74", 250},
      {"gray78", 251},
      {"grey78", 251},
      {"gray82", 252},
      {"grey82", 252},
      {"gray85", 253},
      {"grey85", 253},
      {"gray89", 254},
      {"grey89", 254},
      {"gray93", 255},
      {"grey93", 255},
  };
  return names;
}

}

namespace weqeqq::terminal {

inline Color Color::Parse(std::string_view text) {
  std::string color;
  color.reserve(text.size());
  for (char c : text) {
    color.push_back(
        static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
  }
  auto not_space = [](char c) {
    return !std::isspace(static_cast<unsigned char>(c));
  };
  auto begin = std::find_if(color.begin(), color.end(), not_space);
  auto end = std::find_if(color.rbegin(), color.rend(), not_space).base();
  color = (begin < end) ? std::string(begin, end) : std::string();

  if (color == "default") return Color("default", ColorType::kDefault);

  const auto& names = detail::AnsiColorNames();
  if (auto it = names.find(color); it != names.end()) {
    int n = it->second;
    return Color(std::string(text),
                 n < 16 ? ColorType::kStandard : ColorType::kEightBit, n);
  }

  auto hex_val = [](char c) -> int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
  };

  if (color.size() == 7 && color[0] == '#') {
    int v[6];
    for (int i = 0; i < 6; ++i) {
      v[i] = hex_val(color[i + 1]);
      if (v[i] < 0)
        throw ColorParseError("invalid hex color: " + std::string(text));
    }
    ColorTriplet t{static_cast<std::uint8_t>(v[0] * 16 + v[1]),
                   static_cast<std::uint8_t>(v[2] * 16 + v[3]),
                   static_cast<std::uint8_t>(v[4] * 16 + v[5])};
    return Color(std::string(text), ColorType::kTrueColor, {}, t);
  }

  if (color.rfind("color(", 0) == 0 && color.back() == ')') {
    int n;
    try {
      n = std::stoi(color.substr(6, color.size() - 7));
    } catch (const std::exception&) {
      throw ColorParseError("invalid color number: " + std::string(text));
    }
    if (n < 0 || n > 255)
      throw ColorParseError("color number out of range: " + std::string(text));
    return Color(std::string(text),
                 n < 16 ? ColorType::kStandard : ColorType::kEightBit, n);
  }

  if (color.rfind("rgb(", 0) == 0 && color.back() == ')') {
    std::string inner = color.substr(4, color.size() - 5);
    int comp[3] = {0, 0, 0};
    int idx = 0;
    std::string cur;
    auto flush = [&]() {
      if (idx < 3) {
        try {
          comp[idx++] = cur.empty() ? 0 : std::stoi(cur);
        } catch (const std::exception&) {
          throw ColorParseError("invalid rgb color: " + std::string(text));
        }
      }
      cur.clear();
    };
    for (char c : inner) {
      if (c == ',')
        flush();
      else if (!std::isspace(static_cast<unsigned char>(c)))
        cur.push_back(c);
    }
    flush();
    if (idx != 3)
      throw ColorParseError("invalid rgb color: " + std::string(text));
    ColorTriplet t{static_cast<std::uint8_t>(comp[0]),
                   static_cast<std::uint8_t>(comp[1]),
                   static_cast<std::uint8_t>(comp[2])};
    return Color(std::string(text), ColorType::kTrueColor, {}, t);
  }

  throw ColorParseError("unable to parse color: " + std::string(text));
}

}
