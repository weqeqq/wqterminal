module;

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

export module weqeqq.terminal:live;

import :protocol;
import :segment;
import :console;
import :control;

export namespace weqeqq::terminal {

enum class VerticalOverflow { kCrop, kEllipsis, kVisible };

class Live {
 public:
  bool transient = false;
  double refresh_per_second = 0.0;
  VerticalOverflow vertical_overflow = VerticalOverflow::kEllipsis;

  Live(Console& console, Renderable renderable)
      : console_(&console), renderable_(std::move(renderable)) {}

  ~Live() { Stop(); }

  Live(const Live&) = delete;
  Live& operator=(const Live&) = delete;

  void SetRenderable(Renderable r) {
    std::lock_guard<std::mutex> lk(mutex_);
    renderable_ = std::move(r);
  }

  void Start() {
    std::lock_guard<std::mutex> lk(mutex_);
    if (started_) return;
    started_ = true;
    console_->HideCursor();
    RefreshLocked();
    if (refresh_per_second > 0.0) {
      running_ = true;
      thread_ = std::thread([this] { AutoLoop(); });
    }
  }

  void Update(Renderable r, bool do_refresh = true) {
    std::lock_guard<std::mutex> lk(mutex_);
    renderable_ = std::move(r);
    if (do_refresh) RefreshLocked();
  }

  void Refresh() {
    std::lock_guard<std::mutex> lk(mutex_);
    RefreshLocked();
  }

  void Stop() {
    std::thread worker;
    {
      std::lock_guard<std::mutex> lk(mutex_);
      if (!started_) return;
      started_ = false;
      running_ = false;
      worker = std::move(thread_);
    }
    if (worker.joinable()) worker.join();
    std::lock_guard<std::mutex> lk(mutex_);
    if (transient) {
      std::string out = control::MoveUp(last_height_);
      out += control::CarriageReturn();
      out += control::EraseDown();
      console_->WriteRaw(out);
    }
    console_->ShowCursor();
  }

  Console& GetConsole() { return *console_; }

 private:
  Console* console_;
  Renderable renderable_;
  bool started_ = false;
  int last_height_ = 0;
  std::mutex mutex_;
  std::thread thread_;
  std::atomic<bool> running_{false};

  void AutoLoop() {
    using namespace std::chrono;
    auto interval =
        duration_cast<milliseconds>(duration<double>(1.0 / refresh_per_second));
    while (running_) {
      std::this_thread::sleep_for(interval);
      std::lock_guard<std::mutex> lk(mutex_);
      if (!running_) break;
      RefreshLocked();
    }
  }

  void RefreshLocked() {
    if (!started_) return;
    std::vector<Segment> segs =
        console_->Render(renderable_, console_->Options());
    std::vector<Line> lines = SplitLines(segs);

    int avail = console_->height;
    if (vertical_overflow != VerticalOverflow::kVisible && avail > 0 &&
        static_cast<int>(lines.size()) > avail) {
      if (vertical_overflow == VerticalOverflow::kEllipsis && avail >= 1) {
        lines.resize(avail - 1);
        lines.push_back(Line{Segment("…")});
      } else {
        lines.resize(avail);
      }
    }

    std::string out;
    out += control::MoveUp(last_height_);
    for (Line& line : lines) {
      out += control::CarriageReturn();
      out += control::EraseLine();
      out += console_->SegmentsToString(line);
      out += "\n";
    }
    out += control::EraseDown();
    console_->WriteRaw(out);
    last_height_ = static_cast<int>(lines.size());
  }
};

}
