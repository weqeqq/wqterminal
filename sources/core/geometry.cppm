module;

#include <algorithm>

export module weqeqq.terminal:geometry;

export namespace weqeqq::terminal {

struct Offset {
  int x = 0;
  int y = 0;
  constexpr bool operator==(const Offset&) const = default;
  constexpr Offset operator+(const Offset& o) const {
    return {x + o.x, y + o.y};
  }
  constexpr Offset operator-(const Offset& o) const {
    return {x - o.x, y - o.y};
  }
};

struct Size {
  int width = 0;
  int height = 0;
  constexpr bool operator==(const Size&) const = default;
  constexpr int Area() const { return width * height; }
};

struct Region {
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;

  constexpr bool operator==(const Region&) const = default;

  constexpr int Right() const { return x + width; }
  constexpr int Bottom() const { return y + height; }
  constexpr Offset ToOffset() const { return {x, y}; }
  constexpr Size ToSize() const { return {width, height}; }

  constexpr bool Contains(int px, int py) const {
    return px >= x && px < Right() && py >= y && py < Bottom();
  }

  constexpr Region Intersect(const Region& o) const {
    int nx = std::max(x, o.x);
    int ny = std::max(y, o.y);
    int nr = std::min(Right(), o.Right());
    int nb = std::min(Bottom(), o.Bottom());
    return Region{nx, ny, std::max(0, nr - nx), std::max(0, nb - ny)};
  }
};

}
