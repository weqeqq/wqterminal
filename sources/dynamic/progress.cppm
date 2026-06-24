module;

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <optional>
#include <string>
#include <utility>
#include <vector>

export module weqeqq.terminal:progress;

import :protocol;
import :segment;
import :style;
import :text;
import :measure;
import :console;
import :live;
import :group;
import :cells;
import :filesize;
import :spinner;

export namespace weqeqq::terminal {

class ProgressBar : public IRenderable {
 public:
  double total = 100.0;
  double completed = 0.0;
  int width = -1;
  Style complete_style = Style::Parse("magenta");
  Style finished_style = Style::Parse("green");
  Style back_style = Style::Parse("color(237)");

  ProgressBar() = default;
  ProgressBar(double total, double completed)
      : total(total), completed(completed) {}

  RenderResult Render(Console&, const ConsoleOptions& options) const override {
    int w = width > 0 ? width : options.max_width;
    if (w <= 0) return {};
    double done = std::min(total, std::max(0.0, completed));
    double ratio = total > 0 ? done / total : 0.0;
    int halves = static_cast<int>(static_cast<double>(w) * 2.0 * ratio);
    int bar_count = halves / 2;
    int half = halves % 2;
    Style fg = (done >= total && total > 0) ? finished_style : complete_style;

    RenderResult out;
    if (bar_count > 0) {
      std::string s;
      for (int i = 0; i < bar_count; ++i) s += "━";
      out.emplace_back(s, fg);
    }
    if (half) out.emplace_back("╸", fg);
    int remaining = w - bar_count - half;
    if (remaining > 0) {
      std::string s;
      if (half == 0 && bar_count > 0 && remaining > 0) {
      }
      for (int i = 0; i < remaining; ++i) s += "━";
      out.emplace_back(s, back_style);
    }
    return out;
  }

  Measurement Measure(Console&, const ConsoleOptions& options) const override {
    int w = width > 0 ? width : 4;
    return Measurement{4, std::max(4, w)}.WithMaximum(options.max_width);
  }
};

enum class ProgressColumn {
  kDescription,
  kBar,
  kPercentage,
  kCompleted,
  kMofN,
  kElapsed,
  kRemaining,
  kSpinner,
  kDownloadSize,
  kTransferSpeed,
};

struct ProgressTask {
  int id = 0;
  std::string description;
  double total = 100.0;
  double completed = 0.0;
  std::chrono::steady_clock::time_point start =
      std::chrono::steady_clock::now();

  bool Finished() const { return completed >= total; }
  double Percentage() const {
    return total > 0 ? std::min(100.0, completed / total * 100.0) : 100.0;
  }
  double Elapsed() const {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() -
                                         start)
        .count();
  }
  double Speed() const {
    double e = Elapsed();
    return e > 0 ? completed / e : 0.0;
  }
  double RemainingSeconds() const {
    double s = Speed();
    return s > 0 ? (total - completed) / s : -1.0;
  }
};

}

namespace weqeqq::terminal::detail {

inline std::string FormatTime(double secs) {
  if (secs < 0) return "-:--";
  int s = static_cast<int>(secs);
  int h = s / 3600, m = (s % 3600) / 60, ss = s % 60;
  char buf[24];
  if (h > 0)
    std::snprintf(buf, sizeof(buf), "%d:%02d:%02d", h, m, ss);
  else
    std::snprintf(buf, sizeof(buf), "%d:%02d", m, ss);
  return std::string(buf);
}

class TaskRow : public IRenderable {
 public:
  ProgressTask task;
  std::vector<ProgressColumn> columns;
  int desc_width;

  TaskRow(ProgressTask t, std::vector<ProgressColumn> columns, int desc_width)
      : task(std::move(t)),
        columns(std::move(columns)),
        desc_width(desc_width) {}

  struct Piece {
    bool is_bar = false;
    Text text;
    int width = -1;
  };

  RenderResult Render(Console& console,
                      const ConsoleOptions& options) const override {
    std::vector<Piece> pieces;
    for (ProgressColumn col : columns) pieces.push_back(MakePiece(col));

    int fixed = 0, bars = 0;
    for (const Piece& p : pieces) {
      if (p.is_bar)
        ++bars;
      else
        fixed += p.width >= 0 ? p.width : p.text.CellLength();
    }
    int spaces = static_cast<int>(pieces.size()) - 1;
    int bar_w =
        bars > 0 ? std::max(4, (options.max_width - fixed - spaces) / bars) : 0;

    RenderResult out;
    for (std::size_t i = 0; i < pieces.size(); ++i) {
      const Piece& p = pieces[i];
      if (p.is_bar) {
        ProgressBar bar(task.total, task.completed);
        bar.width = bar_w;
        RenderResult bs = bar.Render(console, options.UpdateWidth(bar_w));
        out.insert(out.end(), bs.begin(), bs.end());
      } else if (p.width >= 0) {
        std::vector<Line> dl = RenderLines(Renderable(p.text), console,
                                           options.UpdateWidth(p.width), true);
        if (!dl.empty()) out.insert(out.end(), dl[0].begin(), dl[0].end());
      } else {
        RenderResult segs = p.text.RenderLine();
        out.insert(out.end(), segs.begin(), segs.end());
      }
      if (i + 1 < pieces.size()) out.emplace_back(" ");
    }
    out.push_back(Segment("\n"));
    return out;
  }

  Measurement Measure(Console&, const ConsoleOptions& options) const override {
    return Measurement{options.max_width, options.max_width};
  }

 private:
  Piece MakePiece(ProgressColumn col) const {
    auto right4 = [](std::string s) {
      while (CellLen(s) < 4) s = " " + s;
      return s;
    };
    switch (col) {
      case ProgressColumn::kDescription:
        return Piece{false, Text(task.description), desc_width};
      case ProgressColumn::kBar:
        return Piece{true, Text(), -1};
      case ProgressColumn::kPercentage:
        return Piece{
            false,
            Text(right4(std::to_string(static_cast<int>(task.Percentage())) +
                        "%")),
            -1};
      case ProgressColumn::kCompleted:
        return Piece{
            false, Text(std::to_string(static_cast<int>(task.completed))), -1};
      case ProgressColumn::kMofN:
        return Piece{false,
                     Text(std::to_string(static_cast<int>(task.completed)) +
                          "/" + std::to_string(static_cast<int>(task.total))),
                     -1};
      case ProgressColumn::kElapsed:
        return Piece{false, Text(detail::FormatTime(task.Elapsed())), -1};
      case ProgressColumn::kRemaining:
        return Piece{false, Text(detail::FormatTime(task.RemainingSeconds())),
                     -1};
      case ProgressColumn::kDownloadSize:
        return Piece{
            false,
            Text(DecimalSize(static_cast<long long>(task.completed)) + "/" +
                 DecimalSize(static_cast<long long>(task.total))),
            -1};
      case ProgressColumn::kTransferSpeed:
        return Piece{
            false,
            Text(DecimalSize(static_cast<long long>(task.Speed())) + "/s"), -1};
      case ProgressColumn::kSpinner:
      default: {
        Spinner sp("dots");
        std::size_t idx =
            sp.frames.empty()
                ? 0
                : static_cast<std::size_t>(task.Elapsed() / sp.interval) %
                      sp.frames.size();
        return Piece{false, Text(sp.frames.empty() ? "" : sp.frames[idx]), -1};
      }
    }
  }
};

}

export namespace weqeqq::terminal {

class Progress {
 public:
  std::vector<ProgressColumn> columns = {
      ProgressColumn::kDescription, ProgressColumn::kBar,
      ProgressColumn::kPercentage, ProgressColumn::kRemaining};

  explicit Progress(Console& console)
      : live_(console, Renderable(MakeGroup())) {}

  int AddTask(const std::string& description, double total = 100.0,
              double completed = 0.0) {
    ProgressTask t;
    t.id = next_id_++;
    t.description = description;
    t.total = total;
    t.completed = completed;
    tasks_.push_back(t);
    return t.id;
  }

  void Update(int task_id, std::optional<double> completed = {},
              std::optional<double> advance = {}) {
    for (ProgressTask& t : tasks_) {
      if (t.id == task_id) {
        if (completed) t.completed = *completed;
        if (advance) t.completed += *advance;
        t.completed = std::clamp(t.completed, 0.0, t.total);
        break;
      }
    }
    Refresh();
  }

  void Advance(int task_id, double step = 1.0) { Update(task_id, {}, step); }

  template <class Fn>
  void Track(const std::string& description, int total, Fn&& body) {
    int id = AddTask(description, static_cast<double>(total));
    Start();
    for (int i = 0; i < total; ++i) {
      body(i);
      Advance(id);
    }
    Stop();
  }

  bool Finished() const {
    for (const ProgressTask& t : tasks_)
      if (!t.Finished()) return false;
    return true;
  }

  void Start() { live_.Start(); }
  void Stop() { live_.Stop(); }
  void Refresh() { live_.Update(Renderable(MakeGroup())); }

  const std::vector<ProgressTask>& Tasks() const { return tasks_; }

 private:
  std::vector<ProgressTask> tasks_;
  int next_id_ = 0;
  Live live_;

  Group MakeGroup() const {
    int desc_width = 0;
    for (const ProgressTask& t : tasks_)
      desc_width = std::max(desc_width, CellLen(t.description));
    Group g;
    for (const ProgressTask& t : tasks_)
      g.Add(Renderable(detail::TaskRow(t, columns, desc_width)));
    return g;
  }
};

}
